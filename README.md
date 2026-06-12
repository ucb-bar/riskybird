# RiskyBird

RiskyBird is an experimental quadcopter platform for hardware-software-mechanical co-design of tiny autonomous robots.

## Overview

This repository contains multiple iterations of RiskyBird hardware designs:

- **riskybirdv2_sensor/** - Sensor board with IMU, optical flow, and camera interface
- **riskybirdv2_esp/** - ESP32-based flight controller
- **riskybirdv2_k230d/** - K230D-based compute module
- **riskybirdv3_base/** - Latest iteration with integrated base board design
- **ecad/** & **ecad_2025/** - Electronic CAD files and libraries

## Design Goals

- Miniaturized form factor for agile flight
- Modular sensor and compute architecture
- On-board ML inference capability
- Rapid prototyping and iteration

## Hardware

The platform uses KiCad for PCB design and integrates:
- Low-cost sensors (IMU, optical flow, cameras)
- ESP32 for real-time control
- Optional AI accelerator (K230D) for perception
- Custom power distribution and motor control

## Educational Content

See [riskybird-academy/](riskybird-academy/) for tutorial materials covering embedded ML, robotics, and reinforcement learning.

## License

Hardware designs and educational materials are open source for non-commercial use.

Contact: vnikiforov@berkeley.edu
