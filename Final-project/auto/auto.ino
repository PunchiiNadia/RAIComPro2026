// =====================================================
// COMPLETE AUTO ROBOT CODE (ALL SENSORS FIXED & NON-CONFLICTING)
// - Front Ultrasonic : D11 & D12
// - Side Ultrasonic  : A0 & A1 (and 13/A0, 4/13, 4/A0 — never touches A2!)
// - IR Sensor        : A2 (Black range: 0 - 45 -> forward 500ms -> stop)
// - Motor Speed      : Left = 130, Right = 68 (Right slower than Left)
// =====================================================

// ---------- Structures ----------
struct Motor {
  byte enPin;                    // L298N ENA / ENB (PWM)
  byte in1Pin;                   // L298N IN1 / IN3
  byte in2Pin;                   // L298N IN2 / IN4
  byte encPin;                   // encoder input (interrupt pin)
  volatile unsigned long ticks;  // encoder count
  volatile unsigned long lastPulseUs;
};

struct Step {
  bool forward;                  // true = forward, false = backward
  int leftPwm;                   // ความเร็วล้อซ้าย
  int rightPwm;                  // ความเร็วล้อขวา (ต่ำกว่าล้อซ้าย)
  unsigned long maxMs;           // เวลาสูงสุดของก้าวนี้
  unsigned int pauseMs;          // เวลาหยุดพักหลังจบก้าว
  float stopDistanceCm;          // >0: หยุดเมื่อเจอวัตถุในระยะนี้
};

struct PinPair {
  byte trigPin;
  byte echoPin;
  const char *label;
};

// ---------- 1. Hardware: Motors & Encoders ----------
const byte MOTOR_COUNT = 2;
const byte A = 0; // Left motor (พิน 5, 7, 8)
const byte B = 1; // Right motor (พิน 6, 9, 10)

Motor motors[MOTOR_COUNT] = {
  { 5, 7,  8, 2, 0, 0 },   // motor A (Left - มอเตอร์ฝืด)
  { 6, 9, 10, 3, 0, 0 }    // motor B (Right - ลดความเร็วให้ต่ำกว่าซ้าย)
};

// ---------- 2. Hardware: Front & Side Ultrasonic Sensors ----------
// Front Ultrasonic (Pins 11 & 12)
const byte FRONT_TRIG_PIN = 11;
const byte FRONT_ECHO_PIN = 12;

// Side Ultrasonic (ตัด A2 ออกเพื่อไม่ให้ไปกวน IR Sensor ที่พอร์ต A2!)
const PinPair SIDE_CANDIDATES[] = {
  { A0, A1, "A0 & A1" },
  { 13, A0, "13 & A0" },
  { A0, A3, "A0 & A3" },
  {  4, 13, "D4 & D13" },
  {  4, A0, "D4 & A0" }
};
const byte CANDIDATE_COUNT = sizeof(SIDE_CANDIDATES) / sizeof(SIDE_CANDIDATES[0]);

// ---------- 3. Hardware: IR Sensor (Port A2) ----------
const byte IR_PIN    = A2;
const int  BLACK_MIN = 0;
const int  BLACK_MAX = 45;

// ---------- 4. ตั้งค่าความเร็วมอเตอร์ (ล้อขวาต่ำกว่าล้อซ้าย) และระยะเซนเซอร์ ----------
const int LEFT_FORWARD_PWM  = 130; // ความเร็วเดินหน้า ล้อซ้าย
const int RIGHT_FORWARD_PWM = 68;  // ความเร็วเดินหน้า ล้อขวา (ต่ำกว่าซ้าย)

const int LEFT_TURN_PWM     = 135; // ความเร็วเลี้ยว ล้อซ้าย
const int RIGHT_TURN_PWM    = 72;  // ความเร็วเลี้ยว ล้อขวา (ต่ำกว่าซ้าย)

const float FRONT_GAP_CM          = 8.0f;   // ระยะห่างกำแพงด้านหน้า 8 cm
const float SIDE_GAP_CM           = 5.0f;   // ระยะห่างกำแพงด้านข้าง 5 cm
const unsigned long FRONT_HOLD_MS = 3000UL; // เจอกำแพงหน้าครบ 3 วินาที -> เลี้ยวขวา

const int   TURN_60_MS     = 320;  // เวลาหมุนเลี้ยวซ้าย 60 องศา (ms)
const int   TURN_90_MS     = 460;  // เวลาหมุนเลี้ยวขวา 90 องศา (ms)
const int   TURN_PAUSE_MS  = 250;

const unsigned long MATCH_LIMIT_MS = 89000UL;
unsigned long matchStartMs = 0;

