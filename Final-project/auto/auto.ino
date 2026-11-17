// =====================================================
// COMPLETE AUTO ROBOT CODE (FIXED SIDE + FRONT ULTRASONIC + LSRB + IR)
// =====================================================

// ---------- Path Pattern Enum ----------
enum PathPattern {
  PAT_STRAIGHT_CLEAR    = 0,
  PAT_CORNER_LEFT_90    = 1,
  PAT_CORNER_RIGHT_90   = 2,
  PAT_DEAD_END_180      = 3,
  PAT_WALL_TOO_CLOSE_45 = 4
};

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
  int basePwm;                   // cruising PWM
  int trim;                      // PWM offset: positive slows A, speeds up B
  int scaleA;                    // per-mille weight of A's ticks (1000 = neutral)
  float kp;                      // PID gains for this leg
  float ki;
  float kd;
  unsigned long maxMs;           // time limit for this leg
  unsigned long targetTicks;     // >0: stop at this tick count instead
  unsigned int pauseMs;          // pause after this leg
  float stopDistanceCm;          // >0: stop if obstacle <= this distance (cm)
};

struct Pid {
  float kp;
  float ki;
  float kd;
  float integral;
  long lastError;
};

struct PinPair {
  byte trigPin;
  byte echoPin;
  const char *label;
};

// ---------- 1. Hardware: Motors & Encoders ----------
const byte MOTOR_COUNT = 2;
const byte A = 0; // Left motor
const byte B = 1; // Right motor

Motor motors[MOTOR_COUNT] = {
  { 5, 7,  8, 2, 0, 0 },   // motor A (Left)
  { 6, 9, 10, 3, 0, 0 }    // motor B (Right)
};

// ---------- 2. Hardware: Front & Side Ultrasonic Sensors ----------
// Front Ultrasonic (Pins 11 & 12 - จาก Test Code 1)
const byte FRONT_TRIG_PIN = 11;
const byte FRONT_ECHO_PIN = 12;

// Side Ultrasonic (ใส่คู่พินครบทุกตัวเหมือนใน Test Code 2 ที่รันผ่านแล้ว 100%)
const PinPair SIDE_CANDIDATES[] = {
  { A0, A2, "A0 & A2" },
  { 13, A0, "13 & A0" },
  { A0, A1, "A0 & A1" },
  { A2, A3, "A2 & A3" },
  {  4, 13, "D4 & D13" },
  {  4, A0, "D4 & A0" }
};
const byte CANDIDATE_COUNT = sizeof(SIDE_CANDIDATES) / sizeof(SIDE_CANDIDATES[0]);

// ---------- 3. Hardware: IR Sensor ----------
// ตั้งช่วง BLACK_MIN = 35 ถึง BLACK_MAX = 45 (ครอบคลุมค่าจริง 39-43 และไม่ชนกับค่า 0 ของ Ultrasonic)
const byte IR_PIN       = A1;
const int  BLACK_MIN    = 35;
const int  BLACK_MAX    = 45;
const int  IR_DRIVE_PWM = 110;

// ---------- 4. Maze Dimensions (20x20 cm) & 45/90 Degree Turn Settings ----------
const float FRONT_WALL_CM     = 18.0f; // ระยะหยุดเมื่อเจอวัตถุ/กำแพง (ซม.)
const float FRONT_DIAGONAL_CM = 21.0f;
const float SIDE_OPEN_CM      = 20.0f; // ถ้าระยะด้านข้าง > 20 cm แปลว่าทางซ้ายเปิดโล่ง
const float SIDE_TOO_CLOSE_CM = 4.5f;

const int   TURN_PWM          = 125;   // ความเร็วมอเตอร์ขณะหมุนตัว
const int   TURN_90_MS        = 420;   // เวลาหมุนตัวเลี้ยว 90 องศา (ms)
const int   TURN_45_MS        = 210;   // เวลาหมุนตัวเลี้ยว 45 องศา (ms)
const int   TURN_PAUSE_MS     = 250;   // เวลาหยุดพักตั้งลำหลังเลี้ยวเสร็จ (ms)

const unsigned long MATCH_LIMIT_MS = 89000UL;
unsigned long matchStartMs = 0;

// ---------- 5. Mission Steps ----------
const Step MISSION[] = {
  // forward, pwm, trim, scaleA,   kp,   ki,   kd, maxMs, targetTicks, pauseMs, stopDistanceCm
  { true,  145, 0, 1035, 8.0f, 4.0f, 0.0f, 15000, 0, 250, FRONT_WALL_CM }, // เดินหน้าตรง (S)
  { false, 110, 0,  995, 8.0f, 4.0f, 0.0f,   350, 0, 250,          0.0f }  // ถอยตั้งลำ 350 ms
};

