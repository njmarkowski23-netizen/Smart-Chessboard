# Smart-Chessboard
Smart Chessboard


Overview

The Smart Chessboard is a wireless physical chess system designed to combine the traditional feel of over-the-board chess with the connectivity of online chess. The project uses two physical chessboards that communicate wirelessly so that a move made on one board is automatically mirrored on the other board.

Each board uses an ESP32 microcontroller, reed switch piece detection, MQTT communication, an XY gantry system, stepper motors, and an electromagnet to detect and replicate chess moves.

Project Purpose

Online chess allows players to compete remotely, but it removes the physical interaction of moving pieces on a real board. This project was designed to bring back the physical experience while still allowing two players to play from separate locations.

Features
Two physical chessboards connected wirelessly
ESP32 microcontrollers for embedded control
MQTT communication for board-to-board move syncing
Reed switches under each square for piece detection
16-channel multiplexers to reduce ESP32 input pin usage
CoreXY-style gantry system for movement
Stepper motors for precise XY positioning
Electromagnet for moving chess pieces
Limit switches for homing and calibration
LCD display for connection status, turns, and timers
Buttons for turn control, reset, and game time selection
Capture support using a graveyard area
How It Works

When a player moves a chess piece, the reed switches underneath the board detect which square changed. The ESP32 reads the board state through multiplexers, determines the move, and sends that move to the second board using MQTT over Wi-Fi.

The receiving board then moves its gantry system to the correct square. An electromagnet mounted under the board turns on, grabs the corresponding piece, moves it to the destination square, and turns off to release it. This allows the second physical board to mirror the move automatically.

Hardware Used
ESP32 microcontrollers
Reed switches
CD74HC4067 16-channel analog multiplexers
NEMA 17 stepper motors
A4988 stepper motor drivers
Electromagnet
IRLZ44N MOSFET
Buck converters
16x2 I2C LCD screens
Limit switches
Push buttons
12V power supplies
CoreXY gantry frame
3D printed motor, pulley, trolley, and LCD mounts
Software Used
Arduino IDE
C/C++
WiFi.h
PubSubClient MQTT library
LiquidCrystal_I2C library
Main Subsystems
Piece Detection

Each chess square has a reed switch underneath it. Magnets inside the chess pieces close the reed switches when pieces are present. Multiplexers allow the ESP32 to read all 64 squares without needing 64 separate input pins.

Wireless Communication

The two ESP32 boards communicate using MQTT. Each board can publish moves and subscribe to moves from the other board. This allows the boards to stay synchronized over Wi-Fi.

Motion Control

The gantry system uses two stepper motors to move an electromagnet underneath the board. The motors move the magnet to specific board coordinates so pieces can be moved from one square to another.

Electromagnet Control

The ESP32 controls the electromagnet through a MOSFET because the ESP32 cannot directly power the 12V electromagnet. The magnet turns on to grab a piece and turns off to release it.

User Interface

Each board includes an LCD and three buttons. The LCD displays connection status, turn information, and timers. The buttons are used for next turn, reset, and selecting different game time modes.

Challenges

Some of the main challenges included wiring a large number of reed switches, keeping jumper wires secure, calibrating the gantry movement, tuning the electromagnet strength, and preventing magnets inside the chess pieces from attracting each other too strongly.

## Code

The Arduino code for the ESP32 boards is located in the `code/` folder.

The project uses one main Arduino program for both boards. The board type is selected in the code using:

```cpp
#define BOARD_LEFT 1
```

Use:

```cpp
#define BOARD_LEFT 1
```

for the left board, and:

```cpp
#define BOARD_LEFT 0
```

for the right board.

Before uploading the code to an ESP32, update the Wi-Fi credentials:

```cpp
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
```

Do not upload real Wi-Fi passwords or private information to GitHub.

## Final Report

The full senior design final report is included in this repository. It contains the project background, motivation, diagrams, parts list, technical components, timeline, feasibility, testing results, and appendix code.

## Repository Structure

```text
Smart-Chessboard/
│
├── README.md
├── Final_Report.pdf
└── code/
    └── smart_chessboard.ino
```