// ---------- 5. Mission Steps ----------
const Step MISSION[] = {
  // forward, leftPwm, rightPwm, maxMs, pauseMs, stopDistanceCm
  { true,  LEFT_FORWARD_PWM, RIGHT_FORWARD_PWM, 15000, 250, FRONT_GAP_CM }, // เดินหน้า
  { false, LEFT_FORWARD_PWM, RIGHT_FORWARD_PWM,   350, 250,         0.0f }  // ถอยตั้งลำ 350 ms
};

const long CONTROL_MS = 20;
const unsigned long MIN_PULSE_US = 200;
const char *lastSidePins = "NONE";

// ---------- Encoders ----------
void countTick(Motor *m) {
  unsigned long now = micros();
  if (now - m->lastPulseUs >= MIN_PULSE_US) {
    m->ticks++;
    m->lastPulseUs = now;
  }
}

void encoderA_ISR() { countTick(&motors[A]); }
void encoderB_ISR() { countTick(&motors[B]); }

void resetTicks() {
  noInterrupts();
  for (byte i = 0; i < MOTOR_COUNT; i++) {
    motors[i].ticks = 0;
  }
  interrupts();
}

// ---------- Motors ----------
void setMotor(const Motor *m, bool forward, int pwm) {
  digitalWrite(m->in1Pin, forward ? HIGH : LOW);
  digitalWrite(m->in2Pin, forward ? LOW : HIGH);
  analogWrite(m->enPin, constrain(pwm, 0, 255));
}

void stopMotors() {
  for (byte i = 0; i < MOTOR_COUNT; i++) {
    const Motor *m = &motors[i];
    analogWrite(m->enPin, 0);
    digitalWrite(m->in1Pin, LOW);
    digitalWrite(m->in2Pin, LOW);
  }
}

void brakeMotors() {
  for (byte i = 0; i < MOTOR_COUNT; i++) {
    const Motor *m = &motors[i];
    digitalWrite(m->in1Pin, LOW);
    digitalWrite(m->in2Pin, LOW);
    analogWrite(m->enPin, 255);
  }
}

// ---------- IR Sensor (Port A2) & 89s Safety Cutoff ----------
int readIR() {
  return analogRead(IR_PIN);
}

void checkSafetyAndParking() {
  int irValue   = readIR();
  bool isBlack  = (irValue >= BLACK_MIN && irValue <= BLACK_MAX);
  bool isTimeUp = (millis() - matchStartMs >= MATCH_LIMIT_MS);

  if (isBlack || isTimeUp) {
    if (isBlack) {
      Serial.print(F("BLACK TAPE DETECTED ON A2! Value = "));
      Serial.println(irValue);

      // เดินหน้าต่ออีก 0.5 วินาที (500 ms) เข้าโซนจอด
      setMotor(&motors[A], true, LEFT_FORWARD_PWM);
      setMotor(&motors[B], true, RIGHT_FORWARD_PWM);
      delay(500);
    } else {
      Serial.println(F("89s TIME LIMIT REACHED!"));
    }

    for (byte i = 0; i < MOTOR_COUNT; i++) {
      digitalWrite(motors[i].in1Pin, HIGH);
      digitalWrite(motors[i].in2Pin, HIGH);
      analogWrite(motors[i].enPin, 255);
    }
    delay(150);

    stopMotors();
    Serial.println(F("PARKED IN PARKING ZONE PERMANENTLY."));
    while (true) {}
  }
}

// ---------- Ultrasonic Sensors ----------
unsigned long pingPins(byte trig, byte echo) {
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long dur = pulseIn(echo, HIGH, 10000UL);

  if (dur == 0 && digitalRead(echo) == HIGH) {
    pinMode(echo, OUTPUT);
    digitalWrite(echo, LOW);
    delayMicroseconds(50);
    pinMode(echo, INPUT);
  }
  return dur;
}

float readPairCm(byte trig, byte echo) {
  unsigned long duration = pingPins(trig, echo);
  if (duration == 0) {
    duration = pingPins(echo, trig);
  }
  if (duration == 0) {
    return 999.0f;
  }
  float cm = duration * 0.0343f / 2.0f;
  if (cm < 2.0f) return 999.0f;
  return cm;
}

float getFrontDistanceCm() {
  return readPairCm(FRONT_TRIG_PIN, FRONT_ECHO_PIN);
}

float getSideDistanceCm() {
  float bestCm = 999.0f;
  for (byte i = 0; i < CANDIDATE_COUNT; i++) {
    float cm = readPairCm(SIDE_CANDIDATES[i].trigPin, SIDE_CANDIDATES[i].echoPin);
    if (cm < bestCm) {
      bestCm = cm;
      lastSidePins = SIDE_CANDIDATES[i].label;
    }
  }
  return bestCm;
}

