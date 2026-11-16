# Autonomous and Manual Competition Robots

This repository contains the programming work for our autonomous and manual competition robots, including autonomous maze navigation, cube delivery, and manual robot control.

## Project Objective

To achieve reliable autonomous maze navigation and cube delivery through ultrasonic obstacle detection, IR sensing for line detection and precise positioning, encoder-based PID control, and LSRB path planning, while maintaining a compact, stable, and modular chassis for accurate movement and efficient performance.

## Design Concept and Rationale

The autonomous robot is designed to combine environmental sensing, movement feedback, and systematic path planning. These features support accurate navigation and consistent cube delivery.

| Feature | Purpose and Design Rationale |
| --- | --- |
| Ultrasonic sensing | Detect nearby walls and obstacles to support navigation decisions and collision avoidance. |
| IR sensing | Detect floor lines and positioning marks to support alignment, stopping, and precise positioning. |
| Wheel encoders | Measure wheel rotation to estimate travel distance and provide feedback for movement control. |
| Encoder-based PID control | Adjust motor output using encoder feedback to reduce movement errors and improve driving consistency. |
| LSRB path planning | Apply a consistent Left → Straight → Right → Back priority when choosing an available route. |
| Compact chassis | Allow the robot to navigate confined maze spaces and turn efficiently. |
| Stable chassis | Support consistent sensor readings, controlled movement, and reliable cube handling. |
| Modular construction | Simplify assembly, maintenance, component replacement, and design adjustments. |

## Repository Structure

| File or Folder | Description |
| --- | --- |
| `README.md` | Project overview, design rationale, and programming requirements. |
| `competition/auto/auto.ino` | Autonomous robot Arduino source code. |
| `competition/manual/your_manual_code.mblock` | Manual robot mBlock project containing Python control logic. |

Both robot programs must be stored in the team leader’s public GitHub repository.

## Autonomous Robot

### Intended Operating Sequence

1. Initialize the sensors, motors, encoders, and control variables.
2. Read ultrasonic sensors to detect walls and obstacles.
3. Read IR sensors to detect lines and positioning references.
4. Use encoder feedback and PID control to regulate movement.
5. At each junction, select an available direction using LSRB priority:
   - **L — Left:** Turn left when the route is available.
   - **S — Straight:** Continue straight if left is unavailable.
   - **R — Right:** Turn right if left and straight are unavailable.
   - **B — Back:** Turn back when no forward route is available.
6. Continue navigating toward the cube delivery location.
7. Align the robot, complete cube delivery, and stop when the task is finished.

The final source code and report should describe the implemented delivery mechanism and completion conditions.

### Programming Requirements

The autonomous code must include:

- **Functions** to organize tasks such as sensing, movement, turning, and navigation.
- **Structures (`struct`)** to group related data.
- **Loops** to repeat sensing and control operations.
- **Arrays** to store multiple values or navigation decisions.
- **Pointers** to access or update data through memory references.

Hardcoded solutions are permitted by the competition specification.

## Manual Robot

The manual robot program provides operator-controlled movement and operation of the robot’s mechanisms.

### Programming Requirements

- All control logic must be written in **mBlock’s Python editor**.
- **Block-based programming is forbidden.**
- Save the project as a `.mblock` file inside `competition/manual/`.
- Explain the controls, motor assignments, and operating sequence in the report.

## Calibration and Testing

Before operation:

- Confirm that pin assignments match the physical wiring.
- Check motor directions and encoder counting directions.
- Calibrate ultrasonic and IR detection thresholds.
- Calibrate encoder targets for movement distances and turns.
- Tune PID gains using the assembled robot.
- Test navigation, positioning, and cube delivery.
- Verify all manual controls and mechanism responses.

## Report Requirements

The report must include a **flowchart and a detailed explanation of the code** for both robots.

Explain the program flow, sensor inputs, motor outputs, control decisions, and stopping conditions. For the autonomous robot, also describe encoder feedback, PID control, and LSRB navigation.

## Submission Requirements

- Store both robot programs in the **team leader’s GitHub repository**.
- Follow the required repository structure.
- Set the repository to **public** so teaching assistants can access and review it.
- Submit the code before **16 November at 23:59**, according to the supplied specification.
