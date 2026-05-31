![Final Smart Chessboards](final_boards.jpg)

# Smart Chessboard

A wireless smart chessboard system that connects two physical chessboards over Wi-Fi.

When a player moves a piece on one board, the move is detected and mirrored on the second board.

---

## Overview

This project was made for our senior design project at Farmingdale State College.

The goal was to keep the physical feeling of playing chess while allowing two people to play from different locations.

The system uses ESP32 microcontrollers, reed switches, multiplexers, MQTT communication, stepper motors, and an electromagnet gantry system.

---

## What It Does

* Detects chess piece movement
* Sends moves between two boards over Wi-Fi
* Uses MQTT for board-to-board communication
* Moves pieces automatically on the opposite board
* Uses an electromagnet to move pieces from underneath the board
* Displays turn, timer, and connection information on an LCD
* Uses buttons for turn control, reset, and time selection
* Uses limit switches to home the gantry system
* Supports captured pieces using a graveyard area

---

## How It Works

1. A player moves a piece on one board.
2. Reed switches under the board detect the move.
3. The ESP32 reads the board through multiplexers.
4. The move is sent to the other board using MQTT.
5. The other ESP32 receives the move.
6. The gantry moves the electromagnet under the correct piece.
7. The electromagnet turns on, moves the piece, and releases it.
8. Both boards stay matched.

---

## Hardware

* ESP32 microcontrollers
* Reed switches
* 16-channel multiplexers
* NEMA 17 stepper motors
* Stepper motor drivers
* Electromagnet
* MOSFET
* Buck converters
* 16x2 I2C LCD displays
* Limit switches
* Push buttons
* 12V power supplies
* Magnetic chess pieces
* Wooden chessboard enclosure
* Aluminum rail gantry system
* 3D printed mounts and supports

---

## Software

* Arduino IDE
* C/C++
* WiFi.h
* PubSubClient MQTT library
* LiquidCrystal_I2C library

---

## Code

The Arduino code is located in the code folder.

The same program is used for both boards.

BOARD_LEFT 1 is used for the left board.

BOARD_LEFT 0 is used for the right board.

Before uploading code, Wi-Fi names and passwords should be changed to correct wifi name and password.

---

## Main Systems

Piece Detection:
Reed switches are placed underneath each square. Magnets inside the chess pieces activate the switches.

Wireless Communication:
The two ESP32 boards communicate using MQTT over Wi-Fi.

Motion Control:
Stepper motors move the gantry system underneath the board.

Electromagnet:
The electromagnet turns on to grab a piece and turns off to release it.

LCD and Buttons:
The LCD shows game information. The buttons control turn switching, reset, and time modes.

---

## Challenges

* Wiring all reed switches
* Reading all 64 squares with limited ESP32 pins
* Keeping jumper wires secure
* Calibrating the gantry movement
* Getting the electromagnet strength right
* Preventing chess piece magnets from attracting each other
* Keeping Wi-Fi and MQTT communication reliable

---

## Final Result

The final system connected two physical chessboards over Wi-Fi.

The boards were able to detect moves, communicate with each other, move pieces using the gantry and electromagnet, and display game information on the LCD.

---

## Team Members

* Ajay Dasrath
* Nicholas Markowski
* Jakub Kolakowski

---

## Project Status

Completed senior design project.
