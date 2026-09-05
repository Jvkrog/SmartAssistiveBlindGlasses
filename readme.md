---

# Smart Assistive Glasses

> An ESP32-based assistive smart eyewear prototype for obstacle proximity awareness, predictive fall detection, haptic feedback, and remote emergency notification.

## Overview

Smart Assistive Glasses is a wearable safety system designed to improve environmental awareness and personal safety for visually impaired users.

The prototype combines **distance sensing, inertial sensing, haptic feedback, audible alerts, and wireless communication** inside a compact eyewear-oriented platform.

The current implementation focuses on two primary functions:

* **Real-time obstacle proximity detection**
* **Predictive and confirmed fall detection**

When a nearby obstacle is detected, the system increases vibration intensity as the obstacle approaches. The inertial subsystem continuously monitors acceleration and angular velocity to identify motion patterns associated with a possible fall. A pre-fall state is triggered by free-fall-like acceleration or a sudden rotational spike, followed by impact confirmation.

Critical events can be reported remotely through Telegram over Wi-Fi.

---

## Current Prototype

```text
                 ┌─────────────────┐
                 │      ESP32      │
                 │  Dual-Core MCU  │
                 └────────┬────────┘
                          │
             ┌────────────┼────────────┐
             │            │            │
             ▼            ▼            ▼
        TFmini LiDAR   MPU6050      Wi-Fi
        Distance       IMU           │
             │            │           ▼
             │            │       Telegram
             │            │
             └─────┬──────┘
                   │
                   ▼
             Decision Logic
                   │
          ┌────────┴─────────┐
          ▼                  ▼
    Vibration Motor        Buzzer
```

## Features

### Obstacle Proximity Feedback

The TFmini distance sensor continuously measures the distance to objects.

The current implementation:

* Reads the TFmini over UART
* Validates distance and signal strength
* Maintains the latest valid distance reading
* Provides continuous proximity feedback
* Increases vibration intensity as an obstacle approaches
* Generates a Telegram alert when an obstacle enters the critical range

Current vibration range:

```text
> 200 cm     → vibration OFF

200–20 cm    → progressively increasing vibration

< 25 cm      → critical obstacle alert
```

---

## Predictive Fall Detection

Unlike a simple impact detector, the prototype uses a small state machine:

```text
             NORMAL
                │
       free-fall / gyro spike
                │
                ▼
            PRE-FALL
                │
          ┌─────┴─────┐
          │           │
       impact       timeout
          │           │
          ▼           ▼
       CONFIRMED     NORMAL
          │
          ▼
       Alert + Buzzer
```

### Pre-Fall Detection

The system looks for:

* Reduced acceleration magnitude resembling free fall
* Sudden rotational movement
* Abnormal inertial behavior

The current implementation uses:

```text
Free-fall threshold  = 0.50 g
Gyroscope threshold  = 200 deg/s
Pre-fall window      = 800 ms
Impact threshold     = 2.10 g
```

The thresholds are configurable in the source code.

### Impact Confirmation

A pre-fall event does not immediately classify the event as a confirmed fall.

The system waits for an impact signature within the configured window.

This reduces false triggers caused by movements such as:

* rapid sitting
* sudden head movement
* jumping
* other transient motion

---

## Emergency Notification

When a fall is confirmed, the system creates a Telegram message containing:

* Fall detection status
* Pre-fall → impact confirmation
* Estimated predictive lead time
* Buzzer status

The message is placed into an asynchronous queue instead of being sent directly from the sensor loop.

---

## Asynchronous Communication Architecture

One important implementation detail is that HTTPS/Telegram communication can take significantly longer than the sensor sampling interval.

Instead of blocking the sensing loop:

```text
Sensor Loop
     │
     ├── Read distance
     ├── Read IMU
     ├── Fall detection
     └── Haptic control
              │
              ▼
        Telegram Queue
              │
              ▼
       Telegram Task
              │
              ▼
          Telegram
```

The Telegram task runs independently using the ESP32's FreeRTOS capabilities.

This allows the sensing/control loop to continue operating while network communication takes place.

### Queue Protection

The Telegram queue has a finite capacity.

When the queue is full, the oldest queued message is discarded before inserting the latest event.

This prevents communication congestion from blocking the safety-critical sensing loop.

---

## Wi-Fi Recovery

The system also performs background Wi-Fi reconnection.

If the connection is lost:

```text
Wi-Fi disconnected
       ↓
5-second retry interval
       ↓
WiFi reconnect attempt
       ↓
Connection restored
```

Sensor processing continues while reconnection attempts occur.

---

## Hardware

The current prototype uses:

| Component              | Purpose                                    |
| ---------------------- | ------------------------------------------ |
| ESP32                  | Main processing and wireless communication |
| TFmini distance sensor | Obstacle ranging                           |
| MPU6050                | Acceleration and gyroscope sensing         |
| Vibration motor        | Haptic proximity feedback                  |
| Piezo buzzer           | Fall/emergency audible alert               |
| LiPo battery           | Portable power                             |
| Wi-Fi                  | Remote communication                       |

The broader invention architecture described in the patent includes additional elements such as GPS, adaptive hazard classification, multi-channel emergency communication, community hazard mapping, and gesture recognition. These are **not necessarily represented in the current repository implementation**. 

