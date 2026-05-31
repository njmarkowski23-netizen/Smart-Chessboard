#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =====================================================
// CHANGE THIS ONLY
// LEFT BOARD  = 1
// RIGHT BOARD = 0
// =====================================================
#define BOARD_LEFT 1

#if BOARD_LEFT
  #define BOARD_ID "LEFT"
#else
  #define BOARD_ID "RIGHT"
#endif

// =====================================================
// WIFI / MQTT
// =====================================================
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mainTopic = "nick_markowski_smartchess_2026/final_v3_chess";

WiFiClient espClient;
PubSubClient client(espClient);

// =====================================================
// LCD
// =====================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// =====================================================
// BUTTONS
// =====================================================
#define BTN_TURN   32
#define BTN_RESET  33
#define BTN_TIME   25

// =====================================================
// LIMIT SWITCHES
// =====================================================
#define X_LIMIT 2
#define Y_LIMIT 15

// =====================================================
// MUX PINS
// =====================================================
const int S0 = 27;
const int S1 = 26;
const int S2 = 14;
const int S3 = 4;

const int MUX1_SIG = 34;
const int MUX2_SIG = 35;
const int MUX3_SIG = 36;
const int MUX4_SIG = 39;

const bool PIECE_PRESENT_IS_LOW = true;

// =====================================================
// MOTOR PINS
// =====================================================
#define A_STEP 18
#define A_DIR  19
#define A_EN   16

#define B_STEP 23
#define B_DIR  5
#define B_EN   13

#define MAGNET_PIN 17

// =====================================================
// MOTOR SETTINGS
// =====================================================
int stepDelayUs = 900;

const float STEPS_PER_SQUARE_X = 200.0;
const float STEPS_PER_SQUARE_Y = 200.0;

const bool A_POS_DIR = true;
const bool B_POS_DIR = false;

const int settleMs = 250;

float currentX = 0.0;
float currentY = 0.0;

// =====================================================
// GRAVEYARD
// =====================================================
const float SAFE_Y = 2.0;
const float GL0_X = -1.0;
const float GL1_X = -2.0;
const float GR0_X = 8.0;
const float GR1_X = 9.0;
const float GY = 0.0;

// =====================================================
// CLOCK
// =====================================================
int timeModes[] = {600, 300, 60};
int timeModeIndex = 0;

int whiteTime = 600;
int brownTime = 600;

bool whiteTurn = true;
bool gameRunning = false;
bool gameOverDisplayed = false;

unsigned long lastClockTick = 0;

// =====================================================
// BOARD STATE
// =====================================================
bool currentState[8][8];
bool lastState[8][8];

String fromSquare = "";
String toSquare = "";

bool ignoreSensors = false;
bool isRemoteMove = false;

unsigned long lastChangeTime = 0;
const unsigned long debounceMs = 150;

// =====================================================
// BUTTON STATE
// =====================================================
bool lastTurnBtn = HIGH;
bool lastTimeBtn = HIGH;

// =====================================================
// TURN BUTTON LOCK
// =====================================================
// false = this board is allowed to press the turn button
// true  = this board already pressed and must wait for the other board
bool turnButtonLocked = false;

unsigned long resetPressStart = 0;
bool resetWasHeld = false;

const unsigned long resetHoldMs = 2000;

// =====================================================
// MUX MAPPINGS
// =====================================================
#if BOARD_LEFT

const char* mux1Map[16] = {
  "B1","B2","B3","B4","B5","B6","B7","B8",
  "A1","A2","A3","A4","A5","A6","A7","A8"
};

const char* mux2Map[16] = {
  "D1","D2","D3","D4","D5","D6","D7","D8",
  "C1","C2","C3","C4","C5","C6","C7","C8"
};

const char* mux3Map[16] = {
  "F1","F2","F3","F4","F5","F6","F7","F8",
  "E1","E2","E3","E4","E5","E6","E7","E8"
};

const char* mux4Map[16] = {
  "H1","H2","H3","H4","H5","H6","H7","H8",
  "G1","G2","G3","G4","G5","G6","G7","G8"
};

#else

const char* mux1Map[16] = {
  "B8","B7","B6","B5","B4","B3","B2","B1",
  "A8","A7","A6","A5","A4","A3","A2","A1"
};

