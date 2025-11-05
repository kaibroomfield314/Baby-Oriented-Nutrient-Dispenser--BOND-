# BOND - Baby-Oriented-Nutrient-Dispenser

A comprehensive smart pill dispensing system consisting of an ESP32-based hardware controller, an iOS mobile application, and a web-based desktop application. BOND provides automated medication management with Bluetooth Low Energy (BLE) control, scheduling, adherence tracking, and comprehensive health monitoring features across multiple platforms.

## Table of Contents

- [Project Overview](#project-overview)
- [Project Structure](#project-structure)
- [ESP32 Pill Dispenser System](#esp32-pill-dispenser-system)
- [iOS Application](#ios-application)
- [Web Application](#web-application)
- [Test Code](#test-code)
- [Hardware Requirements](#hardware-requirements)
- [Getting Started](#getting-started)
- [Documentation](#documentation)
- [Troubleshooting](#troubleshooting)
- [Development](#development)

---

## Project Overview

BOND is a complete medication management system designed to help users (especially babies and children) manage their medication schedules safely and effectively. The system consists of:

### Key Features

- **5-Compartment Rotary Dispenser**: Automated rotating pill storage with precise positioning
- **Multi-Platform Control**: iOS mobile app and web-based desktop application
- **Bluetooth Low Energy Control**: Wireless control and monitoring via mobile and web apps
- **Automated Dispensing**: Servo-controlled dispensing mechanism with IR pill detection
- **Smart Scheduling**: Medication schedules with automatic reminders
- **Adherence Tracking**: Monitor medication compliance with detailed statistics
- **Health Monitoring**: Track side effects, symptoms, and appointments
- **Manual Controls**: Physical buttons and LCD display for standalone operation

### Technology Stack

- **Hardware**: ESP32 microcontroller with stepper motor, servo, electromagnet, and sensors
- **Firmware**: Arduino/ESP32 with modular C++ architecture
- **Mobile App**: Native iOS app built with SwiftUI
- **Web App**: Single-page web application (HTML/CSS/JavaScript)
- **Communication**: Bluetooth Low Energy (BLE) protocol

---

## Project Structure

```
Baby-Oriented-Nutrient-Dispenser--BOND/
│
├── 1.Pill_Dispenser_ESP32/          # ESP32 firmware and hardware control
│   ├── 1.Pill_Dispenser_ESP32.ino   # Main Arduino sketch
│   ├── Config.h                     # Pin definitions and constants
│   ├── ConfigurationSettings.h      # Tunable system parameters
│   ├── BLEManager.h                 # Bluetooth Low Energy communication
│   ├── DispenserController.h        # High-level dispensing operations
│   ├── HardwareController.h         # Motor, servo, and actuator control
│   ├── SensorManager.h              # Sensor input and interrupt handling
│   ├── UIManager.h                  # LCD display and button interface
│   ├── README.md                    # ESP32 system documentation
│   ├── ARCHITECTURE.md              # Code architecture and design patterns
│   └── POSITIONING_SYSTEM_EXPLAINED.md  # Positioning system mathematics
│
├── BOND - SWIFT iOS/                # iOS mobile application
│   └── BOND-APP-IOS/               
│       ├── BOND_APP_IOSApp.swift    # App entry point
│       ├── Managers/                # Business logic managers
│       │   ├── BluetoothManager.swift
│       │   ├── PillManager.swift
│       │   ├── ScheduleManager.swift
│       │   ├── AppointmentManager.swift
│       │   ├── SideEffectManager.swift
│       │   ├── SymptomManager.swift
│       │   └── UserManager.swift
│       ├── Models/                  # Data models
│       │   ├── Pill.swift
│       │   ├── PillSchedule.swift
│       │   ├── User.swift
│       │   ├── Appointment.swift
│       │   ├── SideEffect.swift
│       │   └── Symptom.swift
│       ├── Views/                   # SwiftUI views
│       │   ├── Root/ContentView.swift
│       │   ├── Dispense/DispenseView.swift
│       │   ├── Pills/
│       │   │   ├── PillLibraryView.swift
│       │   │   ├── AddPillView.swift
│       │   │   └── EditPillView.swift
│       │   ├── Schedule/
│       │   │   ├── ScheduleView.swift
│       │   │   ├── AddScheduleView.swift
│       │   │   ├── EditScheduleView.swift
│       │   │   └── ScheduleRow.swift
│       │   ├── Adherence/AdherenceView.swift
│       │   ├── Appointments/AppointmentsView.swift
│       │   ├── SideEffects/SideEffectsView.swift
│       │   ├── Symptoms/SymptomsView.swift
│       │   ├── Connection/ConnectionView.swift
│       │   └── Settings/SettingsView.swift
│       ├── Utilities/               # Helper utilities
│       │   ├── ColorHelper.swift
│       │   ├── Logger.swift
│       │   └── Theme.swift
│       ├── Assets.xcassets/         # App icons and assets
│       └── Info.plist               # App configuration and permissions
│   ├── BOND-APP-IOS.xcodeproj/      # Xcode project file
│   ├── BOND-APP-IOSTests/           # Unit tests
│   └── BOND-APP-IOSUITests/         # UI tests
│
├── Test Code/                       # Hardware component test sketches
│   ├── LED_test/                    # LED functionality test
│   ├── Pill_Dispenser_Test/        # Complete dispenser test
│   ├── Test_Code_Arduino/           # Arduino-specific tests
│   ├── Test_Code_ESP32/             # ESP32 component tests
│   │   ├── Button_test/
│   │   ├── DC_motor_test/
│   │   ├── LCD_test/
│   │   ├── LED_test/
│   │   ├── Magnet_test/
│   │   ├── servo_test/
│   │   └── libraries/ESP32Servo/    # ESP32Servo library
│   └── Test_Code_ESP32_updated_with_Stepper_motor/
│       ├── Stepper_motor_and_servo/
│       └── Stepper_motor_single_rotation/
│
├── web/                            # Web-based desktop application
│   └── index.html                  # Single-page web app (HTML/CSS/JavaScript)
│
├── Pin Out Pill Dispenser_20251101.xlsx  # Hardware wiring documentation
│
└── README.md                        # This file

---

## ESP32 Pill Dispenser System

### System Overview

The ESP32-based pill dispenser is a 5-compartment rotating system with automatic homing, precise positioning, and multi-attempt pill dispensing with detection.

**Key Features:**
- Automatic homing on startup
- Step-based absolute positioning (5 compartments at 72° intervals)
- Multi-attempt dispensing (up to 3 retries per pill)
- IR sensor-based pill detection
- Physical button interface (7 buttons)
- LCD display for status and feedback
- Bluetooth Low Energy control
- Encoder-based position tracking

### Hardware Components

| Component | Function | Pin/Details |
|-----------|----------|-------------|
| **Stepper Motor** | Rotates dispenser plate | DIR: D16, STEP: D17, EN: D18 |
| **Servo Motor** | Dispensing mechanism | Signal: D4 |
| **Electromagnet** | Pill pickup mechanism | Control: D15 |
| **IR Sensor** | Pill detection | D35 |
| **Home Switch** | Homing position sensor | D12 (LOW when pressed) |
| **Encoder** | Position tracking | Ch1: VN(39), Ch2: D34 |
| **LCD Display** | Status display | 16x2 character LCD (multiple pins) |
| **Buttons** | Manual control | 7 buttons (D13-D14, D25-D27, D33, VP) |
| **Status LED** | System status | D2 |

### Architecture

The ESP32 firmware uses a modular architecture with clear separation of concerns:

```
Main Sketch (.ino)
    ↓
┌─────────────────────────────────────┐
│ Business Logic Layer                │
│  - DispenserController              │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Hardware Abstraction Layer        │
│  - HardwareController (outputs)   │
│  - SensorManager (inputs)          │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Communication & Presentation       │
│  - BLEManager                       │
│  - UIManager                        │
└─────────────────────────────────────┘
```

**Module Responsibilities:**
- **Config.h**: Pin definitions and BLE UUIDs
- **ConfigurationSettings.h**: Tunable parameters (motor speeds, timeouts, positions)
- **SensorManager**: Reads home switch, IR sensor, encoder
- **HardwareController**: Controls motor, servo, electromagnet, LED
- **DispenserController**: High-level operations (homing, movement, dispensing)
- **BLEManager**: Bluetooth communication and command parsing
- **UIManager**: LCD display and button debouncing

See `1.Pill_Dispenser_ESP32/ARCHITECTURE.md` for detailed architecture documentation.

### Button Reference

| Button | Pin | Function |
|--------|-----|----------|
| 1 | D13 | Select Compartment 1 |
| 2 | D14 | Select Compartment 2 |
| 3 | D27 | Select Compartment 3 |
| 4 | VP | Select Compartment 4 |
| 5 | D33 | Select Compartment 5 |
| **6** | **D25** | **Trigger Homing** (1s debounce) |
| **7** | **D26** | **Dispense Pill** |

### Quick Start

1. **Upload Code**
   - Open `1.Pill_Dispenser_ESP32/1.Pill_Dispenser_ESP32.ino` in Arduino IDE
   - All `.h` files are included automatically
   - Select ESP32 board and port
   - Click Upload

2. **First Run**
   - System auto-homes on startup
   - Green LED turns on when ready
   - LCD shows "Slot: 1 Ready"
   - BLE advertises as "PillDispenser"

3. **Basic Operation**
   - **Select compartment**: Press Button 1-5
   - **Dispense pill**: Press Button 7
   - **Re-home**: Press Button 6 (short press)
   - **Calibration**: Press and hold Button 6 for 3+ seconds

### Configuration & Calibration

**⚠️ REQUIRED CALIBRATION**: Edit `ConfigurationSettings.h` before first use.

#### Stepper Motor Configuration

```cpp
// Mechanical setup (adjust for your hardware)
int stepperStepsPerRevolution = 200;        // 200 for 1.8° motor, 400 for 0.9°
int stepperMicrostepping = 1;               // Driver setting: 1, 2, 4, 8, 16
float stepperGearRatio = 1.0;               // Gear reduction (1.0 if none)

// Speed control (symmetric pulse timing)
int stepperStepPulseWidthMicroseconds = 15000;   // Pulse width (μs)
// Lower = faster, Higher = slower
// Typical range: 15,000-25,000μs
```

#### Container Positions

```cpp
// Container positions (degrees from home)
float containerPositionsInDegrees[5] = {
    0.0,      // Container 1: at home
    72.0,     // Container 2: 72° from home
    144.0,    // Container 3: 144° from home
    216.0,    // Container 4: 216° from home
    288.0     // Container 5: 288° from home
};
```

#### Calibration Steps

1. Set stepper motor parameters (steps per revolution, microstepping, gear ratio)
2. Calibrate step pulse width timing:
   - Start with 15,000μs (default)
   - Test minimum: reduce until motor misses steps
   - Set safety limits in `stepperMinStepPulseWidthMicroseconds`
3. Test positioning accuracy:
   - Home system
   - Move to each compartment and verify position
   - Adjust `containerPositionsInDegrees[]` if needed
4. Run full rotation timing calibration:
   - Press and hold Button 6 for 3+ seconds
   - Check Serial Monitor for timing results

See `1.Pill_Dispenser_ESP32/README.md` for detailed calibration instructions.

### BLE Command Protocol

The ESP32 accepts the following BLE commands:

| Command | Format | Description |
|---------|--------|-------------|
| `HOME` | `HOME` | Trigger homing sequence |
| `DISPENSE` | `DISPENSE:<compartment>:<count>` | Dispense pills (e.g., `DISPENSE:3:1`) |
| `STATUS` | `STATUS` | Get dispense statistics |
| `RESET` | `RESET` | Reset counters |

**BLE Service UUID**: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`  
**BLE Characteristic UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`  
**Device Name**: `PillDispenser`

### Operation Flow

```
Power On
    ↓
5-Second Switch Diagnostic
    ↓
Auto-Homing Sequence
    ↓
System Ready
    ↓
Select Compartment (Buttons 1-5 or BLE)
    ↓
Plate Rotates to Position
    ↓
Dispense (Button 7 or BLE)
    ↓
Electromagnet → Servo → IR Detect → Success/Retry
    ↓
Auto-Home After Dispense (if enabled)
    ↓
System Ready
```

### Documentation

- **Main Documentation**: `1.Pill_Dispenser_ESP32/README.md`
- **Architecture**: `1.Pill_Dispenser_ESP32/ARCHITECTURE.md`
- **Positioning System**: `1.Pill_Dispenser_ESP32/POSITIONING_SYSTEM_EXPLAINED.md`

---

## iOS Application

### Overview

The BOND iOS app is a comprehensive medication management application built with SwiftUI. It provides a complete interface for controlling the pill dispenser, managing medications, tracking adherence, and monitoring health.

### Features

The app consists of 9 main tabs:

1. **Dispense** - Direct pill dispensing control
2. **Pills** - Pill library management (add, edit, view pills)
3. **Adherence** - Medication compliance tracking and statistics
4. **Schedule** - Medication scheduling and reminders
5. **Appointments** - Medical appointment tracking
6. **Side Effects** - Track medication side effects
7. **Symptoms** - Health symptom monitoring
8. **Connect** - Bluetooth connection management
9. **Settings** - User preferences and configuration

### Architecture

The iOS app follows an MVVM (Model-View-ViewModel) pattern with manager classes:

**Managers** (Business Logic):
- `BluetoothManager` - BLE communication with ESP32
- `PillManager` - Pill data management
- `ScheduleManager` - Medication scheduling
- `AppointmentManager` - Appointment tracking
- `SideEffectManager` - Side effect logging
- `SymptomManager` - Symptom tracking
- `UserManager` - User and preferences management

**Models** (Data Structures):
- `Pill` - Medication information (name, color, compartment, dosage)
- `PillSchedule` - Scheduled medication times
- `DoseRecord` - Individual dose tracking
- `MedicationStatistics` - Adherence statistics
- `User` - User profile and roles (patient, caregiver, admin)
- `Appointment` - Medical appointments
- `SideEffect` - Side effect records
- `Symptom` - Symptom records

**Views** (SwiftUI):
- Tab-based navigation with 9 main views
- Forms for adding/editing pills, schedules, appointments
- Charts and statistics for adherence tracking
- Connection management interface

### Setup & Build Instructions

**Requirements:**
- Xcode 15.0 or later
- iOS 18.6 or later
- macOS with Apple Silicon or Intel
- Bluetooth-enabled iOS device for testing

**Build Steps:**

1. Open `BOND - SWIFT iOS/BOND-APP-IOS.xcodeproj` in Xcode
2. Select your development team in Signing & Capabilities
3. Connect an iOS device or use simulator (BLE requires physical device)
4. Build and run (⌘R)

**Bluetooth Permissions:**
The app requires Bluetooth permissions (configured in `Info.plist`):
- `NSBluetoothAlwaysUsageDescription`: "This app needs Bluetooth to connect to your pill dispenser"
- `NSBluetoothPeripheralUsageDescription`: "This app uses Bluetooth to control your pill dispenser"

### Data Storage

The app uses `UserDefaults` for local data persistence:
- Pills, schedules, and user data are stored locally
- Data persists between app launches
- No cloud sync (can be added as future enhancement)

### BLE Integration

The `BluetoothManager` handles all communication with the ESP32:
- Automatic device discovery and connection
- Command sending (`HOME`, `DISPENSE`, `STATUS`, `RESET`)
- Response parsing and error handling
- Connection state management
- Auto-reconnection support

**Connection Flow:**
1. Scan for "PillDispenser" device
2. Connect to discovered device
3. Discover services and characteristics
4. Send commands and receive responses
5. Handle disconnections and reconnection

---

## Web Application

### Overview

The BOND web application is a desktop-friendly single-page application (SPA) that provides a comprehensive interface for managing medications, schedules, and health data. It offers similar functionality to the iOS app but is optimized for desktop browsers and larger screens.

### Features

The web app includes 8 main sections:

1. **Dashboard** - Overview statistics and today's schedule
   - Total pills tracking
   - Today's adherence percentage
   - Weekly adherence average
   - Upcoming pills in next 24 hours
   - Today's schedule display

2. **Pills** - Pill library management
   - Add, edit, and delete pills
   - Pill details (name, color, compartment, dosage, notes)
   - Visual pill color coding

3. **Schedule** - Medication scheduling
   - Weekly schedule view (Monday-Sunday)
   - Daily medication times
   - Schedule management per pill

4. **Appointments** - Medical appointment tracking
   - Add appointments with doctor, specialty, date/time, location
   - View and manage upcoming appointments
   - Appointment notes

5. **Adherence** - Medication compliance tracking
   - Monthly calendar view with adherence status
   - Daily adherence indicators (taken/missed/pending)
   - Statistics table per pill
   - Total scheduled, taken, missed counts
   - Adherence percentage calculations

6. **Symptoms** - Health symptom logging
   - Log symptoms with date/time and severity
   - Symptom tracking and history
   - Notes for each symptom entry

7. **Side Effects** - Medication side effect tracking
   - Log side effects with date/time
   - Track side effects per medication
   - Notes and severity tracking

8. **Settings** - User preferences and device management
   - User profile (name, email, date of birth)
   - Notification preferences
   - Bluetooth device connection status
   - Data management (clear all data)

### Technical Details

**Technology:**
- Single HTML file with embedded CSS and JavaScript
- No external dependencies (vanilla JavaScript)
- LocalStorage for data persistence
- Responsive design with modern CSS
- No build process required

**Browser Compatibility:**
- Modern browsers (Chrome, Firefox, Safari, Edge)
- Requires JavaScript enabled
- Best viewed on desktop/laptop screens
- Responsive design adapts to different screen sizes

### Usage

1. **Open the Application**
   - Open `web/index.html` in any modern web browser
   - No server required - works as a local file
   - Can be hosted on any web server for remote access

2. **Data Storage**
   - All data is stored in browser's LocalStorage
   - Data persists between browser sessions
   - Data is browser-specific (not synced across devices)
   - Use "Clear All Data" in Settings to reset

3. **Device Connection**
   - Bluetooth device connection status shown in header
   - Device connection managed in Settings section
   - Future implementation: Web Bluetooth API integration for direct ESP32 control

4. **Navigation**
   - Click sidebar menu items to navigate between sections
   - Active section highlighted in sidebar
   - Dashboard is the default landing page

### Features in Detail

**Dashboard:**
- Real-time statistics cards
- Quick access to today's schedule
- Visual adherence indicators
- Upcoming medication alerts

**Pills Management:**
- Color-coded pill library
- Compartment assignment
- Dosage and notes tracking
- Quick add/edit interface

**Schedule:**
- Weekly grid view
- Visual schedule representation
- Easy schedule modification
- Time-based organization

**Adherence Tracking:**
- Calendar-based visual tracking
- Color-coded status (green=taken, red=missed, yellow=pending)
- Monthly statistics
- Per-pill adherence percentages

**Data Management:**
- LocalStorage persistence
- Export/import capability (future enhancement)
- Data clearing functionality
- User preferences storage

### Advantages Over Mobile App

- **Larger Screen**: Better for viewing schedules and statistics
- **Keyboard Input**: Faster data entry for multiple items
- **No Installation**: Works directly in browser
- **Cross-Platform**: Works on Windows, macOS, Linux
- **Desktop Workflow**: Better for desktop-based workflows

### Future Enhancements

- Web Bluetooth API integration for direct ESP32 control
- Cloud synchronization
- Data export/import (JSON/CSV)
- Multi-user support
- Advanced analytics and reporting
- Medication interaction warnings
- Print-friendly views

---

## Test Code

The `Test Code/` directory contains individual component test sketches for hardware validation and debugging.

### Organization

**Test_Code_ESP32/** - ESP32-specific component tests:
- `Button_test/` - Button input testing
- `DC_motor_test/` - DC motor control
- `LCD_test/` - LCD display functionality
- `LED_test/` - LED indicator testing
- `Magnet_test/` - Electromagnet control
- `servo_test/` - Servo motor positioning
- `libraries/ESP32Servo/` - ESP32Servo library (required for servo control)

**Test_Code_ESP32_updated_with_Stepper_motor/** - Stepper motor tests:
- `Stepper_motor_single_rotation/` - Single rotation test
- `Stepper_motor_and_servo/` - Combined stepper and servo test

**Test_Code_Arduino/** - Arduino-specific tests (for reference)

**LED_test/** - Basic LED test (generic)

**Pill_Dispenser_Test/** - Complete system integration test

### Usage

1. **Individual Component Testing**: Upload individual test sketches to verify hardware functionality
2. **Hardware Debugging**: Use test code to isolate issues with specific components
3. **Calibration**: Use stepper motor tests to calibrate timing and positioning
4. **Integration Testing**: Use `Pill_Dispenser_Test` for full system validation

### ESP32Servo Library

The ESP32Servo library is included in `Test Code/Test_Code_ESP32/libraries/ESP32Servo/`. This library is required for servo motor control on ESP32. Copy this library to your Arduino libraries folder if needed for the main project.

---

## Hardware Requirements

### Components List

| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 Development Board | 1 | Any ESP32 variant (ESP32-WROOM, ESP32-DevKitC, etc.) |
| Stepper Motor | 1 | NEMA 17 or similar (200 steps/rev recommended) |
| Stepper Motor Driver | 1 | DRV8825, A4988, or similar |
| Servo Motor | 1 | Standard hobby servo (SG90 or similar) |
| Electromagnet | 1 | 12V electromagnet with relay/transistor |
| IR Sensor | 1 | IR proximity sensor (digital output) |
| Limit Switch | 1 | Home position switch (normally open) |
| Rotary Encoder | 1 | 2-channel quadrature encoder (optional, for position tracking) |
| LCD Display | 1 | 16x2 character LCD with I2C or parallel interface |
| Buttons | 7 | Tactile push buttons |
| LEDs | 1+ | Status indicators |
| Power Supply | 1 | 12V for stepper motor, 5V for ESP32/servo |
| Resistors | Various | Pull-up/pull-down resistors as needed |
| Breadboard/PCB | 1 | For prototyping or final assembly |

### Wiring Reference

See `Pin Out Pill Dispenser_20251101.xlsx` for detailed wiring diagrams and pin assignments.

### Power Requirements

- **ESP32**: 5V (via USB or external supply)
- **Stepper Motor**: 12V (check motor specifications)
- **Servo**: 5V (can share with ESP32 if current allows)
- **Electromagnet**: 12V (may need separate supply depending on current draw)
- **LCD**: 5V (can share with ESP32)

**Important**: Ensure adequate current capacity for all components. Stepper motor and electromagnet may require significant current.

### Physical Assembly

1. Mount stepper motor securely to dispenser base
2. Attach rotating plate to stepper motor shaft
3. Position 5 compartments evenly around plate (72° spacing)
4. Mount home switch at home position
5. Position IR sensor for pill detection
6. Mount servo and electromagnet for dispensing mechanism
7. Secure ESP32 and all electronics in enclosure
8. Route all wires securely

---

## Getting Started

### Initial Setup

#### 1. Hardware Setup

1. Assemble hardware according to wiring diagram
2. Connect all components to ESP32 as specified in `Config.h`
3. Verify power supply connections
4. Double-check all wiring before powering on

#### 2. ESP32 Firmware Setup

1. Install Arduino IDE with ESP32 board support
2. Install required libraries:
   - ESP32Servo (included in Test Code)
   - LiquidCrystal (for LCD display)
3. Open `1.Pill_Dispenser_ESP32/1.Pill_Dispenser_ESP32.ino`
4. Configure `ConfigurationSettings.h`:
   - Set stepper motor parameters
   - Adjust container positions if needed
   - Set timing values
5. Upload to ESP32
6. Open Serial Monitor (115200 baud) to view initialization

#### 3. First-Time Calibration

1. Power on ESP32
2. System will auto-home (watch for LED/green status)
3. Test each compartment:
   - Press Button 1-5 to select compartments
   - Verify plate rotates to correct position
   - Adjust `containerPositionsInDegrees[]` if needed
4. Test dispensing:
   - Select compartment
   - Press Button 7 to dispense
   - Verify pill detection works
5. Run timing calibration:
   - Press and hold Button 6 for 3+ seconds
   - Review Serial Monitor output
   - Adjust timing if needed

#### 4. iOS App Setup

1. Open `BOND - SWIFT iOS/BOND-APP-IOS.xcodeproj` in Xcode
2. Configure signing (select your development team)
3. Build and deploy to iOS device
4. Grant Bluetooth permissions when prompted

#### 5. Pairing App with Dispenser

**iOS App:**
1. Open BOND app on iOS device
2. Navigate to "Connect" tab
3. Ensure ESP32 is powered on and advertising as "PillDispenser"
4. Tap "Scan for Devices"
5. Select "PillDispenser" from discovered devices
6. Wait for connection (status will show "Connected")
7. Test connection by sending a `HOME` command

**Web App:**
1. Open `web/index.html` in a web browser
2. Navigate to Settings section
3. Check device connection status
4. (Web Bluetooth integration coming in future update)

### Basic Usage

#### Manual Operation (Buttons)

1. **Select Compartment**: Press Button 1-5
2. **Dispense**: Press Button 7
3. **Re-home**: Press Button 6 (short press)
4. **View Status**: Check LCD display

#### App Operation

1. **Dispense Pills**:
   - Open "Dispense" tab
   - Select compartment
   - Enter pill count
   - Tap "Dispense"

2. **Manage Pills**:
   - Open "Pills" tab
   - Add new pills with name, color, compartment
   - Edit existing pills
   - View pill library

3. **Create Schedule**:
   - Open "Schedule" tab
   - Tap "Add Schedule"
   - Set time, frequency, pill count
   - Enable notifications

4. **Track Adherence**:
   - Open "Adherence" tab
   - View statistics and charts
   - Check compliance percentage

#### Web App Operation

1. **Open Application**:
   - Open `web/index.html` in any modern web browser
   - No installation required

2. **Manage Pills**:
   - Click "Pills" in sidebar
   - Click "+ Add Pill" button
   - Fill in pill details and save

3. **Create Schedule**:
   - Click "Schedule" in sidebar
   - View weekly schedule grid
   - Add times for each day

4. **Track Adherence**:
   - Click "Adherence" in sidebar
   - View monthly calendar with adherence indicators
   - Review statistics table

5. **Log Symptoms/Side Effects**:
   - Navigate to respective sections
   - Click "Log Symptom" or "Log Side Effect"
   - Fill in details and save

---

## Documentation

### ESP32 Documentation

- **Main README**: `1.Pill_Dispenser_ESP32/README.md`
  - Complete system overview
  - Button reference
  - Pin assignments
  - Calibration guide
  - BLE commands
  - Troubleshooting

- **Architecture**: `1.Pill_Dispenser_ESP32/ARCHITECTURE.md`
  - Module dependency graph
  - Data flow diagrams
  - Design patterns
  - SOLID principles
  - Extensibility examples

- **Positioning System**: `1.Pill_Dispenser_ESP32/POSITIONING_SYSTEM_EXPLAINED.md`
  - Step-based positioning mathematics
  - Position calculation examples
  - Movement algorithms
  - Calibration procedures

### Hardware Documentation

- **Wiring Diagram**: `Pin Out Pill Dispenser_20251101.xlsx`
  - Complete pin assignments
  - Wiring connections
  - Component specifications

### Code Documentation

- **ESP32 Code**: Well-commented header files with inline documentation
- **iOS Code**: Swift documentation comments in managers and models

---

## Troubleshooting

### ESP32 Issues

#### Homing Fails
- **Symptom**: System doesn't home on startup
- **Solutions**:
  - Check D12 home switch wiring (should read LOW when pressed)
  - Verify switch is positioned correctly
  - Check switch continuity with multimeter
  - Review Serial Monitor for error messages

#### Wrong Compartment Reached
- **Symptom**: System rotates to wrong position
- **Solutions**:
  - Recalibrate stepper motor settings (steps per revolution, microstepping, gear ratio)
  - Adjust `containerPositionsInDegrees[]` array
  - Verify encoder connections if using encoder tracking
  - Run timing calibration (hold Button 6 for 3+ seconds)

#### Position Drifts Over Time
- **Symptom**: Accuracy decreases after multiple operations
- **Solutions**:
  - Press Button 6 to re-home periodically
  - Check for mechanical slippage in motor coupling
  - Verify encoder is functioning correctly
  - Increase step pulse width if motor is missing steps

#### Pills Not Detected
- **Symptom**: IR sensor doesn't detect pills
- **Solutions**:
  - Check D35 IR sensor wiring
  - Verify sensor is positioned correctly
  - Increase `pillDetectionTimeoutMilliseconds` in ConfigurationSettings.h
  - Test IR sensor with test code
  - Check sensor output with Serial Monitor

#### Stepper Motor Doesn't Move
- **Symptom**: Motor doesn't rotate
- **Solutions**:
  - Check wiring (DIR: D16, STEP: D17, EN: D18)
  - Verify EN pin is LOW (enabled)
  - Check power supply (12V for motor)
  - Verify driver connections
  - Test with stepper motor test code

#### Stepper Motor Misses Steps
- **Symptom**: Motor moves but position is inaccurate
- **Solutions**:
  - Increase `stepperMinStepPulseWidthMicroseconds` (motor too fast)
  - Check power supply current capacity
  - Verify motor driver current settings
  - Reduce speed (increase pulse width)

#### BLE Not Connecting
- **Symptom**: iOS app can't find or connect to device
- **Solutions**:
  - Verify ESP32 is powered on
  - Check Serial Monitor for "BLE Started" message
  - Ensure device name is "PillDispenser"
  - Reset ESP32 and try again
  - Check Bluetooth permissions in iOS app

### iOS App Issues

#### Can't Find Device
- **Symptom**: Device doesn't appear in scan
- **Solutions**:
  - Ensure ESP32 is powered on and advertising
  - Check Bluetooth is enabled on iOS device
  - Move closer to ESP32 (BLE range is limited)
  - Restart Bluetooth on iOS device
  - Restart app

#### Connection Drops
- **Symptom**: Connection lost during use
- **Solutions**:
  - Check distance between devices (stay within 10m)
  - Verify ESP32 power supply is stable
  - Check for interference (other BLE devices)
  - App should auto-reconnect (check Connection tab)

#### Commands Not Working
- **Symptom**: Commands sent but no response
- **Solutions**:
  - Verify connection status in app
  - Check Serial Monitor on ESP32 for received commands
  - Ensure correct command format (see BLE Command Protocol)
  - Try sending `HOME` command first
  - Reset ESP32 and reconnect

#### App Crashes
- **Symptom**: App crashes on launch or during use
- **Solutions**:
  - Check iOS version (requires 18.6+)
  - Update Xcode and rebuild
  - Clear app data and reinstall
  - Check Xcode console for error messages
  - Review crash logs in Xcode

### General Issues

#### System Resets Unexpectedly
- **Symptom**: ESP32 restarts during operation
- **Solutions**:
  - Check power supply stability and capacity
  - Verify all connections are secure
  - Check for short circuits
  - Monitor Serial Monitor for error messages
  - Reduce current draw (disconnect non-essential components)

#### LCD Display Issues
- **Symptom**: LCD doesn't show text or shows garbage
- **Solutions**:
  - Check LCD wiring (verify all pins)
  - Adjust contrast potentiometer
  - Verify I2C address if using I2C LCD
  - Test with LCD test code
  - Check power supply to LCD

---

## Development

### Project Status

✅ **Operational** - All core features implemented and tested

**Completed Features:**
- ✅ ESP32 firmware with modular architecture
- ✅ 5-compartment rotary dispenser control
- ✅ Automatic homing and positioning
- ✅ Multi-attempt pill dispensing with detection
- ✅ BLE communication protocol
- ✅ LCD display and button interface
- ✅ iOS app with 9 main features
- ✅ Web application with 8 main features
- ✅ Pill management and scheduling
- ✅ Adherence tracking
- ✅ Health monitoring (side effects, symptoms, appointments)

**Future Enhancements:**
- Cloud synchronization for schedules and data
- Multi-user support with caregiver access
- Advanced analytics and reporting
- Voice control integration
- WiFi connectivity option
- Web dashboard for remote monitoring
- Medication interaction warnings
- Refill reminders based on usage

### Version Information

- **ESP32 Firmware**: Version 2.0
- **iOS App**: Version 1.0
- **Web App**: Version 1.0
- **Last Updated**: 2025

### Technology Stack

**ESP32 Firmware:**
- Arduino Framework
- ESP32 Board Support
- ESP32Servo Library
- LiquidCrystal Library
- BLE Libraries (ESP32 built-in)

**iOS Application:**
- Swift 5.0+
- SwiftUI Framework
- Combine Framework (reactive programming)
- CoreBluetooth Framework
- iOS 18.6+ Deployment Target

**Web Application:**
- HTML5
- CSS3 (Modern CSS with CSS Variables)
- Vanilla JavaScript (ES6+)
- LocalStorage API
- No external dependencies

### Contributing

This is a student project for ENG40011 - Engineering Technology Innovation Project. For questions or contributions, please contact the project team.

### License

This project is developed for educational purposes as part of an engineering technology innovation project.

---

## Quick Reference

### ESP32 Pin Assignments

| Component | Pin | Function |
|-----------|-----|----------|
| Stepper DIR | D16 | Direction control |
| Stepper STEP | D17 | Step pulse |
| Stepper EN | D18 | Enable (LOW=enabled) |
| Servo | D4 | PWM signal |
| Electromagnet | D15 | Control |
| Home Switch | D12 | Limit switch |
| IR Sensor | D35 | Pill detection |
| Encoder Ch1 | VN(39) | Position tracking |
| Encoder Ch2 | D34 | Position tracking |
| Button 1 | D13 | Compartment 1 |
| Button 2 | D14 | Compartment 2 |
| Button 3 | D27 | Compartment 3 |
| Button 4 | VP | Compartment 4 |
| Button 5 | D33 | Compartment 5 |
| Button 6 | D25 | Homing trigger |
| Button 7 | D26 | Dispense |
| Status LED | D2 | System status |

### BLE Commands Quick Reference

```
HOME                    → Trigger homing
DISPENSE:<slot>:<count> → Dispense pills (e.g., DISPENSE:3:1)
STATUS                  → Get statistics
RESET                   → Reset counters
```

### File Locations

- **ESP32 Main Code**: `1.Pill_Dispenser_ESP32/1.Pill_Dispenser_ESP32.ino`
- **Configuration**: `1.Pill_Dispenser_ESP32/ConfigurationSettings.h`
- **iOS Project**: `BOND - SWIFT iOS/BOND-APP-IOS.xcodeproj`
- **Web App**: `web/index.html`
- **Wiring Diagram**: `Pin Out Pill Dispenser_20251101.xlsx`

---

**For detailed documentation, see the README files in each subdirectory.**
