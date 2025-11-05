# ESP32 Pill Dispenser System

5-compartment rotating pill dispenser with automatic homing, positioning, and BLE control.

## System Overview

**Homing**: Auto on startup + Button 6 (D25) manual trigger  
**Positioning**: 5 compartments at 72° intervals (0°, 72°, 144°, 216°, 288°)  
**Dispensing**: 3 retry attempts with electromagnet + servo + IR detection  
**Control**: Physical buttons + Bluetooth Low Energy

## File Structure

```
1.Pill_Dispenser_ESP32/
├── 1.Pill_Dispenser_ESP32.ino    ← Main program
├── Config.h                      ← Pin definitions
├── ConfigurationSettings.h       ← TUNE SETTINGS HERE ⭐
├── SensorManager.h               ← Sensors & interrupts
├── HardwareController.h          ← Motors/servo/magnet
├── DispenserController.h         ← Homing & positioning
├── BLEManager.h                  ← Bluetooth
├── UIManager.h                   ← LCD & buttons
├── README.md                     ← This file (system documentation)
├── POSITIONING_SYSTEM_EXPLAINED.md ← Math & calculations
└── ARCHITECTURE.md               ← Code structure
```

# Button Reference

```
## Button Assignments

| Button | Pin | Function |
|--------|-----|----------|
| 1 | D13 | Select Compartment 1 |
| 2 | D14 | Select Compartment 2 |
| 3 | D27 | Select Compartment 3 |
| 4 | VP(D36) | Select Compartment 4 |
| 5 | D33 | Select Compartment 5 |
| **6** | **D25** | **Trigger Homing (1s debounce)** / **Calibration (hold 3+ sec)** |
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
VN(39)  = Encoder channel 1
D34     = Encoder channel 2
D13     = Button 1 (Compartment 1)
D14     = Button 2 (Compartment 2)
D27     = Button 3 (Compartment 3)
VP(D36) = Button 4 (Compartment 4)
D33     = Button 5 (Compartment 5)
D25     = Button 6 (Homing trigger)
D26     = Button 7 (Dispense)
```

### Actuators
```
D16     = Stepper motor DIR (direction)
D17     = Stepper motor STEP (step pulse)
D18     = Stepper motor EN (enable, LOW=enabled)
D4      = Servo signal
D15     = Electromagnet control
D2      = Status LED
```

## Quick Start

### 1. Upload Code
1. Open `1.Pill_Dispenser_ESP32.ino` in Arduino IDE
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
- **Re-home**: Press Button 6 (short press)
- **Calibration (full rotation timing)**: Press and hold Button 6 for 3+ seconds

## Critical Configuration

Edit `ConfigurationSettings.h`:

```cpp
// ⚠️ MUST CALIBRATE: Stepper motor mechanical configuration
int stepperStepsPerRevolution = 200;        // Steps per rotation (200 for 1.8°, 400 for 0.9°)
int stepperMicrostepping = 1;               // Microstepping on driver (1, 2, 4, 8, 16)
float stepperGearRatio = 1.0;               // Gear reduction (1.0 if none)

// ⚠️ MUST CALIBRATE: Step pulse timing - SYMMETRIC PULSE WIDTH (in microseconds)
// How stepper motors work: Uses symmetric pulse timing (HIGH for pulseWidth, LOW for pulseWidth)
// Total time per step = 2 × stepperStepPulseWidthMicroseconds
// Speed = 1,000,000 / (2 × pulseWidth) steps per second
// Lower pulse width = faster movement, higher pulse width = slower movement
int stepperStepPulseWidthMicroseconds = 15000;   // Pulse width for HIGH and LOW phases (symmetric timing)

// Safety limits (to prevent motor damage)
int stepperMinStepPulseWidthMicroseconds = 10000;   // Minimum safe pulse width (fastest speed)
int stepperMaxStepPulseWidthMicroseconds = 50000;    // Maximum pulse width (slowest speed)

// Container positions (degrees from home/start position)
// ARRAY-BASED: Customize any container position
float containerPositionsInDegrees[5] = {
    0.0,      // Container 1: at home (0°)
    65.0,     // Container 2: 65° from home
    144.0,    // Container 3: 144° from home
    216.0,    // Container 4: 216° from home
    288.0     // Container 5: 288° from home
};
// Note: Current spacing is {0.0, 65.0, 144.0, 216.0, 288.0}
// Example custom spacing: {0.0, 72.0, 144.0, 216.0, 288.0} for even 72° spacing

// Auto-homing after dispense
bool autoHomeAfterDispense = true;  // Automatically home after successful pill dispense

// Dispense settings
int maximumDispenseAttempts = 3;
int pillDetectionTimeoutMilliseconds = 2000;
// Note: Servo uses servoMinMicroseconds and servoMaxMicroseconds for full arc sweep
```