const char* mux2Map[16] = {
  "D8","D7","D6","D5","D4","D3","D2","D1",
  "C8","C7","C6","C5","C4","C3","C2","C1"
};

const char* mux3Map[16] = {
  "F8","F7","F6","F5","F4","F3","F2","F1",
  "E8","E7","E6","E5","E4","E3","E2","E1"
};

const char* mux4Map[16] = {
  "H8","H7","H6","H5","H4","H3","H2","H1",
  "G8","G7","G6","G5","G4","G3","G2","G1"
};

#endif

// =====================================================
// MOTOR FUNCTIONS
// =====================================================
void enableMotors() {
  digitalWrite(A_EN, LOW);
  digitalWrite(B_EN, LOW);
}

void disableMotors() {
  digitalWrite(A_EN, HIGH);
  digitalWrite(B_EN, HIGH);
}

void magnetOn() {
  digitalWrite(MAGNET_PIN, HIGH);
  delay(900);
}

void magnetOff() {
  digitalWrite(MAGNET_PIN, LOW);
  delay(700);
}

bool xHit() {
  return digitalRead(X_LIMIT) == LOW;
}

bool yHit() {
  return digitalRead(Y_LIMIT) == LOW;
}

void stepBoth(long aSteps, bool aDir, long bSteps, bool bDir, int delayUs) {
  digitalWrite(A_DIR, aDir);
  digitalWrite(B_DIR, bDir);
  delay(3);

  long maxSteps = max(aSteps, bSteps);

  for (long i = 0; i < maxSteps; i++) {
    if (i < aSteps) digitalWrite(A_STEP, HIGH);
    if (i < bSteps) digitalWrite(B_STEP, HIGH);

    delayMicroseconds(delayUs);

    if (i < aSteps) digitalWrite(A_STEP, LOW);
    if (i < bSteps) digitalWrite(B_STEP, LOW);

    delayMicroseconds(delayUs);
  }
}

void moveBoardDelta(float dx, float dy) {
  if (dx == 0.0f && dy == 0.0f) return;

  long dxSteps = round(dx * STEPS_PER_SQUARE_X);
  long dySteps = round(dy * STEPS_PER_SQUARE_Y);

  long a = dxSteps + dySteps;
  long b = dxSteps - dySteps;

  bool dirA = (a >= 0) ? A_POS_DIR : !A_POS_DIR;
  bool dirB = (b >= 0) ? B_POS_DIR : !B_POS_DIR;

  enableMotors();
  stepBoth(abs(a), dirA, abs(b), dirB, stepDelayUs);

  currentX += dx;
  currentY += dy;

  delay(settleMs);
}

void homeMoveUntilSwitch(float dx, float dy, bool homeY) {
  long dxSteps = round(dx * STEPS_PER_SQUARE_X);
  long dySteps = round(dy * STEPS_PER_SQUARE_Y);

  long a = dxSteps + dySteps;
  long b = dxSteps - dySteps;

  bool dirA = (a >= 0) ? A_POS_DIR : !A_POS_DIR;
  bool dirB = (b >= 0) ? B_POS_DIR : !B_POS_DIR;

  long stepsA = abs(a);
  long stepsB = abs(b);
  long maxSteps = max(stepsA, stepsB);

  enableMotors();

  digitalWrite(A_DIR, dirA);
  digitalWrite(B_DIR, dirB);
  delay(3);

  for (long i = 0; i < maxSteps; i++) {
    if (homeY && yHit()) break;
    if (!homeY && xHit()) break;

    if (i < stepsA) digitalWrite(A_STEP, HIGH);
    if (i < stepsB) digitalWrite(B_STEP, HIGH);

    delayMicroseconds(stepDelayUs);

    if (i < stepsA) digitalWrite(A_STEP, LOW);
    if (i < stepsB) digitalWrite(B_STEP, LOW);

    delayMicroseconds(stepDelayUs);
  }
}

void homeBoard() {
  Serial.println("HOMING START");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Homing...");

  ignoreSensors = true;

  homeMoveUntilSwitch(0, -10.0, true);
  delay(200);
  homeMoveUntilSwitch(-10.0, 0, false);

  currentX = 0;
  currentY = 0;

  scanBoard();
  copyCurrentToLast();

  ignoreSensors = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Homed A1");
  delay(700);

  updateLCD();

  Serial.println("HOMED A1");
}

