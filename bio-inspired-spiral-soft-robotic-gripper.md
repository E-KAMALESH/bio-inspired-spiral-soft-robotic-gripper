---
publishDate: 2025-05-24T00:00:00Z
title: Bio-Inspired Spiral Soft Robotic Gripper
excerpt: A Fibonacci-spiral soft gripper powered by the MYOSA ESP32 kit that uses proximity and tilt sensing to gently curl around delicate objects — designed for borewell rescue and fragile-object handling.
image: bio-inspired-spiral-soft-robotic-gripper/myosa_demo_image.jpg
tags:
  - SoftRobotics
  - ESP32
  - IoT
  - Rescue
  - MYOSA
---

> A soft, intelligent gripper that wraps like an octopus arm — sensing, curling, and holding with zero harm.

---

## Acknowledgements

We thank **Dr. G. Suchitra**, our faculty mentor at Government College of Technology, Coimbatore, for her constant guidance. We also thank the **MYOSA Mini IoT Contest** organizers for providing the platform that made this project possible.

---

## Overview

India has seen over 400 borewell accidents in the last decade — many involving young children trapped in shafts less than 12 inches wide. Existing robotic rescue tools use rigid grippers that risk injuring the victim. Our project builds a **soft, bio-inspired spiral gripper** that can safely wrap around a child's arm or any fragile object without causing harm.

Inspired by the natural curl of an octopus arm and the mathematics of the **Fibonacci spiral**, our gripper distributes gripping force evenly across its segments. A cable running through the segments is pulled by a servo motor, causing the arm to curl inward. The **MYOSA ESP32 motherboard** ties it all together — reading sensors, running a state machine, and displaying live data on the OLED.

**Key features:**
* Proximity-triggered automatic gripping using the APDS9960 sensor
* Curvature-aware closed-loop control using the MPU6050 IMU
* 5-state control loop: IDLE → DETECTED → CURLING → HOLDING → RELEASING
* Real-time OLED display with servo position bar graph
* Serial monitor output for live debugging and data logging
* Wi-Fi remote monitoring capability via ESP32 (optional)

---

## Demo / Examples

### **Images**

<p align="center">
  <img src="/assets/images/bio-inspired-spiral-soft-robotic-gripper/myosa_bioinspired_gripper_cad.jpg" width="800"><br/>
  <i>CAD design of the Fibonacci spiral gripper arm</i>
</p>

<p align="center">
  <img src="/assets/images/bio-inspired-spiral-soft-robotic-gripper/myosa_bioinspired_gripper_cad1.jpg" width="800"><br/>
  <i>CAD side view showing segment hinge structure</i>
</p>

<p align="center">
  <img src="/assets/images/bio-inspired-spiral-soft-robotic-gripper/myosa_bioinspired_gripper_cad2.jpg" width="800"><br/>
  <i>3D printed gripper segments assembled</i>
</p>

<p align="center">
  <img src="/assets/images/bio-inspired-spiral-soft-robotic-gripper/myosa_demo_image.jpg" width="800"><br/>
  <i>Live demo with MYOSA board and gripper in action</i>
</p>

<p align="center">
  <img src="/assets/images/bio-inspired-spiral-soft-robotic-gripper/myosa_demo_dashboard.jpg" width="800"><br/>
  <i>Serial monitor / dashboard showing real-time sensor values</i>
</p>

### **Videos**

<video controls width="100%">
  <source src="/myosa_demo_video.mp4" type="video/mp4">
</video>

---

## Features (Detailed)

### **1. Proximity-Triggered Object Detection (APDS9960)**

The APDS9960 sensor is polled every 50 ms via raw I2C. When the proximity reading exceeds the detection threshold (`PROX_DETECT = 2`), the system transitions from IDLE to OBJECT_DETECTED. A 200 ms debounce confirmation prevents false triggers. This sensor is normally used for gesture control — here, we repurpose it as a **touchless object trigger** for the gripper.

### **2. Curvature Sensing with MPU6050**

The MPU6050 IMU is mounted at the base of the gripper arm. As the arm curls, its tilt angle increases. A **low-pass filter** (`alpha = 0.25`) smooths noisy raw readings:

```
filteredTilt = 0.25 * rawTilt + 0.75 * filteredTilt
```

Once the filtered tilt reaches `TARGET_ANGLE = 35.0°` (after a minimum curl time of 800 ms to avoid idle-noise false triggers), the servo stops and the gripper holds. This provides **closed-loop curvature control** without any external camera or vision system.

### **3. 5-State Gripper Control Machine**

The firmware runs a clean state machine with these states:

