# MERC Robot Project

This repository contains the control code for a robot built for the MERC robotics competition at Ho Chi Minh City University of Technology.

The project is based on an Arduino Mega 2560 and PlatformIO. It includes code for both manual control and autonomous line-following behavior.

## Key Features

- Manual driving with a PS2 controller
- Autonomous line following using QTR sensors and PID control
- DC motor control through BTS7960 drivers
- Lifting motor control through an L298N driver
- Servo control for robot mechanisms
- Optional LCD I2C display support

## Hardware

- Arduino Mega 2560
- QTR sensor array
- PS2 wireless controller module
- BTS7960 motor drivers
- L298N motor driver
- Servo motor
- LiquidCrystal I2C display

## Project Structure

```text
.
├── include/        # Header files
├── lib/            # Local libraries
├── src/            # Main robot firmware
├── test/           # PlatformIO test folder
└── platformio.ini  # PlatformIO configuration
```

## Build and Upload

Install PlatformIO, then build the project:

```bash
pio run
```

Upload the firmware to the Arduino Mega 2560:

```bash
pio run --target upload
```

Open the serial monitor:

```bash
pio device monitor
```