// =====================================================
// POSITION FUNCTIONS
// =====================================================
bool parseSquare(String sq, int &file, int &rank) {
  sq.trim();
  sq.toUpperCase();

  if (sq.length() != 2) return false;

  char f = sq.charAt(0);
  char r = sq.charAt(1);

  if (f < 'A' || f > 'H') return false;
  if (r < '1' || r > '8') return false;

  file = f - 'A';
  rank = r - '1';
  return true;
}

void goToPosition(float x, float y) {
  float dx = x - currentX;
  float dy = y - currentY;

  Serial.print("Going to x=");
  Serial.print(x, 3);
  Serial.print(" y=");
  Serial.println(y, 3);

  moveBoardDelta(dx, dy);
}

void goToSquare(String sq) {
  int file, rank;

  if (!parseSquare(sq, file, rank)) {
    Serial.println("Bad square");
    return;
  }

  goToPosition((float)file, (float)rank);
}

void goToGraveyard(String gy) {
  gy.toLowerCase();

  float gx;

  if (gy == "gl0") gx = GL0_X;
  else if (gy == "gl1") gx = GL1_X;
  else if (gy == "gr0") gx = GR0_X;
  else if (gy == "gr1") gx = GR1_X;
  else {
    Serial.println("Bad graveyard");
    return;
  }

  if (currentY < SAFE_Y) {
    goToPosition(currentX, SAFE_Y);
  }

  goToPosition(gx, SAFE_Y);
  goToPosition(gx, GY);
}

// =====================================================
// PICK / PLACE
// =====================================================
void pickAndPlace(String fromSq, String toSq) {
  Serial.print("PICK ");
  Serial.print(fromSq);
  Serial.print(" -> ");
  Serial.println(toSq);

  ignoreSensors = true;

  goToSquare(fromSq);

  delay(500);
  magnetOn();
  Serial.println("MAGNET ON");

  goToSquare(toSq);

  delay(250);
  magnetOff();
  Serial.println("MAGNET OFF");

  delay(400);

  scanBoard();
  copyCurrentToLast();

  ignoreSensors = false;
}

void captureMove(String capturedSq, String graveyard, String fromSq, String toSq) {
  ignoreSensors = true;

  goToSquare(capturedSq);
  magnetOn();
  goToGraveyard(graveyard);
  magnetOff();

  pickAndPlace(fromSq, toSq);

  scanBoard();
  copyCurrentToLast();

  ignoreSensors = false;
}

// =====================================================
// MUX / REED FUNCTIONS
// =====================================================
void selectMuxChannel(int ch) {
  digitalWrite(S0, bitRead(ch, 0));
  digitalWrite(S1, bitRead(ch, 1));
  digitalWrite(S2, bitRead(ch, 2));
  digitalWrite(S3, bitRead(ch, 3));
  delayMicroseconds(80);
}

void squareToIndex(String sq, int &file, int &rank) {
  parseSquare(sq, file, rank);
}

bool readMuxSig(int sigPin) {
  int v = digitalRead(sigPin);
  return PIECE_PRESENT_IS_LOW ? (v == LOW) : (v == HIGH);
}

void markSquare(String sq, bool present) {
  int f, r;
  if (!parseSquare(sq, f, r)) return;
  currentState[f][r] = present;
}

void scanBoard() {
  for (int f = 0; f < 8; f++) {
    for (int r = 0; r < 8; r++) {
      currentState[f][r] = false;
    }
  }

  for (int ch = 0; ch < 16; ch++) {
    selectMuxChannel(ch);

    markSquare(String(mux1Map[ch]), readMuxSig(MUX1_SIG));
    markSquare(String(mux2Map[ch]), readMuxSig(MUX2_SIG));
    markSquare(String(mux3Map[ch]), readMuxSig(MUX3_SIG));
    markSquare(String(mux4Map[ch]), readMuxSig(MUX4_SIG));
  }
}

void copyCurrentToLast() {
  for (int f = 0; f < 8; f++) {
    for (int r = 0; r < 8; r++) {
      lastState[f][r] = currentState[f][r];
    }
  }
}

String indexToSquare(int f, int r) {
  String s = "";
  s += char('A' + f);
  s += char('1' + r);
  s.toLowerCase();
  return s;
}