## Calibration (REQUIRED!)

### Stepper Motor Configuration
```
1. Set stepperStepsPerRevolution (typically 200 for 1.8° motor, 400 for 0.9°)
2. Set stepperMicrostepping (check your driver settings: 1, 2, 4, 8, or 16)
3. Set stepperGearRatio (1.0 if no gear reduction, otherwise your ratio)
4. Calibrate step pulse width timing (symmetric pulse timing):
   - System uses symmetric pulse timing: HIGH for pulseWidth, LOW for pulseWidth
   - Total time per step = 2 × pulseWidth
   - Start with stepperStepPulseWidthMicroseconds = 15000μs (30,000μs per step = ~33 steps/sec, ~10 RPM for 200 steps/rev)
   - Test minimum pulse width: Start with 10000μs, reduce until motor misses steps
   - Set stepperMinStepPulseWidthMicroseconds to safe minimum (prevents motor damage)
   - Set stepperMaxStepPulseWidthMicroseconds for slowest speed (50000μs default = ~10 steps/sec, ~3 RPM)
   - Lower pulse width = faster speed, higher pulse width = slower speed

### Speed Range Reference

**Speed Calculation:**
- Total time per step = 2 × pulseWidth (symmetric: HIGH for pulseWidth, LOW for pulseWidth)
- Steps per second = 1,000,000 / (2 × pulseWidth)
- RPM = (Steps per second × 60) / (stepsPerRevolution × microstepping × gearRatio)

**Speed Range Table (for 200 steps/rev motor, 1x microstepping, 1:1 gear ratio):**

| Pulse Width (μs) | Time/Step (μs) | Steps/sec | RPM | Full Rotation Time | Use Case |
|------------------|----------------|-----------|-----|-------------------|----------|
| 10,000 (min) | 20,000 | 50 | ~15 RPM | 4.0 sec | Fast |
| 15,000 (default) | 30,000 | ~33 | ~10 RPM | 6.0 sec | Medium (current default) |
| 25,000 | 50,000 | 20 | ~6 RPM | 10.0 sec | Slow |
| 50,000 (max) | 100,000 | 10 | ~3 RPM | 20.0 sec | Very slow |

**Typical working range:** 15,000-25,000μs pulse width (~33-20 steps/sec, ~10-6 RPM)

5. Test positioning accuracy:
   - Home system
   - Move to each compartment and verify position
   - Adjust containerPositionsInDegrees[] if needed
   - Adjust step pulse width if movement is too fast/slow or motor misses steps

6. Run full rotation timing calibration:
   - Press and hold Button 6 for 3+ seconds to start calibration
   - System will rotate 360° and measure actual timing
   - Check Serial Monitor for results:
     * Full rotation time (ms)
     * Time per degree (ms/degree)
     * Calculated time to each compartment
   - Use these values to verify step-based positioning matches mechanical timing
```

### Fine-tuning
- **Auto-homing**: Set `autoHomeAfterDispense = false` to disable auto-home after dispense
- **Container positions**: Edit `containerPositionsInDegrees[]` array for custom spacing
- **Servo range**: Adjust `servoMinMicroseconds` and `servoMaxMicroseconds` for servo limits (servo performs full arc sweep for dispensing)
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
| Wrong compartment reached | Calibrate stepper motor settings (steps per revolution, microstepping, gear ratio) or adjust positions in `containerPositionsInDegrees[]` array |
| Position drifts | Press Button 6 to re-home periodically |
| Pills not detected | Check D35 IR sensor, increase timeout |
| Stepper doesn't move | Check wiring D16/D17/D18, verify EN pin (LOW=enabled), adjust step pulse width |
| Stepper misses steps | Increase stepperMinStepPulseWidthMicroseconds (motor going too fast - pulse width too low) |
| Servo doesn't move | Check D4 connection, verify servoMinMicroseconds and servoMaxMicroseconds settings |

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

- **Button Reference** - See "Button Reference" section above in this file
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

**Version**: 2.0  
**Hardware**: ESP32 with 5-compartment rotating dispenser  
**Motor**: Stepper motor (calibrate steps per revolution, microstepping, gear ratio, and pulse timing)