| State | What Happens |
|---|---|
| IDLE | Servo stays open (10°); waits for object |
| OBJECT_DETECTED | Debounce check; confirms object is real |
| CURLING | Servo increments 3°/step toward 120°; checks tilt |
| HOLDING | Servo locked at 120°; holds for 5 seconds |
| RELEASING | Servo decrements back to 10°; returns to IDLE |

### **4. OLED Live Dashboard**

The SSD1306 OLED updates every 300 ms and displays:
- Proximity value
- Filtered tilt angle (degrees)
- Servo position (degrees)
- Current state label
- A graphical servo position bar at the bottom

### **5. Startup Self-Test**

On power-up, the firmware runs a full servo sweep (10° → 120° → 10°) to verify mechanical integrity before entering the main loop. Sensor init failures are shown on the OLED and halted with error messages.

---

## Usage Instructions

**1. Power up the MYOSA board via USB-C.**

The OLED will show:
```
MYOSA Gripper
Starting...
```
Then run a servo sweep, and settle to:
```
System Ready!
Bring object near
```

**2. Bring an object within ~5–10 cm of the APDS9960 sensor.**

The OLED will update to show `State: DETECTED`, then `State: CURLING`.

**3. The gripper curls automatically.** It stops when either:
- The tilt angle reaches 35° (grip confirmed), or
- The servo reaches full close (120°), or
- 3 seconds have elapsed (timeout safety)

**4. After 5 seconds of holding, the gripper releases automatically.**

You can also monitor all values on the Serial Monitor at **115200 baud**:

```plaintext
State: CURLING | Prox: 4 | Tilt: 28.3 | Servo: 75
State: HOLDING | Prox: 4 | Tilt: 36.1 | Servo: 120
State: RELEASING | Prox: 1 | Tilt: 35.8 | Servo: 105
```

**Optional: Real-time plotting with Python**

```python
import serial
import matplotlib.pyplot as plt

ser = serial.Serial('COM3', 115200)  # Change port as needed
# Parse and plot Tilt and Servo values in real time
```

---

## Tech Stack

* **Microcontroller:** ESP32 (MYOSA Motherboard)
* **Firmware Language:** C++ (Arduino Framework)
* **Sensors:** APDS9960 (proximity/gesture), MPU6050 (tilt/curvature), SSD1306 OLED
* **Actuator:** SG90 / MG995 Servo Motor (cable actuation)
* **Libraries:** `AccelAndGyro`, `oled`, `ESP32Servo`, `Wire` (I2C)
* **Host Software:** Python + PySerial (real-time data logging)
* **Mechanical:** 3D-printed PLA segments, nylon cable, acrylic base
* **Power:** 5V/3A regulated supply for servo; USB-C for ESP32

---

## Requirements / Installation

**Arduino Libraries (install via Arduino IDE Library Manager):**

```bash
# Search and install these in Arduino IDE → Tools → Manage Libraries:
# - ESP32Servo
# - Adafruit SSD1306
# - AccelAndGyro (MYOSA custom library)
# - Wire (built-in)
```

**Board Setup:**
```bash
# In Arduino IDE → File → Preferences → Additional Board Manager URLs, add:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
# Then install: esp32 by Espressif Systems
```

**Python dependencies (for serial plotting):**
```bash
pip install pyserial matplotlib
```

**Upload Settings:**
- Board: `ESP32 Dev Module`
- Upload Speed: `115200`
- Flash Size: `4MB`

---

## File Structure

```
/bio-inspired-spiral-soft-robotic-gripper
├─ bio-inspired-spiral-soft-robotic-gripper.md   ← This file
├─ myosa_sourcecode.ino                           ← Main firmware
├─ Images/
│   ├─ myosa_bioinspired_gripper_cad.jpg
│   ├─ myosa_bioinspired_gripper_cad1.jpg
│   ├─ myosa_bioinspired_gripper_cad2.jpg
│   ├─ myosa_demo_image.jpg
│   └─ myosa_demo_dashboard.jpg
└─ Video/
    └─ myosa_demo_video.mp4
```

---

## License

This project is released under the **MIT License** for open-source use and academic purposes. All hardware designs and firmware are original work by the team.

---

## Contribution Notes

We welcome feedback and improvements! If you'd like to contribute:
- Open an **Issue** for bugs or suggestions
- Submit a **Pull Request** with your improvements
- Future work includes: force-sensitive resistor at tip, ML-based grip profiling, BMP180 depth estimation, and industrial pick-and-place scaling

**Team Members:**
- Kamalesh E — 3rd Year ECE, GCT Coimbatore
- Atchaya M — 2nd Year ECE, GCT Coimbatore
- Dhanuja A — 2nd Year ECE, GCT Coimbatore

**Faculty Mentor:** Dr. G. Suchitra, ECE Department, Government College of Technology, Coimbatore