void printOccupiedSquares() {
  scanBoard();
  Serial.println("Occupied squares:");

  for (int r = 7; r >= 0; r--) {
    for (int f = 0; f < 8; f++) {
      if (currentState[f][r]) {
        Serial.print(indexToSquare(f, r));
        Serial.print(" ");
      }
    }
  }
  Serial.println();
}

// =====================================================
// CHESS LOGIC
// =====================================================
char chessBoard[8][8];

bool isWhitePiece(char p) {
  return p >= 'A' && p <= 'Z';
}

bool isBrownPiece(char p) {
  return p >= 'a' && p <= 'z';
}

bool isEmptyPiece(char p) {
  return p == '.';
}

void setupInitialChessBoard() {
  const char* whiteBack = "RNBQKBNR";
  const char* brownBack = "rnbqkbnr";

  for (int f = 0; f < 8; f++) {
    for (int r = 0; r < 8; r++) {
      chessBoard[f][r] = '.';
    }
  }

  for (int f = 0; f < 8; f++) {
    chessBoard[f][0] = whiteBack[f];
    chessBoard[f][1] = 'P';

    chessBoard[f][6] = 'p';
    chessBoard[f][7] = brownBack[f];
  }
}

int signInt(int v) {
  if (v > 0) return 1;
  if (v < 0) return -1;
  return 0;
}

bool pathClear(int fromF, int fromR, int toF, int toR) {
  int df = signInt(toF - fromF);
  int dr = signInt(toR - fromR);

  int f = fromF + df;
  int r = fromR + dr;

  while (f != toF || r != toR) {
    if (chessBoard[f][r] != '.') {
      return false;
    }

    f += df;
    r += dr;
  }

  return true;
}

void showIllegalMove(String fromSq, String toSq) {
  Serial.print("ILLEGAL MOVE: ");
  Serial.print(fromSq);
  Serial.print(" -> ");
  Serial.println(toSq);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Illegal move!");
  lcd.setCursor(0, 1);
  lcd.print(fromSq + " to " + toSq);
  delay(1200);
  updateLCD();
}

bool isLegalMove(String fromSq, String toSq, bool explain) {
  int fromF, fromR, toF, toR;

  if (!parseSquare(fromSq, fromF, fromR)) {
    if (explain) Serial.println("Illegal: bad from square");
    return false;
  }

  if (!parseSquare(toSq, toF, toR)) {
    if (explain) Serial.println("Illegal: bad to square");
    return false;
  }

  if (fromF == toF && fromR == toR) {
    if (explain) Serial.println("Illegal: same square");
    return false;
  }

  char piece = chessBoard[fromF][fromR];
  char target = chessBoard[toF][toR];

  if (piece == '.') {
    if (explain) Serial.println("Illegal: no piece on from square");
    return false;
  }

  if (whiteTurn && !isWhitePiece(piece)) {
    if (explain) Serial.println("Illegal: it is White's turn");
    return false;
  }

  if (!whiteTurn && !isBrownPiece(piece)) {
    if (explain) Serial.println("Illegal: it is Brown's turn");
    return false;
  }

  if (target != '.') {
    if (isWhitePiece(piece) && isWhitePiece(target)) {
      if (explain) Serial.println("Illegal: cannot capture own White piece");
      return false;
    }

    if (isBrownPiece(piece) && isBrownPiece(target)) {
      if (explain) Serial.println("Illegal: cannot capture own Brown piece");
      return false;
    }
  }

  int dx = toF - fromF;
  int dy = toR - fromR;

  char p = piece;
  if (p >= 'a' && p <= 'z') {
    p = p - 32;
  }

  bool legal = false;

  if (p == 'P') {
    int dir = isWhitePiece(piece) ? 1 : -1;
    int startRank = isWhitePiece(piece) ? 1 : 6;

    // forward 1
    if (dx == 0 && dy == dir && target == '.') {
      legal = true;
    }

    // forward 2 from starting rank
    if (dx == 0 && dy == 2 * dir && fromR == startRank && target == '.') {
      int middleR = fromR + dir;
      if (chessBoard[fromF][middleR] == '.') {
        legal = true;
      }
    }

    // diagonal capture
    if (abs(dx) == 1 && dy == dir && target != '.') {
      legal = true;
    }
  }

  else if (p == 'R') {
    if ((dx == 0 || dy == 0) && pathClear(fromF, fromR, toF, toR)) {
      legal = true;
    }
  }

  else if (p == 'B') {
    if (abs(dx) == abs(dy) && pathClear(fromF, fromR, toF, toR)) {
      legal = true;
    }
  }

  else if (p == 'N') {
    if ((abs(dx) == 1 && abs(dy) == 2) || (abs(dx) == 2 && abs(dy) == 1)) {
      legal = true;
    }
  }

  else if (p == 'Q') {
    if (((dx == 0 || dy == 0) || abs(dx) == abs(dy)) && pathClear(fromF, fromR, toF, toR)) {
      legal = true;
    }
  }

  else if (p == 'K') {
    if (abs(dx) <= 1 && abs(dy) <= 1) {
      legal = true;
    }
  }

  if (!legal) {
    if (explain) Serial.println("Illegal: piece cannot move that way or path blocked");
    return false;
  }

  return true;
}

