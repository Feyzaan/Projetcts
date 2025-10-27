#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <SoftwareSerial.h>

// Bluetooth module connections
#define BT_RX_PIN 2  // Connect to HC-05 TX pin
#define BT_TX_PIN 3  // Connect to HC-05 RX pin
SoftwareSerial bluetooth(BT_RX_PIN, BT_TX_PIN);

#define SERVOMIN  150
#define SERVOMAX  600
#define SERVO_FREQ 50
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#define CH_BASE      0
#define CH_SHOULDER  1
#define CH_ARM       2
#define CH_WRIST_ROT 3
#define CH_WRIST     4
#define CH_GRAPPLER  5
#define NUM_SERVOS   16

int servoPos[NUM_SERVOS]   = {180, 180, 180, 180, 180, 180};
int targetPos[NUM_SERVOS]  = {180, 180, 180, 180, 180, 180};

// Safe range limits for each servo
int minAngle[NUM_SERVOS] = {  0,   0,   0,   0,   0,  60 };
int maxAngle[NUM_SERVOS] = {360, 450, 290, 130, 140, 130 };  // Shoulder max is 540 - 90 = 450

// Status LED
#define STATUS_LED 13
bool ledState = false;
unsigned long lastLedBlink = 0;
const int LED_BLINK_INTERVAL = 1000; // Blink every second when connected

// Connection status
bool bluetoothConnected = false;
unsigned long lastCommandTime = 0;
const int CONNECTION_TIMEOUT = 3000; // 3 seconds timeout

int mapAngle(int ch, int angle) {
  if (ch == CH_BASE || ch == CH_SHOULDER || ch == CH_ARM) {
    return map(constrain(angle, 0, 360), 0, 360, SERVOMIN, SERVOMAX);
  } else {
    return map(constrain(angle, 0, 180), 0, 180, SERVOMIN, SERVOMAX);
  }
}

int smooth(int current, int target, int step=1) {
  if (current < target) return min(current + step, target);
  if (current > target) return max(current - step, target);
  return current;
}

void sendResponse(String message) {
  // Send response to both Serial and Bluetooth
  Serial.println(message);
  bluetooth.println(message);
}

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() < 2) return;
  
  char ch = toupper(cmd.charAt(0));
  int val = cmd.substring(1).toInt();
  int idx = -1;

  // Update connection status
  bluetoothConnected = true;
  lastCommandTime = millis();

  if (ch == 'B') idx = CH_BASE;
  else if (ch == 'S') idx = CH_SHOULDER;
  else if (ch == 'A') idx = CH_ARM;
  else if (ch == 'R') idx = CH_WRIST_ROT;
  else if (ch == 'W') idx = CH_WRIST;
  else if (ch == 'G') idx = CH_GRAPPLER;

  if (idx >= 0) {
    // Special handling for Shoulder
    if (idx == CH_SHOULDER) {
      if (val < 90 || val > 540) {
        sendResponse("Shoulder range: S90–S540");
        return;
      }
      val -= 90; // Convert to internal angle (0–450)
    }

    // Clamp to each servo's valid range
    val = constrain(val, minAngle[idx], maxAngle[idx]);
    targetPos[idx] = val;

    String response = "Servo ";
    response += ch;
    response += " set to ";
    if (idx == CH_SHOULDER)
      response += String(val + 90); // Display user-friendly value
    else
      response += String(val);
    
    sendResponse(response);
  } else if (ch == 'H') {
    // Heartbeat/Hello command from controller
    sendResponse("RoboArm OK");
  } else {
    sendResponse("Invalid command. Use B0-360, S90-540, A0-290, R0-130, W0-140, G60-130");
  }
}

void initializeServos() {
  // Set initial positions
  servoPos[CH_BASE] = targetPos[CH_BASE] = 180;
  servoPos[CH_SHOULDER] = targetPos[CH_SHOULDER] = 180; // Internal angle (270 in user terms)
  servoPos[CH_ARM] = targetPos[CH_ARM] = 145;
  servoPos[CH_WRIST_ROT] = targetPos[CH_WRIST_ROT] = 65;
  servoPos[CH_WRIST] = targetPos[CH_WRIST] = 70;
  servoPos[CH_GRAPPLER] = targetPos[CH_GRAPPLER] = 95;
  
  // Apply to servos
  for (int i = 0; i < 6; i++) {
    int pulse = mapAngle(i, servoPos[i]);
    pwm.setPWM(i, 0, pulse);
  }
}

void checkConnectionStatus() {
  unsigned long now = millis();
  
  // Check if we've lost connection
  if (bluetoothConnected && (now - lastCommandTime > CONNECTION_TIMEOUT)) {
    bluetoothConnected = false;
    sendResponse("Connection timeout - entering safe mode");
  }
  
  // Blink status LED
  if (now - lastLedBlink >= LED_BLINK_INTERVAL) {
    lastLedBlink = now;
    if (bluetoothConnected) {
      // Fast blink when connected and receiving commands
      ledState = !ledState;
    } else {
      // Slow blink when disconnected
      ledState = (now / 2000) % 2;
    }
    digitalWrite(STATUS_LED, ledState);
  }
}

void setup() {
  Wire.begin();
  Serial.begin(115200);
  bluetooth.begin(9600);  // Standard baud rate for HC-05/HC-06
  
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
  
  // Initialize servos to safe positions
  initializeServos();
  
  sendResponse("RoboArm UNO Bluetooth Ready!");
  sendResponse("Waiting for FeyBot controller...");
  Serial.println("Servo ranges: B0-360, S90-540, A0-290, R0-130, W0-140, G60-130");
  
  // Send initial status
  bluetooth.println("READY");
}

void loop() {
  // Check for commands from Serial Monitor
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    processCommand(cmd);
  }
  
  // Check for commands from Bluetooth
  if (bluetooth.available()) {
    String cmd = bluetooth.readStringUntil('\n');
    processCommand(cmd);
  }

  // Check connection status and update LED
  checkConnectionStatus();

  // Smooth movement for all servos
  for (int i = 0; i < 6; i++) {
    int oldPos = servoPos[i];
    servoPos[i] = smooth(servoPos[i], targetPos[i], 2); // Faster movement step
    
    // Only update PWM if position changed
    if (servoPos[i] != oldPos) {
      int pulse = mapAngle(i, servoPos[i]);
      pwm.setPWM(i, 0, pulse);
    }
  }
  
  delay(20);
}