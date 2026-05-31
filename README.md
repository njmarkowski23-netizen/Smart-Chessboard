# Smart Chessboard

A wireless smart chessboard system that connects two physical chessboards over Wi-Fi.
When a player moves a piece on one board, the move is detected and automatically mirrored on the second board.

---

## Project Overview

This project was built as a senior design project at Farmingdale State College.
The goal was to combine the physical experience of traditional chess with the remote connectivity of online chess.

The system uses two ESP32 microcontrollers, reed switches, multiplexers, MQTT communication, stepper motors, and an electromagnet-based gantry system to detect and move chess pieces.

---

## Main Features

* Two physical chessboards connected wirelessly
* ESP32-based embedded control
* MQTT communication between boards
* Reed switch piece detection under each square
* Multiplexers used to reduce ESP32 input pin usage
* XY gantry system for automatic piece movement
* Stepper motors for accurate positioning
* Electromagnet used to move pieces from underneath the board
* Limit switches for homing and calibration
* LCD display for connection status, turns, and timers
* Buttons for turn control, reset, and time mode selection
* Graveyard area for captured pieces

---

## How It Works

1. A player moves a chess piece on one board.
2. Reed switches under the squares detect which piece was lifted and where it was placed.
3. The ESP32 reads the reed switch signals through multiplexers.
4. The move is sent to the other board using MQTT over Wi-Fi.
5. The receiving ESP32 calculates the matching move.
6. The gantry moves the electromagnet underneath the correct piece.
7. The electromagnet turns on, moves the piece, and releases it on the destination square.
8. Both physical boards stay synchronized.

---

## Hardware Used

* ESP32 microcontrollers
* Reed switches
* CD74HC4067 16-channel multiplexers
* NEMA 17 stepper motors
* Stepper motor drivers
* Electromagnet
* IRLZ44N MOSFET
* Buck converters
* 16x2 I2C LCD displays
* Limit switches
* Push buttons
* 12V power supplies
* Magnetic chess pieces
* Wooden chessboard enclosure
* 3D printed mounts and supports
* Aluminum rail gantry system

---

## Software Used

* Arduino IDE
* C/C++
* WiFi.h
* PubSubClient MQTT library
* LiquidCrystal_I2C library

---

## Code

The Arduino code is located in the `code/` folder.

The same main program can be used for both boards by changing this line:

```cpp
#define BOARD_LEFT 1
```

Use:

```cpp
#define BOARD_LEFT 1
```

for the left board.

Use:

```cpp
#define BOARD_LEFT 0
```

for the right board.

Before uploading the code, replace the Wi-Fi credentials with your own:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

Do not upload real Wi-Fi passwords to GitHub.

---

## Repository Structure

```text
Smart-Chessboard/
│
├── README.md
├── Final_Report.pdf
└── code/
    └── smart_chessboard.ino
```

---

## Main Subsystems

### Piece Detection

Each square has a reed switch underneath it.
Magnets inside the chess pieces activate the reed switches, allowing the ESP32 to detect piece locations.

### Wireless Communication

The two ESP32 boards communicate using MQTT.
Each board can publish and receive move data so the boards stay synchronized.

### Motion Control

The gantry system moves an electromagnet underneath the board.
Stepper motors control the X and Y movement so the magnet can reach each square.

### Electromagnet System

The electromagnet is controlled by the ESP32 through a MOSFET.
It turns on to grab a piece and turns off to release it.

### User Interface

Each board includes an LCD and three buttons.
The LCD displays connection status, turn information, and timer information.
The buttons control turn switching, reset, and game time selection.

---

## Challenges

* Wiring and testing a large number of reed switches
* Reading all board squares with limited ESP32 input pins
* Keeping jumper wires secure during testing
* Calibrating the gantry movement
* Tuning the electromagnet strength
* Preventing magnets inside the chess pieces from attracting each other
* Maintaining reliable Wi-Fi and MQTT communication

---

## Final Result

The final system successfully connected two physical chessboards over Wi-Fi.
The boards were able to detect moves, communicate wirelessly, move pieces using the gantry and electromagnet system, and display game information through the LCD interface.

---

## Team Members

* Ajay Dasrath
* Nicholas Markowski
* Jakub Kolakowski

---

## Project Status

Completed senior design project.