void applyChessMove(String fromSq, String toSq) {
  int fromF, fromR, toF, toR;

  if (!parseSquare(fromSq, fromF, fromR)) return;
  if (!parseSquare(toSq, toF, toR)) return;

  char piece = chessBoard[fromF][fromR];

  chessBoard[toF][toR] = piece;
  chessBoard[fromF][fromR] = '.';

  // simple promotion to queen
  if (piece == 'P' && toR == 7) {
    chessBoard[toF][toR] = 'Q';
  }

  if (piece == 'p' && toR == 0) {
    chessBoard[toF][toR] = 'q';
  }
}

void printChessBoard() {
  Serial.println("Chess board:");

  for (int r = 7; r >= 0; r--) {
    Serial.print(r + 1);
    Serial.print(" ");

    for (int f = 0; f < 8; f++) {
      Serial.print(chessBoard[f][r]);
      Serial.print(" ");
    }

    Serial.println();
  }

  Serial.println("  A B C D E F G H");
}

bool validateApplyAndPublishMove(String fromSq, String toSq, bool publishIt) {
  fromSq.trim();
  toSq.trim();
  fromSq.toLowerCase();
  toSq.toLowerCase();

  Serial.print("Checking move: ");
  Serial.print(fromSq);
  Serial.print(" -> ");
  Serial.println(toSq);

  if (!isLegalMove(fromSq, toSq, true)) {
    showIllegalMove(fromSq, toSq);
    return false;
  }

  Serial.println("LEGAL MOVE");

  applyChessMove(fromSq, toSq);
  printChessBoard();

  if (publishIt && client.connected()) {
    String msg = "MOVE:";
    msg += BOARD_ID;
    msg += ":";
    msg += fromSq;
    msg += toSq;

    client.publish(mainTopic, msg.c_str());

    Serial.print("Published: ");
    Serial.println(msg);
  }

  return true;
}

// =====================================================
// LCD / CLOCK
// =====================================================
String formatTime(int seconds) {
  if (seconds < 0) seconds = 0;

  int m = seconds / 60;
  int s = seconds % 60;

  String out = String(m);
  out += ":";
  if (s < 10) out += "0";
  out += String(s);

  return out;
}

void updateLCD() {
  if (gameOverDisplayed) return;

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Turn: ");
  lcd.print(whiteTurn ? "White" : "Brown");

  lcd.setCursor(0, 1);
  lcd.print("W ");
  lcd.print(formatTime(whiteTime));
  lcd.print(" B ");
  lcd.print(formatTime(brownTime));
}

void showGameOverLCD() {
  gameOverDisplayed = true;
  gameRunning = false;

  lcd.clear();

  if (whiteTime <= 0) {
    // White ran out, so Brown wins.
    #if BOARD_LEFT
      lcd.setCursor(0, 0);
      lcd.print("You lose!");
    #else
      lcd.setCursor(0, 0);
      lcd.print("You win!");
    #endif
  }
  else if (brownTime <= 0) {
    // Brown ran out, so White wins.
    #if BOARD_LEFT
      lcd.setCursor(0, 0);
      lcd.print("You win!");
    #else
      lcd.setCursor(0, 0);
      lcd.print("You lose!");
    #endif
  }

  lcd.setCursor(0, 1);
  lcd.print("Game over");
}

void publishClockState();