---

## Software Architecture

The firmware is organized around a non-blocking real-time loop.

```text
setup()
 │
 ├── Initialize Serial
 ├── Initialize TFmini
 ├── Initialize MPU6050
 ├── Configure vibration/buzzer
 ├── Connect Wi-Fi
 ├── Start Telegram task
 └── Send startup notification
```

Runtime:

```text
loop()
 │
 ├── Maintain Wi-Fi
 │
 └── Every ~20 ms
       │
       ├── Read TFmini
       ├── Control vibration
       ├── Check fall state
       └── Handle alerts
```

The current sensor/control loop targets approximately **20 ms scheduling**, while Telegram communication is handled separately.

---

## Fall State Machine

The firmware implements three states:

```text
FALL_NORMAL
     │
     │ free-fall / gyro spike
     ▼
FALL_PRE
     │
     ├── impact detected ──────► FALL_CONFIRMED
     │
     └── timeout ──────────────► FALL_NORMAL
                                     
FALL_CONFIRMED
     │
     │ 5 seconds
     ▼
FALL_NORMAL
```

This separates:

1. Normal motion
2. Potential fall
3. Confirmed fall

rather than treating every sudden movement as a fall.

---

## Repository Structure

```text
.
├── smart_glasses_async.ino
└── README.md
```

The primary firmware is contained in:

```text
smart_glasses_async.ino
```

---

## Getting Started

### Requirements

* ESP32 development board
* Arduino IDE / compatible ESP32 environment
* MPU6050 library
* UniversalTelegramBot library
* ESP32 Wi-Fi support
* TFmini-compatible UART distance sensor

### Libraries

```cpp
#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
```

### Hardware Connections

The current firmware uses:

```text
MPU6050
SDA/SCL → ESP32 I²C

TFmini
TX/RX → ESP32 UART2

Vibration Motor
GPIO 25

Buzzer
GPIO 26
```

The exact wiring should be adapted to the ESP32 board and hardware revision being used.

---

## Configuration

Before flashing the firmware, configure:

```cpp
const char* ssid = "...";
const char* password = "...";

#define BOT_TOKEN "..."
#define CHAT_ID   "..."
```

For a public repository, **do not commit real Wi-Fi credentials, Telegram bot tokens, API keys, or other secrets.**

Use placeholders or move credentials to a separate configuration mechanism.

---

## Current Limitations

This repository represents a **working prototype**, not necessarily the complete implementation described in the associated patent specification.

Current firmware limitations include:

* TFmini-based ranging rather than the patent's specified solid-state LiDAR embodiment
* Single distance measurement path in the current code
* No full multi-sector obstacle classification
* No GPS implementation in the current firmware
* Telegram is currently the implemented remote alert channel
* No community hazard heatmap implementation
* No gesture-recognition implementation
* No machine-learning object classification
* Fall prediction is threshold/state-machine based rather than ML-based
* Current obstacle feedback is primarily distance-based

The patent specification describes a broader system architecture incorporating sensor fusion, hazard categorisation, adaptive feedback, predictive fall-risk processing, GPS-based emergency notification, community hazard mapping, and gesture-based interaction. 

---

## Development Roadmap

### Phase 1 — Current Prototype

* [x] ESP32 control
* [x] Distance sensing
* [x] MPU6050 motion sensing
* [x] Proximity vibration feedback
* [x] Buzzer alerts
* [x] Predictive pre-fall state
* [x] Impact confirmation
* [x] Wi-Fi connectivity
* [x] Telegram notification
* [x] Asynchronous Telegram task
* [x] Wi-Fi reconnection

### Phase 2 — Assistive Intelligence

* [ ] Multi-zone obstacle detection
* [ ] Hazard classification
* [ ] Direction-aware haptic feedback
* [ ] Adaptive vibration patterns
* [ ] Improved fall-risk classification
* [ ] GPS integration
* [ ] Multi-channel emergency communication

### Phase 3 — Intelligent Navigation

* [ ] Community hazard mapping
* [ ] Trust-weighted hazard reports
* [ ] Gesture recognition
* [ ] Context-aware command interpretation
* [ ] Mobile companion application
* [ ] Environmental/object recognition

The proposed later-stage functions correspond to extensions described in the patent specification. 

---

## Patent Context

This repository contains a **prototype implementation associated with a broader smart assistive eyewear invention**.

The patent specification describes an intelligent assistive eyewear system combining:

* distance sensing
* six-axis inertial sensing
* sensor fusion
* hazard classification
* adaptive haptic feedback
* predictive fall detection
* autonomous emergency communication
* GPS location
* community hazard mapping
* gesture recognition

The patent's claimed architecture is broader than the functionality currently implemented in this repository. 

Therefore, **the repository should be considered an implementation/prototyping component rather than a one-to-one representation of every patent claim.**

---

## Disclaimer

This project is an engineering prototype intended for research, development, and assistive-technology experimentation.

It should **not be considered a replacement for established mobility aids, professional assistance, or emergency systems** without appropriate validation and safety testing.

Fall detection and obstacle detection systems can produce false positives and false negatives. Real-world deployment requires extensive testing across different users, environments, lighting conditions, motion patterns, and hardware configurations.

---

## License

```text
Copyright © 2026
```

