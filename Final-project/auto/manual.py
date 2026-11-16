import cyberpi
import mbot2
import gamepad
import time

# ==========================================================
# 1. STRUCTURE (Class to store manual robot state and parameters)
# ==========================================================
class ManualRobotState:
    def __init__(self):
        # Arrays (Lists) storing motor and servo ports for each mechanism
        self.intake_ports   = ["M1", "M2"]
        self.gripper_ports  = ["s1", "s2"]
        self.catapult_ports = ["s3", "s4"]

        # Speed settings for 4-Wheel Drivetrain and Flex-Wheel Intake
        self.normal_speed   = 85     # Normal driving speed (%)
        self.slow_speed     = 40     # Precision slow mode speed for cube/flag placement (%)
        self.intake_speed   = 100    # Ball intake DC motor speed (%)

        # Angle settings for Geared Dual-Servo Gripper (S1 and S2 rotate in opposite directions)
        self.grip_open_angle  = 40   # Angle when gripper is open
        self.grip_close_angle = 115  # Angle when gripper is closed to hold cube/flag
        self.current_grip     = 40   # Current gripper angle

        # Angle settings for Catapult Launcher (Digital Servo on S3/S4)
        self.catapult_ready_angle = 20   # Cocked / ready-to-fire angle
        self.catapult_fire_angle  = 130  # Launch angle to shoot balls

        # Control states
        self.intake_toggle = False
        self.last_n1_state = False

# Create main robot state instance
robot = ManualRobotState()

# ==========================================================
# 2. FUNCTIONS (Modular functions for each of the 4 mechanisms)
# ==========================================================

def clamp_val(val, min_v, max_v):
    if val < min_v:
        return min_v
    if val > max_v:
        return max_v
    return val

# --- Mechanism 1: 4-Wheel Drivetrain (2x Encoder Motors EM1, EM2) ---
def update_drivetrain(state):
    # Hold R2 for slow precision mode, otherwise use normal driving speed
    max_pwr = state.slow_speed if gamepad.is_key_pressed("R2") else state.normal_speed

    # Read Left Joystick (Ly = forward/backward, Lx = turn left/right)
    ly = gamepad.get_joystick("Ly")
    lx = gamepad.get_joystick("Lx")

    # Deadzone filter to prevent joystick drift
    if abs(ly) < 10:
        ly = 0
    if abs(lx) < 10:
        lx = 0

    # If joystick is centered, allow D-Pad buttons for fine alignment
    if ly == 0 and lx == 0:
        if gamepad.is_key_pressed("Up"):
            ly = 50
        elif gamepad.is_key_pressed("Down"):
            ly = -50
        elif gamepad.is_key_pressed("Left"):
            lx = -45
        elif gamepad.is_key_pressed("Right"):
            lx = 45

    # Arcade drive mixing for Left (EM1) and Right (EM2) encoder motors
    left_power  = (ly + lx) * (max_pwr / 100.0)
    right_power = (ly - lx) * (max_pwr / 100.0)

    left_power  = clamp_val(int(left_power), -100, 100)
    right_power = clamp_val(int(right_power), -100, 100)

    if left_power == 0 and right_power == 0:
        mbot2.EM_stop("all")
    else:
        # Left and right motors are mounted opposite to each other, so right motor is inverted
        mbot2.drive_power(left_power, -right_power)

# --- Mechanism 2: 2-Layer Flex-Wheel Intake (Orange DC Motor 170 RPM) ---
def set_intake_motors(state, power):
    # Loop through intake motor ports array (M1 and M2)
    for port in state.intake_ports:
        mbot2.motor_set(power, port)

def update_intake(state):
    # Button N1 (Button 1): Toggle continuous ball intake ON/OFF
    n1_pressed = gamepad.is_key_pressed("N1")
    if n1_pressed and not state.last_n1_state:
        state.intake_toggle = not state.intake_toggle
    state.last_n1_state = n1_pressed

    # L1 = Intake IN (capture balls), L2 = Intake OUT (eject/unjam), or use N1 toggle state
    if gamepad.is_key_pressed("L1"):
        set_intake_motors(state, state.intake_speed)
    elif gamepad.is_key_pressed("L2"):
        set_intake_motors(state, -state.intake_speed)
    elif state.intake_toggle:
        set_intake_motors(state, state.intake_speed)
    else:
        set_intake_motors(state, 0)

# --- Mechanism 3: Geared Dual-Servo Gripper (2x MS-1.5A Servos on S1, S2) ---
def set_dual_gripper(state, angle):
    angle = clamp_val(int(angle), 10, 170)
    state.current_grip = angle

    # Geared dual-servo jaws move in opposite directions:
    # Servo 1 (s1) moves to 'angle', Servo 2 (s2) moves to '180 - angle'
    mbot2.servo_set(angle, state.gripper_ports[0])
    mbot2.servo_set(180 - angle, state.gripper_ports[1])

def update_gripper(state):
    # Button N2 (Button 2) = Close gripper (grab), Button N3 (Button 3) = Open gripper (release)
    if gamepad.is_key_pressed("N2"):
        set_dual_gripper(state, state.grip_close_angle)
    elif gamepad.is_key_pressed("N3"):
        set_dual_gripper(state, state.grip_open_angle)
    else:
        # Use Right Joystick (Ry) for fine manual gripper angle adjustment
        ry = gamepad.get_joystick("Ry")
        if abs(ry) > 20:
            step = 2 if ry > 0 else -2
            set_dual_gripper(state, state.current_grip + step)

# --- Mechanism 4: Catapult Launcher (Digital Servo on S3/S4) ---
def set_catapult_servo(state, angle):
    for port in state.catapult_ports:
        mbot2.servo_set(angle, port)

def update_catapult(state):
    # Press R1 to launch ball and automatically reset catapult to ready position
    if gamepad.is_key_pressed("R1"):
        mbot2.EM_stop("all") # Briefly stop wheels while firing for accurate ball trajectory
        set_catapult_servo(state, state.catapult_fire_angle)
        time.sleep(0.35)
        set_catapult_servo(state, state.catapult_ready_angle)
        time.sleep(0.20)

# --- Emergency Stop (Button N4 / Button 4) ---
def check_emergency_stop(state):
    if gamepad.is_key_pressed("N4"):
        state.intake_toggle = False
        mbot2.EM_stop("all")
        set_intake_motors(state, 0)
        set_catapult_servo(state, state.catapult_ready_angle)

# ==========================================================
# 3. INITIALIZATION & MAIN CONTROL LOOP
# ==========================================================
def init_robot(state):
    cyberpi.console.clear()
    cyberpi.console.println("Manual Robot Ready!")
    cyberpi.console.println("L-Stick: Drive 4WD")
    cyberpi.console.println("L1/L2/N1: Intake")
    cyberpi.console.println("R1: Fire Catapult")
    cyberpi.console.println("N2/N3: Dual Gripper")

    # Stop all motors and move servos to initial safe positions
    mbot2.EM_stop("all")
    set_intake_motors(state, 0)
    set_dual_gripper(state, state.grip_open_angle)
    set_catapult_servo(state, state.catapult_ready_angle)

# Initialize robot before entering main loop
init_robot(robot)

# Main control loop polling Bluetooth gamepad inputs continuously
while True:
    check_emergency_stop(robot)
    update_drivetrain(robot)
    update_intake(robot)
    update_gripper(robot)
    update_catapult(robot)
    time.sleep(0.02)