void switchTurn(bool publishIt) {
  gameRunning = true;
  whiteTurn = !whiteTurn;
  turnButtonLocked = true;
  lastClockTick = millis();
  updateLCD();

  if (publishIt) publishClockState();
}

void cycleTimeMode(bool publishIt) {
  timeModeIndex++;
  if (timeModeIndex >= 3) timeModeIndex = 0;

  whiteTime = timeModes[timeModeIndex];
  brownTime = timeModes[timeModeIndex];
  gameRunning = false;
  gameOverDisplayed = false;
  whiteTurn = true;
  turnButtonLocked = false;

  setupInitialChessBoard();

  updateLCD();

  if (publishIt) publishClockState();
}

void resetGame(bool publishIt) {
  gameRunning = false;
  whiteTurn = true;

  whiteTime = timeModes[timeModeIndex];
  brownTime = timeModes[timeModeIndex];
  gameOverDisplayed = false;

  turnButtonLocked = false;

  fromSquare = "";
  toSquare = "";

  setupInitialChessBoard();

  updateLCD();

  if (publishIt) publishClockState();

  Serial.println("Game reset");
  printChessBoard();
}

void updateClock() {
  if (!gameRunning) return;
  if (gameOverDisplayed) return;

  unsigned long now = millis();

  if (now - lastClockTick >= 1000) {
    lastClockTick = now;

    if (whiteTurn) whiteTime--;
    else brownTime--;

    if (whiteTime <= 0 || brownTime <= 0) {
      if (whiteTime < 0) whiteTime = 0;
      if (brownTime < 0) brownTime = 0;
      showGameOverLCD();
      publishClockState();
      return;
    }

    updateLCD();
  }
}

// =====================================================
// MOVE DETECTION
// =====================================================
void detectChanges() {
  if (ignoreSensors) return;
  if (millis() - lastChangeTime < debounceMs) return;

  bool changed = false;

  for (int f = 0; f < 8; f++) {
    for (int r = 0; r < 8; r++) {
      if (currentState[f][r] != lastState[f][r]) {
        changed = true;
        lastChangeTime = millis();

        String sq = indexToSquare(f, r);

        if (lastState[f][r] == true && currentState[f][r] == false) {
          // piece removed
          fromSquare = sq;
          Serial.print("FROM detected: ");
          Serial.println(fromSquare);
        }

        if (lastState[f][r] == false && currentState[f][r] == true) {
          // piece placed
          toSquare = sq;
          Serial.print("TO detected: ");
          Serial.println(toSquare);
        }
      }
    }
  }

  if (fromSquare != "" && toSquare != "") {
    Serial.print("FULL MOVE DETECTED: ");
    Serial.print(fromSquare);
    Serial.print(" -> ");
    Serial.println(toSquare);

    // Chess logic is added here. If the move is illegal, it does NOT publish.
    validateApplyAndPublishMove(fromSquare, toSquare, true);

    fromSquare = "";
    toSquare = "";
  }

  // IMPORTANT: always update baseline after processing
  if (changed) {
    copyCurrentToLast();
  }
}
// =====================================================
// MQTT
// =====================================================
void publishClockState() {
  if (!client.connected()) return;

  String msg = "CLOCK:";
  msg += BOARD_ID;
  msg += ":";
  msg += (whiteTurn ? "1" : "0");
  msg += ":";
  msg += String(whiteTime);
  msg += ":";
  msg += String(brownTime);
  msg += ":";
  msg += String(timeModeIndex);
  msg += ":";
  msg += (gameRunning ? "1" : "0");

  client.publish(mainTopic, msg.c_str());
}

