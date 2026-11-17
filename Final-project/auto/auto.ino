// =====================================================
// AUTONOMOUS MAZE DRIVE WITH LSRB (Left-Straight-Right-Back)
// Arduino Uno/Nano + L298N + 2 Encoders + 2x HC-SR04
// =====================================================

// ---------- LSRB Action Enum ----------
enum LsrbAction {
  LSRB_LEFT     = 'L',
  LSRB_STRAIGHT = 'S',
  LSRB_RIGHT    = 'R',
  LSRB_BACK     = 'B'
};

// ---------- Structures ----------
struct Motor {
  byte enPin;                    // L298N ENA / ENB (PWM)
  byte in1Pin;                   // L298N IN1 / IN3
  byte in2Pin;                   // L298N IN2 / IN4
  byte encPin;                   // encoder input (interrupt pin)
  volatile unsigned long ticks;  // encoder count
  volatile unsigned long lastPulseUs;  // time of the last accepted pulse
};

struct Step {
  bool forward;                // true = forward, false = backward
  int basePwm;                 // cruising PWM
  int trim;                    // PWM offset: positive slows A, speeds up B
  int scaleA;                  // per-mille weight of A's ticks (1000 = neutral)
  float kp;                    // PID gains for this leg only
  float ki;
  float kd;
  unsigned long maxMs;         // time limit for this leg
  unsigned long targetTicks;   // >0: stop at this tick count instead
  unsigned int pauseMs;        // pause after this leg
  float stopDistanceCm;        // >0: stop if obstacle is closer than this distance (cm)
};

struct Pid {
  float kp;
  float ki;
  float kd;
  float integral;
  long lastError;
};

// ---------- Hardware ----------
const byte MOTOR_COUNT = 2;
const byte A = 0; // Left motor
const byte B = 1; // Right motor

Motor motors[MOTOR_COUNT] = {
  { 5, 7,  8, 2, 0, 0 },   // motor A (Left)
  { 6, 9, 10, 3, 0, 0 }    // motor B (Right)
};

// ---------- Ultrasonic Sensor Pins (บรรทัด 56-63) ----------
// 1. Front Ultrasonic Sensor
const byte FRONT_TRIG_PIN = 11;
const byte FRONT_ECHO_PIN = 12;
const byte FRONT_ALT_PIN  = 13;

// 2. Left (Side) Ultrasonic Sensor
const byte SIDE_TRIG_PIN  = A0;
const byte SIDE_ECHO_PIN  = A2;

// ---------- LSRB & Smooth Turn Settings (บรรทัด 66-71) ----------
const float FRONT_WALL_CM = 18.0f; // ระยะกำแพงด้านหน้าที่ถือว่าทางตัน (ซม.)
const float SIDE_WALL_CM  = 20.0f; // ระยะกำแพงด้านซ้าย (ถ้า > 20 cm แปลว่าทางซ้ายโล่ง)
const int   TURN_PWM      = 125;   // ความเร็วหลักตอนหมุนเลี้ยว L, R, B
const int   TURN_90_MS    = 420;   // เวลาหมุนเลี้ยว 90 องศา (ms)
const int   TURN_PAUSE_MS = 250;   // เวลาหยุดตั้งลำหลังเลี้ยวเสร็จ (ms)

// ---------- Hardcoded mission steps for Straight & Reverse-Align ----------
const Step MISSION[] = {
  // forward, pwm, trim, scaleA,   kp,   ki,   kd, maxMs, targetTicks, pauseMs, stopDistanceCm
  { true,  145, 0, 1035, 8.0f, 4.0f, 0.0f, 15000, 0, 250, FRONT_WALL_CM }, // Straight (S)
  { false, 105, 0,  995, 8.0f, 4.0f, 0.0f,   350, 0, 250,          0.0f }  // Short reverse before LSRB turn
};

const byte STEP_COUNT = sizeof(MISSION) / sizeof(MISSION[0]);

// ---------- Control settings ----------
const int   START_PWM      = 60;
const long  RAMP_MS        = 200;
const int   MAX_CORRECTION = 80;
const long  CONTROL_MS     = 20;
const float I_LIMIT        = 60.0f;

const unsigned long MIN_PULSE_US = 200;

Pid pid = { 0.0f, 0.0f, 0.0f, 0.0f, 0 };

#define DEBUG 1

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

// ---------- Ultrasonic Sensor (Fixed + Auto Pin-Swap) ----------
unsigned long pingPins(byte trig, byte echo) {
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long dur = pulseIn(echo, HIGH, 12000UL);

  if (dur == 0 && digitalRead(echo) == HIGH) {
    pinMode(echo, OUTPUT);
    digitalWrite(echo, LOW);
    delayMicroseconds(50);
    pinMode(echo, INPUT);
  }
  return dur;
}

float readOneUltrasonic(byte trig, byte echo) {
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
  float d1 = readOneUltrasonic(FRONT_TRIG_PIN, FRONT_ECHO_PIN); // 11, 12
  float d2 = readOneUltrasonic(FRONT_ECHO_PIN, FRONT_ALT_PIN);  // 12, 13
  return (d1 < d2) ? d1 : d2;
}

float getSideDistanceCm() {
  float d1 = readOneUltrasonic(SIDE_TRIG_PIN, SIDE_ECHO_PIN);   // A0, A2
  float d2 = readOneUltrasonic(FRONT_ECHO_PIN, SIDE_TRIG_PIN);  // 12, A0
  return (d1 < d2) ? d1 : d2;
}

