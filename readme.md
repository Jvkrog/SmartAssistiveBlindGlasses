# Embedded Systems Portfolio

> Collection of embedded systems, IoT, and robotics projects built during my engineering journey.

This repository serves as an index of my embedded projects developed using ESP32, ESP8266, Arduino, sensors, wireless communication, robotics, and real-time embedded programming.

---

# Projects

## Smart Assistive Glasses

An ESP32-based assistive smart eyewear prototype for **obstacle proximity awareness, predictive fall detection, haptic feedback, audible alerts, and remote emergency notification**.

The implementation combines a TFmini distance sensor, MPU6050 IMU, ESP32, vibration feedback, buzzer output, Wi-Fi, and asynchronous Telegram communication.

### Features

- Real-time obstacle proximity detection
- TFmini UART distance sensing
- Progressive vibration feedback based on obstacle distance
- MPU6050 accelerometer + gyroscope monitoring
- Predictive pre-fall detection
- Impact-based fall confirmation
- Audible emergency indication
- Telegram emergency notifications over Wi-Fi
- Non-blocking asynchronous Telegram task
- Wi-Fi reconnection handling
- FreeRTOS-based task separation

### Repository

This repository contains the current prototype implementation:

`smart_glasses_async.ino`

> **Patent note:** The associated patent specification describes a broader smart assistive eyewear architecture including sensor fusion, multi-tier hazard classification, adaptive haptic feedback, GPS-assisted emergency communication, community hazard mapping, and gesture recognition. The code in this repository is a prototype and does not necessarily implement every element described or claimed in the patent specification. fileciteturn43file0

---

## Autonomous Rover

An autonomous robotics platform built around **ESP32**, **ESP32-CAM**, and **TFmini LiDAR**, capable of both manual and autonomous navigation through a browser-based dashboard.

### Features

- TFmini LiDAR-based obstacle detection
- ESP32-CAM live video streaming
- Browser dashboard for remote control
- Manual directional controls
- Mobile gyroscope steering
- Autonomous obstacle avoidance mode
- Real-time Wi-Fi operation

**Repository:**

https://github.com/Jvkrog/Autonomous-Rover

---

## Smart Wrist Band

A wearable safety and monitoring system designed around motion sensing and real-time emergency notifications.

### Features

- MPU6050 motion sensing
- Fall detection
- Emergency alert mechanism
- Blynk dashboard integration
- Telegram notifications
- Embedded real-time monitoring

**Repository:**

https://github.com/Jvkrog/Smart-Wrist-Band

---

## FireBot

An IoT-enabled autonomous fire detection and monitoring system focused on early hazard detection and remote alerting.

### Features

- Fire detection sensors
- Real-time monitoring
- Wireless notifications
- Embedded safety system
- IoT-based architecture

**Repository:**

https://github.com/Jvkrog/FireBot

---

## Esp32Cam-Rover (ESP32 Rover)

A browser-controlled robotic rover powered by ESP32 with wireless navigation and live video streaming capabilities.

### Features

- ESP32-CAM live video streaming
- Wi-Fi browser control
- L298N motor driver
- Real-time wireless navigation
- Embedded web interface

**Repository:**

https://github.com/Jvkrog/Esp32Cam-Rover

---

# Technologies

### Microcontrollers

- ESP32
- ESP32-CAM
- ESP8266
- Arduino Nano

### Sensors

- MPU6050
- TFmini LiDAR / distance sensing
- DHT11
- Flame sensors
- Ultrasonic sensors

### Communication

- Wi-Fi
- HTTP
- Telegram Bot API
- Blynk
- UART
- I²C

### Programming

- Arduino C/C++
- Embedded firmware
- Real-time programming
- FreeRTOS tasks
- Sensor integration
- Motor control
- Wireless communication

---

# Engineering Focus

These projects helped build practical experience in:

- Embedded firmware development
- Real-time embedded systems
- Sensor integration and fusion
- IoT system design
- Wireless communication
- Robotics and autonomous navigation
- Haptic and actuator control
- Remote monitoring and alerting
- Hardware-software integration
- Rapid prototyping

---

# Current Focus

My current engineering work has expanded from embedded systems into **backend engineering and autonomous trading infrastructure through TAlgo-X**.

These projects represent the embedded-systems foundation behind that progression, covering low-level hardware interaction, real-time sensing, wireless systems, robotics, and safety-oriented control logic.

---

# Author

**Vamshi Krishna**

GitHub: https://github.com/Jvkrog
