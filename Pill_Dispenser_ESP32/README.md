# ESP32 Pill Dispenser System

5-compartment rotating pill dispenser with automatic homing, positioning, and BLE control.

## System Overview

**Homing**: Auto on startup + Button 6 (D25) manual trigger  
**Positioning**: 5 compartments at 72° intervals (0°, 72°, 144°, 216°, 288°)  
**Dispensing**: 3 retry attempts with electromagnet + servo + IR detection  
**Control**: Physical buttons + Bluetooth Low Energy

## File Structure

```
NewCodeLara/
├── NewCodeLara.ino               ← Main program
├── Config.h                      ← Pin definitions
├── ConfigurationSettings.h       ← TUNE SETTINGS HERE ⭐
├── SensorManager.h               ← Sensors & interrupts
├── HardwareController.h          ← Motors/servo/magnet
├── DispenserController.h         ← Homing & positioning
├── BLEManager.h                  ← Bluetooth
├── UIManager.h                   ← LCD & buttons
└── Documentation/
    ├── README.md                             (this file)
    ├── BUTTON_REFERENCE.md                   Button assignments
    ├── COMPARTMENT_POSITIONING_GUIDE.md      Positioning details
    ├── POSITIONING_SYSTEM_EXPLAINED.md       Math & calculations
    └── ARCHITECTURE.md                       Code structure
```

# Button Reference

```
## Button Assignments

| Button | Pin | Function |
|--------|-----|----------|
| 1 | D13 | Select Compartment 1 |
| 2 | D14 | Select Compartment 2 |
| 3 | D27 | Select Compartment 3 |
| 4 | VP | Select Compartment 4 |
| 5 | D33 | Select Compartment 5 |
| **6** | **D25** | **Trigger Homing (1s debounce)** |
| **7** | **D26** | **Dispense Pill** |

## Usage

**Startup**: Auto-homing runs  
**Buttons 1-5**: Select compartment  
**Button 6**: Re-home anytime  
**Button 7**: Dispense from selected compartment
```

## Pin Assignments

### Sensors & Buttons
```
D12     = Home switch (pressure, LOW when pressed)
D35     = IR pill detector
D36/D34 = Encoder channels
D13     = Button 1 (Compartment 1)
D14     = Button 2 (Compartment 2)
D27     = Button 3 (Compartment 3)
VP      = Button 4 (Compartment 4)
D33     = Button 5 (Compartment 5)
D25     = Button 6 (Homing trigger)
D26     = Button 7 (Dispense)
```

### Actuators
```
D16     = Motor speed (PWM)
D17     = Motor direction IN1
D18     = Motor direction IN2
D4      = Servo signal
D15     = Electromagnet control
D2      = Status LED
```

## Quick Start

### 1. Upload Code
1. Open `NewCodeLara.ino` in Arduino IDE
2. All `.h` files included automatically
3. Select ESP32 board
4. Click Upload

### 2. First Run
- System auto-homes on startup
- Green LED turns on when ready
- LCD shows "Slot: 1 Ready"
- BLE advertises as "PillDispenser"

### 3. Basic Operation
- **Select compartment**: Press Button 1-5
- **Dispense pill**: Press Button 7
- **Re-home**: Press Button 6 (if position drifts)

## Critical Configuration

Edit `ConfigurationSettings.h`:

```cpp
// Motor speeds (use 200-255 range for most drivers)
int motorHomingSpeedPWM = 255;
int motorRunningSpeedPWM = 255;

// Container positions (degrees from home/start position)
// ARRAY-BASED: Customize any container position
float containerPositionsInDegrees[5] = {
    0.0,      // Container 1: at home (0°)
    72.0,     // Container 2: 72° from home
    144.0,    // Container 3: 144° from home
    216.0,    // Container 4: 216° from home
    288.0     // Container 5: 288° from home
};
// Example custom spacing: {0.0, 75.0, 150.0, 220.0, 290.0}

// ⚠️ MUST CALIBRATE: Rotation speed for positioning accuracy
float estimatedMotorDegreesPerSecond = 180.0;

// Auto-homing after dispense
bool autoHomeAfterDispense = true;  // Automatically home after successful pill dispense

// Dispense settings
int maximumDispenseAttempts = 3;
int pillDetectionTimeoutMilliseconds = 2000;
int servoDispensingAngleInDegrees = 90;
```

## Calibration (REQUIRED!)

### Motor Speed Calibration
```
1. Home system (Button 6 or startup)
2. Select Container 5 (Button 5)
3. Time how long plate takes to reach Container 5
   Expected: ~1.6 seconds at default 180°/s
4. Calculate: actual_speed = 288° / time_in_seconds
   Example: 288° / 2.0s = 144°/s
5. Update estimatedMotorDegreesPerSecond in ConfigurationSettings.h
6. Re-upload and test all compartments
```

### Fine-tuning
- **Auto-homing**: Set `autoHomeAfterDispense = false` to disable auto-home after dispense
- **Container positions**: Edit `containerPositionsInDegrees[]` array for custom spacing
- **Servo angle**: Adjust `servoDispensingAngleInDegrees` (0-180°)
- **IR timeout**: Adjust `pillDetectionTimeoutMilliseconds` if pills not detected
- **Settling time**: Adjust `delayAfterCompartmentMoveMilliseconds` if plate oscillates

## BLE Commands

Send via Bluetooth app:
```
HOME           → Trigger homing sequence
DISPENSE:3:1   → Dispense 1 pill from compartment 3
STATUS         → Get dispense statistics
RESET          → Reset counters
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Homing fails | Check D12 switch wired correctly (LOW when pressed) |
| Wrong compartment reached | Calibrate `estimatedMotorDegreesPerSecond` or adjust positions in `containerPositionsInDegrees[]` array |
| Position drifts | Press Button 6 to re-home periodically |
| Pills not detected | Check D35 IR sensor, increase timeout |
| Motor doesn't move | Check wiring D16/D17/D18, try PWM 200-255 |
| Servo doesn't move | Check D4 connection and angle settings |

## Operation Flow

```
Power On
    ↓
5-Second Switch Diagnostic
    ↓
Auto-Homing Sequence (Button 6 also triggers this)
    ↓
System Ready
    ↓
Select Compartment (Buttons 1-5)
    ↓
Plate Rotates to Position
    ↓
Press Dispense (Button 7)
    ↓
Electromagnet → Servo → IR Detect → Success/Retry
    ↓
Auto-Home After Dispense (if enabled)
    ↓
System Ready (at home position)
```

## Further Documentation

- **BUTTON_REFERENCE.md** - Complete button & pin reference
- **COMPARTMENT_POSITIONING_GUIDE.md** - Detailed positioning system explanation
- **POSITIONING_SYSTEM_EXPLAINED.md** - Position calculation logic and examples
- **ARCHITECTURE.md** - Code structure and module relationships

## System Status

✅ **Operational** - All core features implemented and tested

**Features**:
- ✅ Homing with retry and diagnostics
- ✅ Position-based compartment navigation
- ✅ Multi-attempt pill dispensing with IR detection
- ✅ BLE control and statistics
- ✅ LCD display and button interface

**Next Steps**:
1. Calibrate motor speed for your hardware
2. Test dispense from all 5 compartments
3. Adjust settings as needed

---

**Version**: 1.0  
**Hardware**: ESP32 with 5-compartment rotating dispenser  
**Motor Speed**: 255 PWM (calibrate `estimatedMotorDegreesPerSecond`)

