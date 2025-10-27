// ===============================
// FeyBot Wireless Controller for ESP32-C3 (Bluetooth Version)
// ---------------------------------------
// Controls a robotic arm via Bluetooth Classic
// - 2x Analog Joysticks (Base, Shoulder, Wrist, Wrist Rotation)
// - Rotary Encoder (Gripper)
// - Encoder Button (Reset)
// - Cherry MX Button (Camera Mode Toggle)
// - 128x64 OLED Display (I2C)
// - Joystick Buttons (future use)
//
// Author: Feyzaan (2025) - Bluetooth version
// ===============================
//
// Pinout:
//   J1_X: GPIO 0   (Right joystick X - Wrist Rotation)
//   J1_Y: GPIO 1   (Right joystick Y - Wrist Up/Down)
//   J2_X: GPIO 3   (Left joystick X - Base)
//   J2_Y: GPIO 4   (Left joystick Y - Shoulder)
//   ENC_A: GPIO 7  (Encoder A)
//   ENC_B: GPIO 10 (Encoder B)
//   ENC_BTN: GPIO 20 (Encoder Button)
//   CHERRY_BTN: GPIO 6 (Cherry MX Button)
//   RJ_BTN: GPIO 2 (Right Joystick Button)
//   LJ_BTN: GPIO 5 (Left Joystick Button)
//   OLED SDA: GPIO 9
//   OLED SCL: GPIO 8

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// ========== OLED CONFIG ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ========== PIN DEFINITIONS ==========
#define J1_X 0   // Right joystick X (Wrist Rotation)
#define J1_Y 1   // Right joystick Y (Wrist Up/Down)
#define J2_X 3   // Left joystick X (Base)
#define J2_Y 4   // Left joystick Y (Shoulder)
#define ENC_A 7
#define ENC_B 10
#define ENC_BTN 20   // Encoder Button
#define CHERRY_BTN 6 // Cherry MX Button
#define RJ_BTN 2     // Right Joystick Button
#define LJ_BTN 5     // Left Joystick Button

// ========== SERVO CONTROL ==========
// Current servo positions (tracked locally)
int basePos = 180;      // B0-360
int shoulderPos = 270;  // S90-540 (display as 270)
int armPos = 145;       // A0-290
int wristRotPos = 65;   // R0-130
int wristPos = 70;      // W0-140
int gripperPos = 95;    // G60-130

// Control settings
const int DEADZONE = 10;  // Analog deadzone (out of 100)
const int SERVO_STEP = 2; // Degrees per update
const int ENCODER_SENSITIVITY = 2; // Encoder steps per servo degree

// ========== ENCODER (Robust State Machine) ==========
volatile int encoderCount = 0;
volatile uint8_t lastEncoderState = 0;

void IRAM_ATTR onEncoderChange() {
  uint8_t state = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  uint8_t combined = (lastEncoderState << 2) | state;
  
  if (combined == 0b0001 || combined == 0b0111 || combined == 0b1110 || combined == 0b1000) {
    encoderCount--;
  } else if (combined == 0b0010 || combined == 0b0100 || combined == 0b1101 || combined == 0b1011) {
    encoderCount++;
  }
  lastEncoderState = state;
}

volatile bool encoderBtnPressed = false;
void IRAM_ATTR onEncoderBtn() {
  encoderBtnPressed = true;
}

// ========== CHERRY BUTTON ==========
bool cameraMode = false;
bool lastCherryState = false;

// ========== TIMING ==========
unsigned long lastUpdate = 0;
const int UPDATE_INTERVAL = 100; // ms - slower for Bluetooth
unsigned long lastOledUpdate = 0;
const int OLED_UPDATE_INTERVAL = 300; // ms

// Normalize analogRead (0-4095) to -100 to 100
int normAxis(int val) {
  int normalized = map(val, 0, 4095, -100, 100);
  // Apply deadzone
  if (abs(normalized) < DEADZONE) return 0;
  return normalized;
}

// Show boot splash
void showBootScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(22, 26);
  display.print("FeyBot");
  display.setTextSize(1);
  display.setCursor(25, 45);
  display.print("Bluetooth");
  display.display();
  delay(1500);
  display.clearDisplay();
  display.display();
}

void sendServoCommand(char servo, int value) {
  String command = String(servo) + String(value);
  SerialBT.println(command);
  Serial.println("Sent: " + command);
}

void resetAllServos() {
  basePos = 180;
  shoulderPos = 270;
  armPos = 145;
  wristRotPos = 65;
  wristPos = 70;
  gripperPos = 95;
  
  sendServoCommand('B', basePos);
  delay(50);
  sendServoCommand('S', shoulderPos);
  delay(50);
  sendServoCommand('A', armPos);
  delay(50);
  sendServoCommand('R', wristRotPos);
  delay(50);
  sendServoCommand('W', wristPos);
  delay(50);
  sendServoCommand('G', gripperPos);
}

