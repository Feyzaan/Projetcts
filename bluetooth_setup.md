# Bluetooth Setup for RoboArm

## Hardware Requirements

- HC-05 or HC-06 Bluetooth module
- Arduino Uno
- Jumper wires
- Breadboard (optional)

## Wiring Connections

### HC-05/HC-06 to Arduino Uno:
```
HC-05/HC-06    Arduino Uno
-----------    -----------
VCC       →    5V or 3.3V
GND       →    GND
TXD       →    Pin 2 (BT_RX_PIN)
RXD       →    Pin 3 (BT_TX_PIN)
```

**Important Notes:**
- Some HC-05 modules work with 5V, others require 3.3V - check your module's specifications
- If your module requires 3.3V, you may need a voltage divider for the RXD pin
- Make sure to connect TX to RX and RX to TX (cross-connect)

## Software Setup

1. **Install Required Libraries:**
   - Adafruit PWM Servo Driver Library
   - SoftwareSerial (built-in with Arduino IDE)

2. **Upload the Code:**
   - Use the `roboarm_bluetooth.ino` file
   - Select Arduino Uno as your board
   - Upload to your Arduino

## Bluetooth Module Configuration (Optional)

If you want to change the Bluetooth module name or PIN:

1. **Enter AT Command Mode:**
   - Connect HC-05 EN pin to HIGH (3.3V)
   - Power on the module
   - Open Serial Monitor at 38400 baud

2. **Common AT Commands:**
   ```
   AT                    // Test connection
   AT+NAME=RoboArm       // Set device name
   AT+PSWD=1234          // Set PIN code
   AT+UART=9600,0,0      // Set baud rate
   ```

## Usage

### Via Serial Monitor:
1. Open Arduino IDE Serial Monitor
2. Set baud rate to 115200
3. Send commands like: `B180`, `S270`, `A100`

### Via Bluetooth:
1. Pair your device with the Bluetooth module (default PIN: 1234 or 0000)
2. Use a Bluetooth terminal app on your phone/computer
3. Connect to the module
4. Send the same commands: `B180`, `S270`, `A100`

## Command Reference

| Command | Description | Range |
|---------|-------------|-------|
| B0-360  | Base rotation | 0° to 360° |
| S90-540 | Shoulder angle | 90° to 540° |
| A0-290  | Arm angle | 0° to 290° |
| R0-130  | Wrist rotation | 0° to 130° |
| W0-140  | Wrist angle | 0° to 140° |
| G60-130 | Gripper | 60° to 130° |

## Troubleshooting

1. **Module not responding:**
   - Check wiring connections
   - Verify power supply voltage
   - Try different baud rates

2. **Can't pair with device:**
   - Ensure module is in pairing mode
   - Check if module name appears in Bluetooth scan
   - Try default PINs: 1234, 0000, or 1111

3. **Commands not working:**
   - Verify command format (e.g., `B180` not `B 180`)
   - Check for line ending characters
   - Ensure Bluetooth connection is established

## Recommended Bluetooth Terminal Apps

- **Android:** Bluetooth Terminal, Serial Bluetooth Terminal
- **iOS:** Bluetooth Terminal, LightBlue Explorer
- **Windows:** Tera Term, PuTTY
- **Linux/Mac:** screen, minicom, or any serial terminal

## Example Usage Session

```
// Connect via Bluetooth terminal
> B180
Servo B set to 180

> S270
Servo S set to 270

> A100
Servo A set to 100

> G90
Servo G set to 90
```