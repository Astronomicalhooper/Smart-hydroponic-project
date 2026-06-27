# Smart-hydroponic-project
# Smart Fully Automated Vertical Hydroponic System
Author: Samuel Nathan Bobai
Contact: nathanbsamuel25@gmail.com
Platform: ESP32 | Language: C++ | License: MIT
================================================================

## Badges
[ESP32] [C++] [pH Sensor] [TDS Sensor] [DHT22] [Relay] [I2C LCD] [MIT License]

================================================================
## Project Overview
================================================================
An ESP32-based autonomous hydroponic controller that maintains
optimal growing conditions without manual intervention.

The system continuously monitors:
  - pH level (target 5.8 – 6.5)
  - Nutrient concentration / TDS (target 800 – 1200 ppm)
  - Ambient temperature and humidity (DHT22)
  - Reservoir water level (ultrasonic HC-SR04)

A Chain-of-Thought (CoT) decision engine triggers the correct
dosing pump or alerts the operator, logging every decision to
the serial console for full traceability.

================================================================
## Key Features
================================================================
  - Closed-loop pH regulation using pH-Up/Down peristaltic pumps
  - TDS nutrient management via dosing pump
  - DHT22 temperature and humidity monitoring
  - HC-SR04 ultrasonic water-level sensing with refill guard
  - Scheduled grow-light control (configurable on/off hours)
  - I2C LCD 16x2 live status display
  - Structured serial logging for every CoT decision step
  - Active-low relay compatibility (4-channel relay board)

================================================================
## System Architecture (Block Overview)
================================================================

  [Reservoir / Water Tank]
         |
         | (water + nutrients)
         v
  [Vertical Tower - 5 Grow Channels]
         |
         | (sensor readings)
         v
  +---------------------+
  |       ESP32         |  <-- Main controller
  |  - pH logic         |
  |  - TDS logic        |
  |  - Light schedule   |
  |  - LCD display      |
  +---------------------+
     |       |       |
     v       v       v
 [Relay Board - 4 Channel]
   |       |       |       |
   v       v       v       v
[pH Up] [pH Dn] [Nutr.] [Light]
 Pump    Pump    Pump   Relay

  [Sensors]
    - pH probe      --> GPIO 34
    - TDS probe     --> GPIO 35
    - DHT22         --> GPIO 4
    - HC-SR04 Trig  --> GPIO 26
    - HC-SR04 Echo  --> GPIO 27

================================================================
## Chain-of-Thought pH Control Flow
================================================================

  [Read pH Sensor]
         |
         v
  Is pH < 5.8? ---YES---> [Activate pH UP Pump A for 1s]
         |
        NO
         |
         v
  Is pH > 6.5? ---YES---> [Activate pH DOWN Pump B for 1s]
         |
        NO
         |
         v
  [pH in range 5.8-6.5 -- No action taken]

  Same CoT pattern applies to TDS (nutrient) regulation.

================================================================
## Wiring Reference
================================================================

  Component                  | ESP32 Pin | Notes
  ---------------------------|-----------|----------------------
  pH Sensor (analog out)     | GPIO 34   | Analog input
  HC-SR04 Trig               | GPIO 26   | Digital output
  HC-SR04 Echo               | GPIO 27   | Digital input
  TDS Sensor                 | GPIO 35   | Analog input
  DHT22 Data                 | GPIO 4    | OneWire digital
  Relay IN1 -- pH Up pump    | GPIO 18   | Active LOW
  Relay IN2 -- pH Down pump  | GPIO 19   | Active LOW
  Relay IN3 -- Nutrient pump | GPIO 21   | Active LOW
  Relay IN4 -- Grow light    | GPIO 22   | Active LOW
  I2C SDA (LCD)              | GPIO 21   | I2C data
  I2C SCL (LCD)              | GPIO 22   | I2C clock
  Sensor VCC                 | 3.3 V     | Power
  Relay / Pump VCC           | 5V/12V    | External PSU recommended

================================================================
## Quick Start
================================================================

  1. Install Arduino IDE and ESP32 board support:
       https://github.com/espressif/arduino-esp32

  2. Install libraries via Library Manager:
       - DHT sensor library (Adafruit)
       - LiquidCrystal_I2C (Frank de Brabander)

  3. Open src/hydroponic_main.ino in Arduino IDE.

  4. Select board: ESP32 Dev Module.
     Select the correct COM/USB port.

  5. Calibrate PH_OFFSET using pH 4.0 and 7.0 buffer solutions.
     Adjust the linear map constants in readPH() accordingly.

  6. Upload. Open Serial Monitor at 115200 baud to observe
     the Chain-of-Thought decision logs in real time.

================================================================
## Dependencies
================================================================

  Library              | Author            | Purpose
  ---------------------|-------------------|--------------------
  DHT sensor library   | Adafruit          | Temp & humidity
  LiquidCrystal_I2C    | Frank de Brabander| I2C LCD display
  Arduino ESP32 core   | Espressif         | Board support

================================================================
## Repository Structure
================================================================

  hydroponic/
  +-- src/
  |   +-- hydroponic_main.ino   # Main firmware
  +-- docs/
  |   +-- README.txt            # This file
  +-- README.md
  +-- LICENSE

================================================================
## License & Author
================================================================

  Samuel Nathan Bobai
  Computer Science undergraduate
  National Open University of Nigeria
  Embedded Systems Intern @ Nhub Incubators
  nathanbsamuel25@gmail.com

  Released under the MIT License.
  Copyright (c) 2026 Samuel Nathan Bobai
