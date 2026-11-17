# Group 11: Girly Gang
## Computer Programming — Final Project

Autonomous and manual competition robots developed to apply programming concepts to robot navigation, movement control, and object handling.


## Team Members

| Name | Section |
| --- | --- |
| Naphat Kaewpradit | SEC 1 |
| Punchita Teerachawansak | SEC 2 |
| Ranravee Thithinan | SEC 3 |

## Autonomous Robot

**Objective:** Reliable autonomous maze navigation and cube delivery using ultrasonic obstacle detection, IR line detection and positioning, encoder-based PID control, and LSRB path planning.

**Design rationale:**
- Ultrasonic sensors detect walls and obstacles.
- IR sensing identifies floor markings for positioning and parking.
- Encoder feedback and PID control improve movement consistency.
- LSRB uses Left → Straight → Right → Back as the intended navigation priority.
- A compact, stable, modular chassis supports accurate movement and easy maintenance.

**Programming:** Arduino C/C++. The source includes functions, structures, loops, arrays, and pointers.

## Manual Robot

**Objective:** One stable, modular robot for efficient ball grabbing, launching, and cube handling.

**Design rationale:**
- A stable chassis supports controlled driving and launching.
- Gripping and launching mechanisms support object handling and scoring.
- Modular construction simplifies adjustment and maintenance.

## Repository Files

| Path | Purpose |
| --- | --- |
| `competition/auto/auto.ino` | Autonomous Arduino program |
| `competition/manual/manual.mblock` | Required mBlock project for submission |
