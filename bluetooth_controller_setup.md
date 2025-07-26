# FeyBot Bluetooth Controller Setup Guide

## System Overview

This system consists of two main components:
1. **ESP32-C3 FeyBot Controller** - Sends control commands via Bluetooth
2. **Arduino Uno RoboArm** - Receives commands via HC-05/HC-06 Bluetooth module

## Hardware Requirements

### ESP32-C3 FeyBot Controller:
- ESP32-C3 microcontroller
- 2x Analog joysticks
- Rotary encoder with button
- Cherry MX button
- 128x64 OLED display (I2C)
- Breadboard and jumper wires

### Arduino Uno RoboArm:
- Arduino Uno
- Adafruit PCA9685 PWM Servo Driver
- HC-05 or HC-06 Bluetooth module
- 6x Servo motors
- Power supply for servos
- Jumper wires

## Wiring Diagrams

### ESP32-C3 FeyBot Controller Pinout:
```
Component          ESP32-C3 Pin
---------------------------------
Right Joystick X   → GPIO 0  (Wrist Rotation)
Right Joystick Y   → GPIO 1  (Wrist Up/Down)
Left Joystick X    → GPIO 3  (Base)
Left Joystick Y    → GPIO 4  (Shoulder)
Encoder A          → GPIO 7
Encoder B          → GPIO 10
Encoder Button     → GPIO 20
Cherry MX Button   → GPIO 6
Right Joy Button   → GPIO 2
Left Joy Button    → GPIO 5
OLED SDA           → GPIO 9
OLED SCL           → GPIO 8
VCC                → 3.3V
GND                → GND
```

### Arduino Uno RoboArm Wiring:
```
Component          Arduino Pin
---------------------------------
HC-05/HC-06 VCC    → 5V (or 3.3V)
HC-05/HC-06 GND    → GND
HC-05/HC-06 TX     → Pin 2 (BT_RX_PIN)
HC-05/HC-06 RX     → Pin 3 (BT_TX_PIN)
PCA9685 VCC        → 5V
PCA9685 GND        → GND
PCA9685 SDA        → A4
PCA9685 SCL        → A5
Status LED         → Pin 13 (built-in)
```

## Software Setup

### 1. ESP32-C3 Controller Libraries:
Install these libraries in Arduino IDE:
- `Adafruit GFX Library`
- `Adafruit SSD1306`
- `ESP32 Board Package` (for Bluetooth support)

### 2. Arduino Uno Libraries:
Install these libraries in Arduino IDE:
- `Adafruit PWM Servo Driver Library`
- `SoftwareSerial` (built-in)

### 3. Upload Code:
1. Upload `feybot_bluetooth_controller.ino` to ESP32-C3
2. Upload `roboarm_bluetooth_receiver.ino` to Arduino Uno

## Control Mapping

### Joystick Controls:
| Control | Input | Function | Range |
|---------|-------|----------|-------|
| Base Rotation | Left Stick X | Rotate base left/right | 0° - 360° |
| Shoulder | Left Stick Y | Move shoulder up/down | 90° - 540° |
| Wrist Rotation | Right Stick X | Rotate wrist | 0° - 130° |
| Wrist Up/Down | Right Stick Y | Move wrist up/down | 0° - 140° |
| Gripper | Encoder | Open/close gripper | 60° - 130° |

### Button Functions:
| Button | Function |
|--------|----------|
| Encoder Button | Reset all servos to home position |
| Cherry MX | Toggle camera mode (future use) |
| Joystick Buttons | Reserved for future features |

## Operation Instructions

### 1. Power Up Sequence:
1. Power on Arduino Uno (RoboArm will initialize servos)
2. Power on ESP32-C3 (FeyBot will start Bluetooth and show "Waiting for Bluetooth...")
3. Wait for automatic pairing (devices should connect automatically)
4. FeyBot display will show "Connected!" when paired
5. Arduino status LED will blink fast when receiving commands

### 2. Control Operation:
- **Smooth Control**: Move joysticks gradually for smooth servo movement
- **Deadzone**: Small joystick movements are ignored to prevent jitter
- **Real-time Feedback**: OLED shows current servo positions and joystick values
- **Safety Limits**: All servos have built-in range limits to prevent damage

### 3. Status Indicators:
- **FeyBot OLED**: Shows servo positions, joystick values, and connection status
- **Arduino LED**: 
  - Fast blink = Connected and receiving commands
  - Slow blink = Disconnected or no commands
- **Serial Output**: Both devices output debug information

## Troubleshooting

### Connection Issues:
1. **No Bluetooth Connection**:
   - Check ESP32-C3 Bluetooth is enabled
   - Verify HC-05/HC-06 wiring and power
   - Reset both devices and try again
   - Check baud rates match (9600)

2. **Commands Not Working**:
   - Verify Bluetooth connection status
   - Check servo power supply
   - Monitor Serial output for error messages
   - Ensure joysticks are centered at startup

3. **Erratic Movement**:
   - Check joystick wiring and connections
   - Verify servo power supply is adequate
   - Reduce movement speed in code if needed
   - Check for interference

### Configuration Options:

#### Adjustable Parameters in ESP32 Code:
```cpp
const int DEADZONE = 10;           // Joystick deadzone (0-100)
const int SERVO_STEP = 2;          // Movement speed (degrees per update)
const int UPDATE_INTERVAL = 100;   // Control update rate (ms)
const int ENCODER_SENSITIVITY = 2; // Encoder steps per servo degree
```

#### Adjustable Parameters in Arduino Code:
```cpp
const int CONNECTION_TIMEOUT = 3000; // Bluetooth timeout (ms)
const int LED_BLINK_INTERVAL = 1000; // Status LED blink rate (ms)
```

## Advanced Features

### 1. Camera Mode:
- Toggle with Cherry MX button
- Currently displays mode on OLED
- Reserved for future MediaPipe integration

### 2. Reset Function:
- Press encoder button to reset all servos to home position
- Useful for returning to known safe position

### 3. Connection Monitoring:
- Automatic timeout detection
- Status feedback on both devices
- Graceful handling of connection loss

## Safety Features

1. **Range Limiting**: All servos constrained to safe operational ranges
2. **Smooth Movement**: Gradual position changes prevent sudden jerky movements
3. **Connection Timeout**: Automatic safe mode if connection is lost
4. **Home Position**: Known safe starting position for all servos
5. **Status Monitoring**: Real-time feedback on system status

## Future Enhancements

- Camera mode integration with MediaPipe
- Additional control modes
- Wireless firmware updates
- Data logging and playback
- Multiple robot support

## Example Usage Session

```
1. Power on both devices
2. Wait for "Connected!" message on FeyBot OLED
3. Move left joystick left → Base rotates left
4. Move left joystick up → Shoulder moves up
5. Turn encoder clockwise → Gripper closes
6. Press encoder button → All servos return to home
7. Press Cherry MX → Toggle camera mode
```

The system provides intuitive, real-time control of the robotic arm with comprehensive feedback and safety features.