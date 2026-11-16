/*
  LSRB MAZE ROBOT -- extension of ultra_drive_turn(1).txt
  Arduino Uno R3 / ATmega328P, L298N, two single-channel encoders,
  two HC-SR04 sensors, one downward-facing TCRT5000 module.

  READ BEFORE RUNNING
  ------------------
  1. This is a WALL-maze solver on an orthogonal, equal-cell grid.
     Place the wheel axle at the START CELL CENTER, facing into the maze.
     The side ultrasonic MUST face LEFT at 90 degrees, not diagonally.
     Cell centers must allow a full in-place rotation of the chassis.
  2. Calibrate CELL_TICKS and TURN_90_TICKS_* below on the actual floor.
     The supplied values are EXAMPLES, not measurements of your robot.
     Straight PID corrects wheel-count mismatch; it does not correct slip
     or absolute heading. There is no IMU in this circuit.
  3. GOAL = a UNIQUE dark floor patch. Ordinary black lines/parking marks
     must NOT trigger it. Check 'I' readings and GOAL_ACTIVE_LEVEL first.
     This sketch does not follow a line or operate a cube-lifting actuator.
  4. LSRB is the left-hand rule. Guaranteed completion / dead-end pruning
     assumes a connected tree maze (no loops), with a reachable goal.
     It is not a general shortest-path solver for mazes containing loops.
  5. Fixed pins only. NO automatic TRIG/ECHO swapping.

  PIN MAP (S = shield signal row, V = regulated 5 V, G = common ground)
  --------------------------------------------------------------------
  L298N ENA D5; IN1 D7; IN2 D8; ENB D6; IN3 D9; IN4 D10.
  Left encoder DO D2; right encoder DO D3. Encoder AO unused.
  FRONT HC-SR04: TRIG D13, ECHO A0.
  LEFT  HC-SR04: TRIG D11, ECHO D12.
  TCRT5000: DO A1; AO A2 (AO is diagnostic only).
  Remove ENA/ENB jumpers when controlling their PWM inputs.
  IMPORTANT: diagram's yellow wires appear to bridge the two trigger
  signals. Separate them: front TRIG only D13, left TRIG only D11.
  No ECHO pin is ever configured as an output by this sketch.

  SERIAL MONITOR: 115200 baud; send one letter (newline optional)
  E = new exploration, from start cell / original heading.
  R = replay saved solution, after MANUALLY replacing robot at start.
  P = print reduced path; I = stationary sensor diagnostics.
  F = drive one cell (calibration); L / T = turn left / right 90 degrees.
  X = stop immediately when processed; H = help.
  During a run, only X is accepted; other commands are discarded.
  X is a software stop, not a replacement for the battery switch.

  Exploration saves its simplified path to EEPROM ONLY at the goal.
  Reset/power cycle retains the last complete path. E begins a new RAM
  path but leaves the previously saved EEPROM path until a new success.
  To reload the previous EEPROM solution after an interrupted E, reset.
  Calibration changes or a changed maze require a fresh exploration.

  HOW IT WORKS
  ------------
  At each cell center: read left and front; rotate right, read front to
  measure the former right direction, and rotate left to restore heading.
  Choose L, else S, else R, else B. Move exactly one calibrated cell.
  A straight-only cell is NOT recorded as a junction. Corners, branches,
  crossroads and dead ends ARE recorded. This distinction is essential
  for xBx pruning across corridors that contain several straight cells.
  Reduce xBx using net angle, e.g. LBL->S, LBS->R, LBR->B, SBL->R.
  During replay, skip straight-only cells and consume one saved token at
  every recorded type of node. Stop only when the goal marker is detected;
  an exhausted path is NOT automatically interpreted as the goal.

  Based on your Motor / Step / Pid structures, interrupt counting,
  pointer-based motor helpers, PWM ramp and wheel synchronization.
  Original timed forward/back mission replaced by grid navigation.
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <math.h>
#include <util/atomic.h>

// ---------- Original data structures ----------
struct Motor {
  byte enPin;
  byte in1Pin;
  byte in2Pin;
  byte encPin;
  volatile unsigned long ticks;
  volatile unsigned long lastPulseUs;
};

struct Step {
  bool forward;
  int basePwm;
  int trim;
  int scaleA;
  float kp;
  float ki;
  float kd;
  unsigned long maxMs;
  unsigned long targetTicks;
  unsigned int pauseMs;
  float stopDistanceCm;
};

struct Pid {
  float kp, ki, kd, integral;
  long lastError;
};

struct Openings {
  bool left, straight, right;
};

enum RobotState : byte { READY, EXPLORING, REPLAYING, FINISHED, FAULT };
enum MoveResult : byte { MOVE_OK, MOVE_GOAL, MOVE_FAILED };

// ---------- Fixed hardware ----------
const byte MOTOR_COUNT = 2;
const byte A = 0;
const byte B = 1;
Motor motors[MOTOR_COUNT] = {
  {5, 7, 8, 2, 0, 0},
  {6, 9, 10, 3, 0, 0}
};
const bool MOTOR_REVERSED[MOTOR_COUNT] = {false, false};
const byte FRONT_TRIG_PIN = 13;
const byte FRONT_ECHO_PIN = A0;
const byte SIDE_TRIG_PIN = 11;
const byte SIDE_ECHO_PIN = 12;
const byte IR_DIGITAL_PIN = A1;
const byte IR_ANALOG_PIN = A2;

// ---------- CALIBRATE THESE BEFORE EXPLORING ----------
// Count RISING edges for exactly one cell-center to cell-center movement.
const unsigned long CELL_TICKS = 360;  // EXAMPLE ONLY: replace with measurement.
// Each wheel's ticks for one in-place 90-degree turn. Separate left/right
// settings allow calibration of asymmetric motor/gearbox behavior.
const unsigned long TURN_90_TICKS_LEFT[MOTOR_COUNT]  = {80, 80};
const unsigned long TURN_90_TICKS_RIGHT[MOTOR_COUNT] = {80, 80};
const int TURN_PWM = 115;
const int TURN_MIN_PWM = 75;
const unsigned long TURN_SLOW_TICKS = 20;

// At a cell center measure the distance to a closed wall and the distance
// into an open neighboring cell. Put these thresholds between the two.
// Thresholds refer to sensor-to-wall distance, NOT wall height.
const float FRONT_OPEN_CM = 18.0f;
const float SIDE_OPEN_CM = 18.0f;
const float OPEN_HYSTERESIS_CM = 1.5f;
const float EMERGENCY_FRONT_CM = 5.0f; // allow stopping distance + nose offset.

const bool USE_IR_GOAL = true;
const byte GOAL_ACTIVE_LEVEL = LOW;   // verify DO on your actual goal patch.
const unsigned long GOAL_HOLD_MS = 80;
const unsigned long GOAL_ARM_MS = 1000; // ignore marker during departure.

const Step CELL_STEP = {
  true, 115, 0, 1035,     // forward, PWM, trim, scaleA (from original code)
  8.0f, 4.0f, 0.0f,     // kp, ki, kd: starting gains, must tune
  6000, CELL_TICKS, 100, EMERGENCY_FRONT_CM
};

const int START_PWM = 70;
const unsigned long RAMP_MS = 200;
const unsigned long CONTROL_MS = 20;
const int MAX_CORRECTION = 40;
const float I_LIMIT = 30.0f;
const unsigned long MIN_PULSE_US = 200;
const unsigned long STALL_MS = 900;
const unsigned long TURN_TIMEOUT_MS = 3500;
const unsigned int BRAKE_MS = 40;
const unsigned int TURN_PAUSE_MS = 120;
const unsigned long PING_GAP_MS = 65; // separates ALL ultrasonic triggers.
const unsigned long ECHO_TIMEOUT_US = 18000;
const byte MAX_INVALID_FRONT_READS = 3;
const uint16_t MAX_PATH = 200;
const unsigned long MAX_CELL_MOVES = 2000; // bounds loops / unreachable goal.

// ---------- State (fixed arrays; no String / heap allocation) ----------
RobotState state = READY;
Pid pid = {0, 0, 0, 0, 0};
char path[MAX_PATH];
uint16_t pathLength = 0;
uint16_t replayIndex = 0;
bool pathComplete = false;
unsigned long cellMoves = 0;
unsigned long runStartedMs = 0;
bool goalTiming = false;
unsigned long goalSinceMs = 0;
unsigned long lastPingMs = 0;
bool havePinged = false;

// Explicit declarations keep Arduino's generated prototypes unambiguous.
bool serviceStop();
bool waitWithStop(unsigned long durationMs);
void stopMotors();
void brakeMotors();
void fail(const __FlashStringHelper *message);
bool turnInPlace(bool left);
MoveResult runStep(const Step *s, bool allowGoal);

// ---------- Encoders: interrupt-only writes, atomic snapshots ----------
void countTick(Motor *m) {
  unsigned long now = micros();
  if (now - m->lastPulseUs >= MIN_PULSE_US) {
    ++m->ticks;
    m->lastPulseUs = now;
  }
}
void encoderA_ISR() { countTick(&motors[A]); }
void encoderB_ISR() { countTick(&motors[B]); }
void resetTicks() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for (byte i = 0; i < MOTOR_COUNT; ++i) motors[i].ticks = 0;
  }
}
void readTicks(unsigned long *out) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for (byte i = 0; i < MOTOR_COUNT; ++i) out[i] = motors[i].ticks;
  }
}

// ---------- Motors / stop handling ----------
void setMotor(const Motor *m, bool forward, int pwm) {
  byte index = (m == &motors[A]) ? A : B;
  bool direction = forward != MOTOR_REVERSED[index];
  digitalWrite(m->in1Pin, direction ? HIGH : LOW);
  digitalWrite(m->in2Pin, direction ? LOW : HIGH);
  analogWrite(m->enPin, constrain(pwm, 0, 255));
}
void stopMotors() {
  for (byte i = 0; i < MOTOR_COUNT; ++i) {
    analogWrite(motors[i].enPin, 0);
    digitalWrite(motors[i].in1Pin, LOW);
    digitalWrite(motors[i].in2Pin, LOW);
  }
}
void brakeMotor(byte i) {
  digitalWrite(motors[i].in1Pin, LOW);
  digitalWrite(motors[i].in2Pin, LOW);
  analogWrite(motors[i].enPin, 255);
}
void brakeMotors() {
  for (byte i = 0; i < MOTOR_COUNT; ++i) brakeMotor(i);
}
void fail(const __FlashStringHelper *message) {
  stopMotors();
  state = FAULT;
  goalTiming = false;
  Serial.print(F("STOP: "));
  Serial.println(message);
}
bool serviceStop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'X' || c == 'x') fail(F("User stop. Reposition before restarting."));
  }
  return state != FAULT;
}
bool waitWithStop(unsigned long durationMs) {
  unsigned long started = millis();
  while (millis() - started < durationMs) {
    if (!serviceStop()) return false;
    delay(1);
  }
  return state != FAULT;
}
bool finishMotion(unsigned int pauseMs) {
  brakeMotors();
  if (!waitWithStop(BRAKE_MS)) return false;
  stopMotors();
  return waitWithStop(pauseMs);
}

// ---------- Ultrasonic: fixed pin directions; no echo means UNKNOWN ----------
float readOneUltrasonic(byte trig, byte echo) {
  while (havePinged && millis() - lastPingMs < PING_GAP_MS) {
    if (!serviceStop()) return -1.0f;
    delay(1);
  }
  if (!serviceStop()) return -1.0f;
  lastPingMs = millis();
  havePinged = true;
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  unsigned long duration = pulseIn(echo, HIGH, ECHO_TIMEOUT_US);
  if (!serviceStop()) return -1.0f;
  if (duration == 0) return -1.0f;
  float cm = duration * 0.0343f * 0.5f;
  // Very short valid echoes remain obstacles, not open space.
  return cm;
}
float getFrontDistanceCm() {
  return readOneUltrasonic(FRONT_TRIG_PIN, FRONT_ECHO_PIN);
}
float getSideDistanceCm() {
  return readOneUltrasonic(SIDE_TRIG_PIN, SIDE_ECHO_PIN);
}

// Two agreeing valid votes out of up to five, while stationary.
// Readings in the hysteresis band and missing echoes cast no vote.
bool classifyOpening(byte trig, byte echo, float threshold, bool *isOpen) {
  byte openVotes = 0, closedVotes = 0;
  for (byte i = 0; i < 5; ++i) {
    float d = readOneUltrasonic(trig, echo);
    if (state == FAULT) return false;
    if (d < 0.0f) continue;
    if (d > threshold + OPEN_HYSTERESIS_CM) ++openVotes;
    else if (d < threshold - OPEN_HYSTERESIS_CM) ++closedVotes;
    if (openVotes >= 2) { *isOpen = true; return true; }
    if (closedVotes >= 2) { *isOpen = false; return true; }
  }
  fail(F("No reliable wall reading. Check wiring, aim and thresholds."));
  return false;
}

// ---------- IR finish marker ----------
bool goalDetected() {
  if (!USE_IR_GOAL || millis() - runStartedMs < GOAL_ARM_MS) {
    goalTiming = false;
    return false;
  }
  if (digitalRead(IR_DIGITAL_PIN) != GOAL_ACTIVE_LEVEL) {
    goalTiming = false;
    return false;
  }
  if (!goalTiming) { goalTiming = true; goalSinceMs = millis(); }
  return millis() - goalSinceMs >= GOAL_HOLD_MS;
}
bool goalAtCellCenter() {
  // Reset after turns: a sweep over an ordinary line must not accumulate
  // with a later stationary reading. Marker must still be unique.
  goalTiming = false;
  unsigned long started = millis();
  while (millis() - started <= GOAL_HOLD_MS + 10) {
    if (!serviceStop()) return false;
    if (goalDetected()) return true;
    delay(1);
  }
  return false;
}

// ---------- Wheel-count PID, retained from original ----------
void pidBegin(Pid *p, const Step *s) {
  p->kp = s->kp; p->ki = s->ki; p->kd = s->kd;
  p->integral = 0.0f; p->lastError = 0;
}
float pidUpdate(Pid *p, long error, float dt) {
  if (dt <= 0.0f) return 0.0f;
  p->integral += error * dt;
  if (p->ki > 0.0f) {
    float limit = I_LIMIT / p->ki;
    p->integral = constrain(p->integral, -limit, limit);
  } else p->integral = 0.0f;
  float derivative = (error - p->lastError) / dt;
  p->lastError = error;
  float output = p->kp * error + p->ki * p->integral + p->kd * derivative;
  return constrain(output, (float)-MAX_CORRECTION, (float)MAX_CORRECTION);
}

MoveResult runStep(const Step *s, bool allowGoal) {
  resetTicks();
  pidBegin(&pid, s);
  unsigned long ticks[MOTOR_COUNT], previous[MOTOR_COUNT] = {0, 0};
  unsigned long lastProgress[MOTOR_COUNT] = {millis(), millis()};
  unsigned long started = millis(), lastControl = started;
  unsigned long lastFrontCheck = started;
  byte invalidFront = 0;

  while (true) {
    if (!serviceStop()) return MOVE_FAILED;
    if (allowGoal && goalDetected()) {
      return finishMotion(s->pauseMs) ? MOVE_GOAL : MOVE_FAILED;
    }
    unsigned long now = millis();
    if (now - started >= s->maxMs) {
      fail(F("Cell motion timed out; calibrate distance / PWM."));
      return MOVE_FAILED;
    }
    readTicks(ticks);
    // scaleA accounts for per-wheel rolling-distance/count differences.
    unsigned long weightedA = ticks[A] * (unsigned long)s->scaleA / 1000UL;
    if (s->targetTicks > 0 && (weightedA + ticks[B]) / 2 >= s->targetTicks) {
      return finishMotion(s->pauseMs) ? MOVE_OK : MOVE_FAILED;
    }
    for (byte i = 0; i < MOTOR_COUNT; ++i) {
      if (ticks[i] != previous[i]) {
        previous[i] = ticks[i]; lastProgress[i] = now;
      }
      if (now - lastProgress[i] > STALL_MS) {
        fail(F("Wheel stalled or encoder pulses missing."));
        return MOVE_FAILED;
      }
    }
    if (s->stopDistanceCm > 0 && now - lastFrontCheck >= PING_GAP_MS) {
      float d = getFrontDistanceCm();
      lastFrontCheck = millis();
      if (state == FAULT) return MOVE_FAILED;
      if (d < 0) {
        if (++invalidFront >= MAX_INVALID_FRONT_READS) {
          fail(F("Front echoes lost during travel.")); return MOVE_FAILED;
        }
      } else {
        invalidFront = 0;
        if (d <= s->stopDistanceCm) {
          // A partial cell must NEVER be mistaken for a new cell center.
          brakeMotors();
          waitWithStop(BRAKE_MS);
          fail(F("Obstacle before cell center. Check cell calibration / clearance."));
          return MOVE_FAILED;
        }
      }
    }
    now = millis();
    if (now - lastControl >= CONTROL_MS) {
      float dt = (now - lastControl) / 1000.0f;
      lastControl = now; // actual dt, including pulseIn delays.
      readTicks(ticks);
      long error = (long)(ticks[A] * (unsigned long)s->scaleA / 1000UL)
                   - (long)ticks[B];
      int correction = (int)lroundf(pidUpdate(&pid, error, dt));
      unsigned long elapsed = now - started;
      int base = s->basePwm;
      if (elapsed < RAMP_MS) {
        base = START_PWM + (long)(s->basePwm - START_PWM) * elapsed / RAMP_MS;
      }
      setMotor(&motors[A], s->forward,
               constrain(base - s->trim - correction, 0, 255));
      setMotor(&motors[B], s->forward,
               constrain(base + s->trim + correction, 0, 255));
    }
    delay(1);
  }
}

// ---------- Encoder-based turns; each wheel must reach its target ----------
bool turnInPlace(bool left) {
  stopMotors();
  if (!waitWithStop(50)) return false;
  resetTicks();
  const unsigned long *target = left ? TURN_90_TICKS_LEFT : TURN_90_TICKS_RIGHT;
  unsigned long ticks[MOTOR_COUNT], previous[MOTOR_COUNT] = {0, 0};
  unsigned long lastProgress[MOTOR_COUNT] = {millis(), millis()};
  unsigned long started = millis();
  while (true) {
    if (!serviceStop()) return false;
    readTicks(ticks);
    if (ticks[A] >= target[A] && ticks[B] >= target[B]) break;
    unsigned long now = millis();
    if (now - started > TURN_TIMEOUT_MS) {
      fail(F("Turn timed out.")); return false;
    }
    for (byte i = 0; i < MOTOR_COUNT; ++i) {
      if (ticks[i] >= target[i]) { brakeMotor(i); continue; }
      if (ticks[i] != previous[i]) {
        previous[i] = ticks[i]; lastProgress[i] = now;
      }
      if (now - lastProgress[i] > STALL_MS) {
        fail(F("Turn stalled / encoder missing.")); return false;
      }
      int pwm = target[i] - ticks[i] <= TURN_SLOW_TICKS ? TURN_MIN_PWM : TURN_PWM;
      // Left turn: A backward, B forward. Right turn: A forward, B backward.
      setMotor(&motors[i], (i == A) ? !left : left, pwm);
    }
    delay(1);
  }
  return finishMotion(TURN_PAUSE_MS);
}
bool executeTurn(char turn) {
  if (turn == 'S') return true;
  if (turn == 'L') return turnInPlace(true);
  if (turn == 'R') return turnInPlace(false);
  if (turn == 'B') return turnInPlace(false) && turnInPlace(false);
  fail(F("Invalid path token.")); return false;
}

bool scanOpenings(Openings *o) {
  if (!classifyOpening(SIDE_TRIG_PIN, SIDE_ECHO_PIN, SIDE_OPEN_CM, &o->left))
    return false;
  if (!classifyOpening(FRONT_TRIG_PIN, FRONT_ECHO_PIN, FRONT_OPEN_CM, &o->straight))
    return false;
  // No right sensor: temporarily turn right, measure, then restore heading.
  if (!turnInPlace(false)) return false;
  if (!classifyOpening(FRONT_TRIG_PIN, FRONT_ECHO_PIN, FRONT_OPEN_CM, &o->right))
    return false; // FAULT; do not move again automatically.
  if (!turnInPlace(true)) return false;
  Serial.print(F("Open L/S/R: "));
  Serial.print(o->left); Serial.print('/'); Serial.print(o->straight);
  Serial.print('/'); Serial.println(o->right);
  return true;
}

// ---------- LSRB and standard dead-end path simplification ----------
char selectTurn(const Openings *o) {
  if (o->left) return 'L';
  if (o->straight) return 'S';
  if (o->right) return 'R';
  return 'B';
}
bool isNode(const Openings *o) {
  return o->left || o->right || !o->straight;
}
byte turnQuarterUnits(char turn) {
  switch (turn) {
    case 'R': return 1;
    case 'B': return 2;
    case 'L': return 3;
    default: return 0;
  }
}
bool validToken(char turn) {
  return turn == 'L' || turn == 'S' || turn == 'R' || turn == 'B';
}
void simplifyPath() {
  const char turnByAngle[4] = {'S', 'R', 'B', 'L'};
  while (pathLength >= 3 && path[pathLength - 2] == 'B') {
    uint16_t first = pathLength - 3;
    byte angle = (turnQuarterUnits(path[first]) +
                  turnQuarterUnits(path[first + 1]) +
                  turnQuarterUnits(path[first + 2])) % 4;
    path[first] = turnByAngle[angle];
    pathLength -= 2;
  }
}
void printPath() {
  Serial.print(F("Reduced path [")); Serial.print(pathLength);
  Serial.print(F("]: "));
  for (uint16_t i = 0; i < pathLength; ++i) Serial.print(path[i]);
  Serial.println();
  Serial.println(pathComplete ? F("Complete solution.") : F("Partial / not learned yet."));
}
bool appendTurn(char turn) {
  if (pathLength >= MAX_PATH) {
    fail(F("Path buffer full.")); return false;
  }
  path[pathLength++] = turn;
  simplifyPath();
  printPath();
  return true;
}

// ---------- EEPROM: versioned, CRC checked, committed last ----------
// Addresses 0 magic; 1 format; 2..3 length; 4..5 CRC; 6.. path.
const byte EEPROM_MAGIC = 0xA7;
const byte EEPROM_FORMAT = 1;
const int EEPROM_PATH_BASE = 6;
uint16_t crcByte(uint16_t crc, byte value) {
  crc ^= (uint16_t)value << 8;
  for (byte i = 0; i < 8; ++i) {
    crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
  }
  return crc;
}
uint16_t pathCRC() {
  uint16_t crc = crcByte(0xFFFF, EEPROM_FORMAT);
  crc = crcByte(crc, (byte)pathLength);
  crc = crcByte(crc, (byte)(pathLength >> 8));
  for (uint16_t i = 0; i < pathLength; ++i) crc = crcByte(crc, (byte)path[i]);
  return crc;
}
bool savePath() {
  if (!pathComplete || EEPROM_PATH_BASE + pathLength > EEPROM.length()) return false;
  stopMotors();
  EEPROM.update(0, 0); // interrupted write cannot be loaded as complete.
  EEPROM.update(1, EEPROM_FORMAT);
  EEPROM.update(2, (byte)pathLength);
  EEPROM.update(3, (byte)(pathLength >> 8));
  uint16_t crc = pathCRC();
  EEPROM.update(4, (byte)crc);
  EEPROM.update(5, (byte)(crc >> 8));
  for (uint16_t i = 0; i < pathLength; ++i) EEPROM.update(EEPROM_PATH_BASE + i, path[i]);
  EEPROM.update(0, EEPROM_MAGIC);
  return true;
}
bool loadPath() {
  pathComplete = false; pathLength = 0;
  if (EEPROM.read(0) != EEPROM_MAGIC || EEPROM.read(1) != EEPROM_FORMAT) return false;
  uint16_t length = EEPROM.read(2) | ((uint16_t)EEPROM.read(3) << 8);
  if (length > MAX_PATH || EEPROM_PATH_BASE + length > EEPROM.length()) return false;
  for (uint16_t i = 0; i < length; ++i) {
    char token = EEPROM.read(EEPROM_PATH_BASE + i);
    if (!validToken(token)) return false;
    path[i] = token;
  }
  pathLength = length;
  uint16_t stored = EEPROM.read(4) | ((uint16_t)EEPROM.read(5) << 8);
  if (pathCRC() != stored) { pathLength = 0; return false; }
  pathComplete = true;
  return true;
}

// ---------- Exploration / replay ----------
void finishAtGoal() {
  stopMotors();
  if (state == REPLAYING && replayIndex != pathLength) {
    fail(F("Goal seen before all route tokens. Check goal / node sensing."));
    return;
  }
  bool wasExploring = state == EXPLORING;
  state = FINISHED;
  pathComplete = true;
  Serial.println(F("GOAL reached. Motors stopped."));
  if (wasExploring) {
    Serial.println(savePath() ? F("Solution saved in EEPROM.") : F("EEPROM save failed."));
  }
  printPath();
  Serial.println(F("To replay: put robot at ORIGINAL start + heading, then send R."));
}
void navigateOneCell() {
  if (goalAtCellCenter()) { finishAtGoal(); return; }
  if (state == FAULT) return;
  if (cellMoves >= MAX_CELL_MOVES) {
    fail(F("Move limit reached. Goal absent, maze loop, or sensing error.")); return;
  }
  Openings openings;
  if (!scanOpenings(&openings)) return;
  bool node = isNode(&openings);
  char decision = 'S';
  if (state == EXPLORING) {
    decision = selectTurn(&openings);
    if (node && !appendTurn(decision)) return;
  } else if (node) {
    if (replayIndex >= pathLength) {
      fail(F("Unexpected junction after end of saved route.")); return;
    }
    decision = path[replayIndex++];
    if ((decision == 'L' && !openings.left) ||
        (decision == 'S' && !openings.straight) ||
        (decision == 'R' && !openings.right)) {
      fail(F("Saved exit is blocked. Check start pose / maze / sensors.")); return;
    }
  }
  Serial.print(F("Decision: ")); Serial.println(decision);
  if (!executeTurn(decision)) return;
  // Recheck after the real turn (also verifies back is open for B).
  bool ahead;
  if (!classifyOpening(FRONT_TRIG_PIN, FRONT_ECHO_PIN, FRONT_OPEN_CM, &ahead)) return;
  if (!ahead) { fail(F("Chosen passage blocked after turn.")); return; }
  goalTiming = false;
  MoveResult result = runStep(&CELL_STEP, true);
  if (result == MOVE_FAILED) return;
  ++cellMoves;
  if (result == MOVE_GOAL) finishAtGoal();
}
void beginRun(bool replay) {
  stopMotors();
  if (replay && !pathComplete) {
    Serial.println(F("No complete solution in RAM. Explore E or reset to load EEPROM."));
    return;
  }
  if (USE_IR_GOAL && digitalRead(IR_DIGITAL_PIN) == GOAL_ACTIVE_LEVEL) {
    Serial.println(F("Start is on GOAL level. Check IR polarity / move off marker."));
    return;
  }
  if (!replay) { pathLength = 0; pathComplete = false; }
  replayIndex = 0; cellMoves = 0; goalTiming = false;
  state = replay ? REPLAYING : EXPLORING;
  Serial.println(replay ? F("REPLAY in 2 seconds.") : F("LSRB EXPLORATION in 2 seconds."));
  if (!waitWithStop(2000)) return;
  runStartedMs = millis();
}
void printHelp() {
  Serial.println(F("115200 baud: E explore | R replay | P path | I sensors"));
  Serial.println(F("Calibration: F one cell | L left 90 | T right 90 | X stop | H help"));
  Serial.println(F("Always reposition at start center + original heading before E/R."));
}
void sensorDiagnostics() {
  state = READY;
  Serial.print(F("Front cm (-1 unknown): ")); Serial.println(getFrontDistanceCm());
  Serial.print(F("Left cm (-1 unknown): ")); Serial.println(getSideDistanceCm());
  Serial.print(F("IR DO: ")); Serial.print(digitalRead(IR_DIGITAL_PIN));
  Serial.print(F(" | AO: ")); Serial.println(analogRead(IR_ANALOG_PIN));
}

void setup() {
  Serial.begin(115200);
  for (byte i = 0; i < MOTOR_COUNT; ++i) {
    pinMode(motors[i].enPin, OUTPUT);
    pinMode(motors[i].in1Pin, OUTPUT);
    pinMode(motors[i].in2Pin, OUTPUT);
    pinMode(motors[i].encPin, INPUT_PULLUP);
  }
  stopMotors();
  pinMode(FRONT_TRIG_PIN, OUTPUT); digitalWrite(FRONT_TRIG_PIN, LOW);
  pinMode(SIDE_TRIG_PIN, OUTPUT); digitalWrite(SIDE_TRIG_PIN, LOW);
  pinMode(FRONT_ECHO_PIN, INPUT);
  pinMode(SIDE_ECHO_PIN, INPUT);
  pinMode(IR_DIGITAL_PIN, INPUT_PULLUP);
  pinMode(IR_ANALOG_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(motors[A].encPin), encoderA_ISR, RISING);
  attachInterrupt(digitalPinToInterrupt(motors[B].encPin), encoderB_ISR, RISING);
  Serial.println(F("=== LSRB Maze Robot: stationary until command ==="));
  Serial.println(loadPath() ? F("EEPROM solution loaded.") : F("No valid EEPROM solution."));
  printHelp();
}
void loop() {
  if (state == EXPLORING || state == REPLAYING) {
    navigateOneCell();
    return;
  }
  stopMotors();
  if (!Serial.available()) return;
  char command = Serial.read();
  if (command >= 'a' && command <= 'z') command -= 'a' - 'A';
  switch (command) {
    case 'E': beginRun(false); break;
    case 'R': beginRun(true); break;
    case 'P': printPath(); break;
    case 'I': sensorDiagnostics(); break;
    case 'H': printHelp(); break;
    case 'X': fail(F("User stop.")); break;
    case 'F':
    case 'L':
    case 'T': {
      state = READY;
      Serial.println(F("Calibration motion in 2 seconds; X stops."));
      if (!waitWithStop(2000)) break;
      if (command == 'F') runStep(&CELL_STEP, false);
      else turnInPlace(command == 'L');
      unsigned long ticks[MOTOR_COUNT]; readTicks(ticks);
      Serial.print(F("Final raw ticks A/B: ")); Serial.print(ticks[A]);
      Serial.print('/'); Serial.println(ticks[B]);
      break;
    }
    default: break; // ignores CR / LF / unsupported commands.
  }
}