float getDistanceCm() {
  float frontCm = getFrontDistanceCm();
  float sideCm  = getSideDistanceCm();
  return (frontCm < sideCm) ? frontCm : sideCm;
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

// ---------- Smooth Encoder-Synced Turn for L, R, B ----------
void turnSmooth90(bool turnLeft) {
  resetTicks();
  unsigned long ticks[MOTOR_COUNT];
  unsigned long startMs = millis();

  while (millis() - startMs < (unsigned long)TURN_90_MS) {
    readTicks(ticks);
    // ใช้ Encoder ล้อซ้าย-ขวาช่วยซิงค์ความเร็วขณะหมุนตัวเพื่อให้หมุนสมูทตรงกลางช่อง
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

// ---------- LSRB Decision Engine (Left -> Straight -> Right -> Back) ----------
LsrbAction decideLsrbAction() {
  float sideCm = getSideDistanceCm();

  // 1. L (Left): ถ้าด้านซ้ายโล่ง ให้เลี้ยวซ้าย 90 องศา
  if (sideCm > SIDE_WALL_CM) {
    return LSRB_LEFT;
  }

  // 2. ถ้าด้านซ้ายมีกำแพงและด้านหน้าตัน ให้ลองเลี้ยวขวา (R) ก่อน แล้วเช็กว่าเป็นซอยตัน (B) หรือไม่
  return LSRB_RIGHT;
}

void executeLsrbTurn() {
  LsrbAction action = decideLsrbAction();

  if (action == LSRB_LEFT) {
#if DEBUG
    Serial.println(F("LSRB -> [L] Left is open: Turning LEFT 90 deg"));
#endif
    turnSmooth90(true);
  } else {
#if DEBUG
    Serial.println(F("LSRB -> [R] Left blocked: Turning RIGHT 90 deg"));
#endif
    turnSmooth90(false);

    // เช็กด้านหน้าหลังเลี้ยวขวา: ถ้ายังเจอทางตันอีก แปลว่าเป็นซอยตัน ต้องกลับหลังหัน [B] (หมุนขวาอีก 90 องศา รวมเป็น 180 องศา)
    float frontAfterRight = getFrontDistanceCm();
    if (frontAfterRight <= FRONT_WALL_CM) {
#if DEBUG
      Serial.println(F("LSRB -> [B] Dead-end detected! Turning another 90 deg (180 U-Turn)"));
#endif
      turnSmooth90(false);
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

// ---------- One leg of the mission ----------
void runStep(const Step *s) {
  resetTicks();
  pidBegin(&pid, s);

  const float dt = CONTROL_MS / 1000.0f;

  unsigned long ticks[MOTOR_COUNT];
  int pwm[MOTOR_COUNT];

  unsigned long startTime = millis();
  unsigned long nextTime  = startTime;
  byte usDivider = 0;
  float lastDist = 999.0f;

#if DEBUG
  byte debugDivider = 0;
  Serial.println(s->forward ? F("--- LSRB [S] Straight ---") : F("--- Reverse Align ---"));
  Serial.println(F("time_ms,dist_cm,error,pwmA,pwmB"));
#endif

  while (true) {
    unsigned long now = millis();
    if ((long)(now - nextTime) < 0) {
      continue;
    }
    nextTime += CONTROL_MS;

    unsigned long elapsed = now - startTime;
    readTicks(ticks);

    // Stop conditions
    if (elapsed >= s->maxMs) break;
    if (s->targetTicks > 0 && (ticks[A] + ticks[B]) / 2 >= s->targetTicks) break;

    // เช็ก Ultrasonic ทุกๆ 60 ms
    if (s->stopDistanceCm > 0.0f && (++usDivider >= 3)) {
      usDivider = 0;
      lastDist = getDistanceCm();
      nextTime = millis() + CONTROL_MS;
      if (lastDist <= s->stopDistanceCm) {
#if DEBUG
        Serial.print(F("Wall at "));
        Serial.print(lastDist);
        Serial.println(F(" cm. Stopping straight leg."));
#endif
        break;
      }
    }

    // Soft-start ramp
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

#if DEBUG
    if (++debugDivider >= 5) {
      debugDivider = 0;
      Serial.print(elapsed);
      Serial.print(',');
      Serial.print(lastDist);
      Serial.print(',');
      Serial.print(error);
      Serial.print(',');
      Serial.print(pwm[A]);
      Serial.print(',');
      Serial.println(pwm[B]);
    }
#endif
  }

  brakeMotors();
  delay(200);
  stopMotors();

#if DEBUG
  readTicks(ticks);
  Serial.print(F("Final ticks A="));
  Serial.print(ticks[A]);
  Serial.print(F(" B="));
  Serial.print(ticks[B]);
  Serial.print(F(" diff="));
  Serial.println((long)ticks[A] - (long)ticks[B]);
#endif

  delay(s->pauseMs);
}

// ---------- Whole LSRB cycle ----------
void runMission(const Step *steps, byte count) {
  for (const Step *s = steps; s < steps + count; s++) {
    runStep(s);
  }
  // ประมวลผล LSRB (Left, Right, หรือ Back 180)
  executeLsrbTurn();
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

  stopMotors();

  attachInterrupt(digitalPinToInterrupt(motors[A].encPin), encoderA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(motors[B].encPin), encoderB_ISR, RISING);

  Serial.println(F("=== LSRB Maze Robot Starting in 2 seconds ==="));
  delay(2000);
}

// ---------- Main loop ----------
void loop() {
  runMission(MISSION, STEP_COUNT);
}