String getPart(String data, int index) {
  int start = 0;
  int end = data.indexOf(':');

  for (int i = 0; i < index; i++) {
    start = end + 1;
    end = data.indexOf(':', start);
    if (end == -1) end = data.length();
  }

  return data.substring(start, end);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("MQTT received: ");
  Serial.println(msg);

  String type = getPart(msg, 0);
  String sender = getPart(msg, 1);

  if (sender == BOARD_ID) {
    Serial.println("Ignoring own MQTT message");
    return;
  }

  if (type == "MOVE") {
    String move = getPart(msg, 2);

    if (move.length() != 4) return;

    String from = move.substring(0, 2);
    String to = move.substring(2, 4);

    // Check and update this board's virtual chess board too.
    if (isLegalMove(from, to, true)) {
      applyChessMove(from, to);
      printChessBoard();

      isRemoteMove = true;
      pickAndPlace(from, to);
      isRemoteMove = false;
    } else {
      Serial.println("Remote move rejected by chess logic");
    }
  }

  if (type == "CAPTURE") {
    String capturedSq = getPart(msg, 2);
    String graveyard = getPart(msg, 3);
    String fromSq = getPart(msg, 4);
    String toSq = getPart(msg, 5);

    if (isLegalMove(fromSq, toSq, true)) {
      applyChessMove(fromSq, toSq);
      printChessBoard();

      isRemoteMove = true;
      captureMove(capturedSq, graveyard, fromSq, toSq);
      isRemoteMove = false;
    } else {
      Serial.println("Remote capture rejected by chess logic");
    }
  }

  if (type == "CLOCK") {
    whiteTurn = getPart(msg, 2) == "1";
    whiteTime = getPart(msg, 3).toInt();
    brownTime = getPart(msg, 4).toInt();
    timeModeIndex = getPart(msg, 5).toInt();
    gameRunning = getPart(msg, 6) == "1";

    lastClockTick = millis();

    // If the other board reports timeout, show the win/lose screen once.
    if (whiteTime <= 0 || brownTime <= 0) {
      if (whiteTime < 0) whiteTime = 0;
      if (brownTime < 0) brownTime = 0;
      gameRunning = false;
      showGameOverLCD();
      return;
    }

    // Normal clock update from the other board. This also unlocks this board's button.
    turnButtonLocked = false;
    gameOverDisplayed = false;
    updateLCD();
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  delay(700);
}

void reconnectMQTT() {
  while (!client.connected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting MQTT");

    String clientId = String(BOARD_ID) + "_ESP32_" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      client.subscribe(mainTopic);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT Connected");
      delay(700);

      updateLCD();
    } else {
      delay(1000);
    }
  }
}

// =====================================================
// BUTTONS
// =====================================================
void handleButtons() {
  bool turnBtn = digitalRead(BTN_TURN);
  bool resetBtn = digitalRead(BTN_RESET);
  bool timeBtn = digitalRead(BTN_TIME);

  if (lastTurnBtn == HIGH && turnBtn == LOW) {
    delay(30);
    if (digitalRead(BTN_TURN) == LOW) {
      if (!turnButtonLocked) {
        switchTurn(true);
      } else {
        Serial.println("Turn button ignored: waiting for other board");
      }
    }
  }
  lastTurnBtn = turnBtn;

  if (lastTimeBtn == HIGH && timeBtn == LOW) {
    delay(30);
    if (digitalRead(BTN_TIME) == LOW) {
      cycleTimeMode(true);
    }
  }
  lastTimeBtn = timeBtn;

  if (resetBtn == LOW) {
    if (resetPressStart == 0) {
      resetPressStart = millis();
      resetWasHeld = false;
    }

    if (!resetWasHeld && millis() - resetPressStart >= resetHoldMs) {
      resetWasHeld = true;
      resetGame(true);
    }
  } else {
    resetPressStart = 0;
    resetWasHeld = false;
  }
}

// =====================================================
// SERIAL
// =====================================================
void publishManualMove(String move) {
  String msg = "MOVE:";
  msg += BOARD_ID;
  msg += ":";
  msg += move;

  client.publish(mainTopic, msg.c_str());
}