void setup() {
  Wire.begin(9, 8); // OLED I2C
  Serial.begin(115200);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  pinMode(CHERRY_BTN, INPUT_PULLUP);
  pinMode(RJ_BTN, INPUT_PULLUP);
  pinMode(LJ_BTN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), onEncoderChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_BTN), onEncoderBtn, FALLING);

  // Initialize encoder state
  lastEncoderState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (true);
  }
  display.setRotation(2);
  showBootScreen();

  // Initialize Bluetooth
  SerialBT.begin("FeyBot_Controller"); // Bluetooth device name
  Serial.println("FeyBot Bluetooth Controller Started");
  Serial.println("Waiting for connection...");
  
  // Wait for Bluetooth connection
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Waiting for");
  display.setCursor(0, 30);
  display.print("Bluetooth...");
  display.display();
  
  while (!SerialBT.hasClient()) {
    delay(100);
  }
  
  Serial.println("Bluetooth Connected!");
  display.clearDisplay();
  display.setCursor(0, 20);
  display.print("Connected!");
  display.display();
  delay(1000);
  
  // Send initial reset command
  resetAllServos();
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate < UPDATE_INTERVAL) return;
  lastUpdate = now;

  bool commandSent = false;

  // Read analog joystick values
  int j1x = normAxis(analogRead(J1_X)) * -1; // Invert right X
  int j1y = normAxis(analogRead(J1_Y));
  int j2x = normAxis(analogRead(J2_X));
  int j2y = normAxis(analogRead(J2_Y)) * -1; // Invert left Y

  // Base control (J2_X - Left joystick X)
  if (j2x != 0) {
    int newBase = constrain(basePos + (j2x > 0 ? SERVO_STEP : -SERVO_STEP), 0, 360);
    if (newBase != basePos) {
      basePos = newBase;
      sendServoCommand('B', basePos);
      commandSent = true;
    }
  }

  // Shoulder control (J2_Y - Left joystick Y)
  if (j2y != 0) {
    int newShoulder = constrain(shoulderPos + (j2y > 0 ? SERVO_STEP : -SERVO_STEP), 90, 540);
    if (newShoulder != shoulderPos) {
      shoulderPos = newShoulder;
      sendServoCommand('S', shoulderPos);
      commandSent = true;
    }
  }

  // Wrist Rotation control (J1_X - Right joystick X)
  if (j1x != 0) {
    int newWristRot = constrain(wristRotPos + (j1x > 0 ? SERVO_STEP : -SERVO_STEP), 0, 130);
    if (newWristRot != wristRotPos) {
      wristRotPos = newWristRot;
      sendServoCommand('R', wristRotPos);
      commandSent = true;
    }
  }

  // Wrist Up/Down control (J1_Y - Right joystick Y)
  if (j1y != 0) {
    int newWrist = constrain(wristPos + (j1y > 0 ? SERVO_STEP : -SERVO_STEP), 0, 140);
    if (newWrist != wristPos) {
      wristPos = newWrist;
      sendServoCommand('W', wristPos);
      commandSent = true;
    }
  }

  // Gripper control (Encoder)
  if (abs(encoderCount) >= ENCODER_SENSITIVITY) {
    int direction = encoderCount > 0 ? 1 : -1;
    int newGripper = constrain(gripperPos + (direction * SERVO_STEP), 60, 130);
    if (newGripper != gripperPos) {
      gripperPos = newGripper;
      sendServoCommand('G', gripperPos);
      commandSent = true;
    }
    encoderCount = 0; // Reset encoder count
  }

  // Handle encoder button (reset all servos)
  if (encoderBtnPressed) {
    encoderBtnPressed = false;
    resetAllServos();
    Serial.println("Reset all servos");
    commandSent = true;
  }

  // Handle Cherry MX toggle (camera mode)
  bool currentCherry = !digitalRead(CHERRY_BTN);
  if (currentCherry && !lastCherryState) {
    cameraMode = !cameraMode;
    Serial.println(cameraMode ? "Camera Mode ON" : "Camera Mode OFF");
  }
  lastCherryState = currentCherry;

  // Read joystick buttons (for future use)
  bool rJoyBtn = !digitalRead(RJ_BTN);
  bool lJoyBtn = !digitalRead(LJ_BTN);

  // OLED display update (throttled)
  if (now - lastOledUpdate >= OLED_UPDATE_INTERVAL) {
    lastOledUpdate = now;
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    
    display.print("B:"); display.print(basePos);
    display.print(" S:"); display.println(shoulderPos);
    display.print("A:"); display.print(armPos);
    display.print(" R:"); display.println(wristRotPos);
    display.print("W:"); display.print(wristPos);
    display.print(" G:"); display.println(gripperPos);
    
    display.print("Joy: ");
    display.print(j2x); display.print(","); display.print(j2y);
    display.print(" ");
    display.print(j1x); display.print(","); display.println(j1y);
    
    display.print("Mode: "); display.println(cameraMode ? "CAM" : "MANUAL");
    display.print("BT: "); display.println(SerialBT.hasClient() ? "CONN" : "DISC");
    
    display.display();
  }

  // Add small delay if command was sent to avoid overwhelming the receiver
  if (commandSent) {
    delay(50);
  }
}