// ---------- Turn Function ----------
void turnByDegrees(bool turnLeft, int degrees) {
  unsigned long durationMs = (degrees <= 60) ? (unsigned long)TURN_60_MS
                                             : (unsigned long)TURN_90_MS;
  unsigned long startMs = millis();

  Serial.print(turnLeft ? F("TURN LEFT ") : F("TURN RIGHT "));
  Serial.print(degrees);
  Serial.println(F(" deg"));

  while (millis() - startMs < durationMs) {
    checkSafetyAndParking();

    if (turnLeft) {
      setMotor(&motors[A], false, LEFT_TURN_PWM);
      setMotor(&motors[B], true,  RIGHT_TURN_PWM);
    } else {
      setMotor(&motors[A], true,  LEFT_TURN_PWM);
      setMotor(&motors[B], false, RIGHT_TURN_PWM);
    }
    delay(10);
  }

  brakeMotors();
  delay(180);
  stopMotors();
  delay(TURN_PAUSE_MS);
}

// ---------- Drive Step ----------
void runStep(const Step *s) {
  resetTicks();

  unsigned long startTime = millis();
  unsigned long nextTime  = startTime;
  byte usDivider = 0;
  byte debugDivider = 0;
  float lastDist = 999.0f;

  while (true) {
    unsigned long now = millis();
    if ((long)(now - nextTime) < 0) continue;
    nextTime += CONTROL_MS;

    // 1. เช็ก IR Sensor ที่พอร์ต A2 ตลอดเวลา
    checkSafetyAndParking();

    unsigned long elapsed = now - startTime;
    if (elapsed >= s->maxMs) break;

    // 2. เช็ก Ultrasonic หน้าและข้าง
    if (s->stopDistanceCm > 0.0f && (++usDivider >= 3)) {
      usDivider = 0;
      float frontCm = getFrontDistanceCm();
      float sideCm  = getSideDistanceCm();
      nextTime = millis() + CONTROL_MS;

      if (frontCm <= FRONT_GAP_CM || sideCm <= SIDE_GAP_CM) {
        lastDist = (frontCm < sideCm) ? frontCm : sideCm;
        break;
      }
      lastDist = (frontCm < sideCm) ? frontCm : sideCm;
    }

    setMotor(&motors[A], s->forward, s->leftPwm);
    setMotor(&motors[B], s->forward, s->rightPwm);

    if (++debugDivider >= 5) {
      debugDivider = 0;
      Serial.print(F("Side("));
      Serial.print(lastSidePins);
      Serial.print(F(") Dist="));
      Serial.print(lastDist);
      Serial.print(F("cm | IR_A2="));
      Serial.println(readIR());
    }
  }

  brakeMotors();
  delay(200);
  stopMotors();
  delay(s->pauseMs);
}

// ---------- Maze Decision (Front 8cm 3s Right / Side > 5cm Left 60 deg) ----------
void executeMazeDecision() {
  delay(50);
  float frontCm = getFrontDistanceCm();
  float sideCm  = getSideDistanceCm();

  if (frontCm <= FRONT_GAP_CM) {
    unsigned long holdStart = millis();
    bool heldFor3Sec = true;

    while (millis() - holdStart < FRONT_HOLD_MS) {
      checkSafetyAndParking();
      if (getFrontDistanceCm() > FRONT_GAP_CM + 2.0f) {
        heldFor3Sec = false;
        break;
      }
      delay(50);
    }

    if (heldFor3Sec) {
      Serial.println(F("[FRONT <= 8cm FOR 3s] -> Reverse & Turn RIGHT 90 deg"));
      runStep(&MISSION[1]);
      turnByDegrees(false, 90);
      return;
    }
  }

  if (sideCm > SIDE_GAP_CM) {
    Serial.println(F("[SIDE > 5cm] -> Turn LEFT 60 deg"));
    turnByDegrees(true, 60);
  } else {
    runStep(&MISSION[1]);
    turnByDegrees(false, 90);
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < MOTOR_COUNT; i++) {
    const Motor *m = &motors[i];
    pinMode(m->enPin, OUTPUT);
    pinMode(m->in1Pin, OUTPUT);
    pinMode(m->in2Pin, OUTPUT);
    pinMode(m->encPin, INPUT_PULLUP);
  }

  pinMode(IR_PIN, INPUT);
  stopMotors();

  attachInterrupt(digitalPinToInterrupt(motors[A].encPin), encoderA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(motors[B].encPin), encoderB_ISR, RISING);

  Serial.println(F("=== AUTO ROBOT (IR=A2, SIDE=A0/A1, FRONT=11/12) READY ==="));
  delay(2000);
  matchStartMs = millis();
}

// ---------- Main Loop ----------
void loop() {
  runStep(&MISSION[0]);
  executeMazeDecision();
}