const byte STEP_COUNT = sizeof(MISSION) / sizeof(MISSION[0]);

// ---------- Control Settings ----------
const int   START_PWM      = 60;
const long  RAMP_MS        = 200;
const int   MAX_CORRECTION = 80;
const long  CONTROL_MS     = 20;
const float I_LIMIT        = 60.0f;
const unsigned long MIN_PULSE_US = 200;

Pid pid = { 0.0f, 0.0f, 0.0f, 0.0f, 0 };
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

void readTicks(unsigned long *out) {
  noInterrupts();
  for (byte i = 0; i < MOTOR_COUNT; i++) {
    out[i] = motors[i].ticks;
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

// ---------- Rulebook 89-Second Safety Cutoff & IR Parking Check ----------
int readIR() {
  pinMode(IR_PIN, INPUT);
  return analogRead(IR_PIN);
}

void checkSafetyAndParking() {
  if (millis() - matchStartMs >= MATCH_LIMIT_MS) {
    brakeMotors();
    delay(150);
    stopMotors();
    Serial.println(F("89s MATCH TIME LIMIT REACHED -> STOPPED SAFELY."));
    while (true) {}
  }

  int irValue = readIR();
  if (irValue >= BLACK_MIN && irValue <= BLACK_MAX) {
    Serial.print(F("BLACK TAPE DETECTED! A1 = "));
    Serial.println(irValue);

    for (byte i = 0; i < MOTOR_COUNT; i++) {
      setMotor(&motors[i], true, IR_DRIVE_PWM);
    }
    delay(500);

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

// ---------- Ultrasonic Sensors (เหมือนใน Test Code 1 & Test Code 2 เป๊ะ) ----------
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

// อ่านทั้งตัวหน้าและตัวข้าง (เพื่อให้ทั้ง 2 ตัวตอบสนองได้เหมือนตอนรัน Test Code 1 & 2)
float getAnyObstacleDistanceCm() {
  float frontCm = getFrontDistanceCm();
  float sideCm  = getSideDistanceCm();
  return (frontCm < sideCm) ? frontCm : sideCm;
}

// ---------- Turn Function Supporting Both 45 and 90 Degrees ----------
void turnByDegrees(bool turnLeft, int degrees) {
  unsigned long durationMs = (degrees <= 45) ? (unsigned long)TURN_45_MS
                                             : (unsigned long)TURN_90_MS;

  resetTicks();
  unsigned long ticks[MOTOR_COUNT];
  unsigned long startMs = millis();

  Serial.print(turnLeft ? F("TURN LEFT ") : F("TURN RIGHT "));
  Serial.print(degrees);
  Serial.println(F(" deg"));

  while (millis() - startMs < durationMs) {
    checkSafetyAndParking();

    readTicks(ticks);
    long syncError = (long)ticks[A] - (long)ticks[B];
    int syncCorr   = constrain((int)(syncError * 4), -30, 30);

    int pwmA = constrain(TURN_PWM - syncCorr, 70, 200);
    int pwmB = constrain(TURN_PWM + syncCorr, 70, 200);

    if (turnLeft) {
      setMotor(&motors[A], false, pwmA);
      setMotor(&motors[B], true,  pwmB);
    } else {
      setMotor(&motors[A], true,  pwmA);
      setMotor(&motors[B], false, pwmB);
    }
    delay(10);
  }

  brakeMotors();
  delay(180);
  stopMotors();
  delay(TURN_PAUSE_MS);
}

// ---------- Path Pattern Classifier & Decision Engine (45° & 90° LSRB) ----------
PathPattern classifyPathPattern(float sideCm) {
  if (sideCm > SIDE_OPEN_CM) {
    return PAT_CORNER_LEFT_90;
  }
  return PAT_CORNER_RIGHT_90;
}

void executeMazeDecision() {
  delay(50);
  float sideCm = getSideDistanceCm();

  Serial.print(F("Side ("));
  Serial.print(lastSidePins);
  Serial.print(F(")="));
  Serial.print(sideCm);
  Serial.println(F(" cm"));

  PathPattern pattern = classifyPathPattern(sideCm);

  if (pattern == PAT_CORNER_LEFT_90) {
    Serial.println(F("[Pattern 1] Left Open -> Turn LEFT 90 deg"));
    turnByDegrees(true, 90);

    delay(40);
    float frontAfterLeft = getFrontDistanceCm();
    if (frontAfterLeft <= FRONT_WALL_CM && getSideDistanceCm() > SIDE_OPEN_CM) {
      Serial.println(F("[Pattern 5] Diagonal adjust -> Turn LEFT 45 deg"));
      turnByDegrees(true, 45);
    }
  } else {
    Serial.println(F("[Pattern 2] Left Wall -> Turn RIGHT 90 deg"));
    turnByDegrees(false, 90);

    delay(50);
    float frontAfterRight = getFrontDistanceCm();

    if (frontAfterRight <= FRONT_WALL_CM) {
      Serial.println(F("[Pattern 3] Dead-End -> Turn RIGHT another 90 deg (180 U-Turn)"));
      turnByDegrees(false, 90);
    } else if (frontAfterRight <= FRONT_DIAGONAL_CM && getSideDistanceCm() <= SIDE_TOO_CLOSE_CM) {
      Serial.println(F("[Pattern 4] Too close to wall -> Adjust RIGHT 45 deg"));
      turnByDegrees(false, 45);
    }
  }
}

// ---------- PID ----------
void pidBegin(Pid *p, const Step *s) {
  p->kp = s->kp;
  p->ki = s->ki;
  p->kd = s->kd;
  p->integral = 0.0f;
  p->lastError = 0;
}

float pidUpdate(Pid *p, long error, float dt) {
  p->integral += error * dt;
  if (p->ki > 0.0f) {
    float iMax = I_LIMIT / p->ki;
    p->integral = constrain(p->integral, -iMax, iMax);
  } else {
    p->integral = 0.0f;
  }
  float derivative = (error - p->lastError) / dt;
  p->lastError = error;
  float out = p->kp * error + p->ki * p->integral + p->kd * derivative;
  return constrain(out, (float)-MAX_CORRECTION, (float)MAX_CORRECTION);
}

// ---------- One Leg of the Mission ----------
void runStep(const Step *s) {
  resetTicks();
  pidBegin(&pid, s);

  const float dt = CONTROL_MS / 1000.0f;
  unsigned long ticks[MOTOR_COUNT];
  int pwm[MOTOR_COUNT];

  unsigned long startTime = millis();
  unsigned long nextTime  = startTime;
  byte usDivider = 0;
  byte debugDivider = 0;
  float lastDist = 999.0f;

  while (true) {
    unsigned long now = millis();
    if ((long)(now - nextTime) < 0) continue;
    nextTime += CONTROL_MS;

    checkSafetyAndParking();

    unsigned long elapsed = now - startTime;
    readTicks(ticks);

    if (elapsed >= s->maxMs) break;
    if (s->targetTicks > 0 && (ticks[A] + ticks[B]) / 2 >= s->targetTicks) break;

    // เช็กทั้งเซนเซอร์หน้าและเซนเซอร์ข้างเหมือนใน Test Code 1 & 2
    if (s->stopDistanceCm > 0.0f && (++usDivider >= 3)) {
      usDivider = 0;
      lastDist = getAnyObstacleDistanceCm();
      nextTime = millis() + CONTROL_MS;
      if (lastDist <= s->stopDistanceCm) {
        Serial.print(F("[ULTRASONIC DETECTED] Dist="));
        Serial.print(lastDist);
        Serial.println(F(" cm -> Stopping!"));
        break;
      }
    }

    int base = s->basePwm;
    if ((long)elapsed < RAMP_MS) {
      base = START_PWM + (int)((long)(s->basePwm - START_PWM) * (long)elapsed / RAMP_MS);
    }

    long weightedA = (long)(ticks[A] * (unsigned long)s->scaleA / 1000UL);
    long error = weightedA - (long)ticks[B];
    int correction = (int)lround(pidUpdate(&pid, error, dt));

    pwm[A] = constrain(base - s->trim - correction, 0, 255);
    pwm[B] = constrain(base + s->trim + correction, 0, 255);

    for (byte i = 0; i < MOTOR_COUNT; i++) {
      setMotor(&motors[i], s->forward, pwm[i]);
    }

    if (++debugDivider >= 5) {
      debugDivider = 0;
      Serial.print(F("SidePins="));
      Serial.print(lastSidePins);
      Serial.print(F(" | MinDist_cm="));
      Serial.print(lastDist);
      Serial.print(F(" | IR_A1="));
      Serial.println(readIR());
    }
  }

  brakeMotors();
  delay(200);
  stopMotors();
  delay(s->pauseMs);
}

// ---------- Whole Maze Navigation Cycle ----------
void runMission(const Step *steps, byte count) {
  for (const Step *s = steps; s < steps + count; s++) {
    runStep(s);
  }
  executeMazeDecision();
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

  Serial.println(F("=== AUTO ROBOT (BOTH ULTRASONICS + LSRB + IR) READY ==="));
  delay(2000);
  matchStartMs = millis();
}

// ---------- Main Loop ----------
void loop() {
  runMission(MISSION, STEP_COUNT);
}