void handleSerial() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "home") {
    homeBoard();
  }
  else if (cmd == "switches") {
    Serial.print("X_LIMIT=");
    Serial.print(digitalRead(X_LIMIT));
    Serial.print(" Y_LIMIT=");
    Serial.println(digitalRead(Y_LIMIT));
  }
  else if (cmd == "reeds") {
    printOccupiedSquares();
  }
  else if (cmd == "board") {
    printChessBoard();
  }
  else if (cmd == "where") {
    Serial.print("X=");
    Serial.print(currentX);
    Serial.print(" Y=");
    Serial.println(currentY);
  }
  else if (cmd == "magon") {
    digitalWrite(MAGNET_PIN, HIGH);
    Serial.println("Magnet ON");
  }
  else if (cmd == "magoff") {
    digitalWrite(MAGNET_PIN, LOW);
    Serial.println("Magnet OFF");
  }
  else if (cmd == "reset") {
    resetGame(true);
  }
  else if (cmd == "time") {
    cycleTimeMode(true);
  }
  else if (cmd == "turn") {
    if (!turnButtonLocked) {
      switchTurn(true);
    } else {
      Serial.println("Turn command ignored: waiting for other board");
    }
  }
  else if (cmd.startsWith("goto ")) {
    String target = cmd.substring(5);
    target.trim();

    if (target.startsWith("gl") || target.startsWith("gr")) {
      goToGraveyard(target);
    } else {
      goToSquare(target);
    }
  }
  else if (cmd.startsWith("move ")) {
    int p1 = cmd.indexOf(' ');
    int p2 = cmd.indexOf(' ', p1 + 1);

    String from = cmd.substring(p1 + 1, p2);
    String to = cmd.substring(p2 + 1);

    if (validateApplyAndPublishMove(from, to, true)) {
      pickAndPlace(from, to);
    }
  }
  else if (cmd.startsWith("capture ")) {
    int p1 = cmd.indexOf(' ');
    int p2 = cmd.indexOf(' ', p1 + 1);
    int p3 = cmd.indexOf(' ', p2 + 1);
    int p4 = cmd.indexOf(' ', p3 + 1);

    String capturedSq = cmd.substring(p1 + 1, p2);
    String graveyard = cmd.substring(p2 + 1, p3);
    String fromSq = cmd.substring(p3 + 1, p4);
    String toSq = cmd.substring(p4 + 1);

    // For a capture, chess logic validates fromSq -> toSq while the target still exists.
    if (isLegalMove(fromSq, toSq, true)) {
      applyChessMove(fromSq, toSq);
      printChessBoard();

      captureMove(capturedSq, graveyard, fromSq, toSq);

      if (client.connected()) {
        String msg = "CAPTURE:";
        msg += BOARD_ID;
        msg += ":";
        msg += capturedSq;
        msg += ":";
        msg += graveyard;
        msg += ":";
        msg += fromSq;
        msg += ":";
        msg += toSq;
        client.publish(mainTopic, msg.c_str());
        Serial.print("Published: ");
        Serial.println(msg);
      }
    } else {
      showIllegalMove(fromSq, toSq);
    }
  }
  else if (cmd.length() == 4) {
    String from = cmd.substring(0, 2);
    String to = cmd.substring(2, 4);

    if (validateApplyAndPublishMove(from, to, true)) {
      pickAndPlace(from, to);
    }
  }
  else {
    Serial.println("Commands:");
    Serial.println("home, switches, reeds, board, where");
    Serial.println("goto a1, goto gl0");
    Serial.println("move e2 e4 OR e2e4");
    Serial.println("magon, magoff");
    Serial.println("turn, time, reset");
  }
}

// =====================================================
// SETUP / LOOP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.print("STARTING ");
  Serial.println(BOARD_ID);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Chessboard");
  lcd.setCursor(0, 1);
  lcd.print(BOARD_ID);
  delay(1000);

  pinMode(BTN_TURN, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_TIME, INPUT_PULLUP);

  pinMode(X_LIMIT, INPUT_PULLUP);
  pinMode(Y_LIMIT, INPUT_PULLUP);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  pinMode(MUX1_SIG, INPUT_PULLUP);
  pinMode(MUX2_SIG, INPUT_PULLUP);
  pinMode(MUX3_SIG, INPUT_PULLUP);
  pinMode(MUX4_SIG, INPUT_PULLUP);

  pinMode(A_STEP, OUTPUT);
  pinMode(A_DIR, OUTPUT);
  pinMode(A_EN, OUTPUT);

  pinMode(B_STEP, OUTPUT);
  pinMode(B_DIR, OUTPUT);
  pinMode(B_EN, OUTPUT);

  pinMode(MAGNET_PIN, OUTPUT);
  digitalWrite(MAGNET_PIN, LOW);

  digitalWrite(A_STEP, LOW);
  digitalWrite(B_STEP, LOW);

  disableMotors();

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnectMQTT();

  scanBoard();
  copyCurrentToLast();

  resetGame(false);

  Serial.println("Ready");
  Serial.println("Type home first");
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  handleButtons();
  handleSerial();
  updateClock();

  scanBoard();
  detectChanges();

  delay(20);
}
