# System Architecture

## Module Dependency Graph

```
┌────────────────────────────────────────────────────────────────┐
│                     Main Arduino Sketch                        │
│                  1.Pill_Dispenser_ESP32.ino                    │
│                                                                │
│  • Orchestrates all modules                                    │
│  • Initializes system                                          │
│  • Routes BLE commands                                         │
│  • Handles button events                                       │
└────────┬──────────────┬──────────────┬──────────────┬──────────┘
         │              │              │              │
         │              │              │              │
         v              v              v              v
┌────────────────┐ ┌──────────────┐ ┌─────────────┐ ┌────────────────┐
│  BLEManager    │ │  UIManager   │ │Dispenser    │ │System          │
│                │ │              │ │Controller   │ │Configuration   │
│ • BLE server   │ │ • LCD display│ │             │ │                │
│ • Commands     │ │ • Buttons    │ │ • Homing    │ │ • Speeds       │
│ • Responses    │ │ • Debouncing │ │ • Movement  │ │ • Timeouts     │
│ • Connection   │ │ • Messages   │ │ • Dispensing│ │ • Angles       │
└────────────────┘ └──────────────┘ │ • Statistics│ │ • Attempts     │
                                    └──────┬──────┘ └────────────────┘
                                           │
                        ┌──────────────────┴────────────────────┐
                        │                                       │
                        v                                       v
              ┌───────────────────┐                  ┌─────────────────┐
              │Hardware           │                  │Sensor           │
              │Controller         │                  │Manager          │
              │                   │                  │                 │
              │ • Motor control   │                  │ • Home switch   │
              │ • Servo control   │                  │ • IR sensor     │
              │ • Electromagnet   │                  │ • Encoder       │
              │ • Status LED      │                  │ • Interrupts    │
              └─────────┬─────────┘                  └────────┬────────┘
                        │                                     │
                        └─────────────┬───────────────────────┘
                                      │
                                      v
                            ┌───────────────────┐
                            │     Config.h      │
                            │                   │
                            │ • Pin definitions │
                            │ • BLE UUIDs       │
                            │ • Constants       │
                            └───────────────────┘
```

---

## Data Flow

### **1. BLE Command Flow**

```
iOS/Android Device
      ↓
[BLE Command] → BLEManager.parseBLECommandAndExtractParameters()
      ↓
BLECommand struct (stored in BLEManager)
      ↓
Main loop checks: bleManager->hasNewCommandAvailableToProcess()
      ↓
Main loop: bleManager->getAndClearMostRecentCommand()
      ↓
Switch on command type
      ↓
Handler function (e.g., handleBLEDispenseCommand)
      ↓
DispenserController.dispensePillsFromCompartment()
      ↓
HardwareController + SensorManager (execute operation)
      ↓
BLEManager.sendDispenseResultToConnectedDevice() or sendErrorResponseToConnectedDevice()
      ↓
iOS/Android Device
```

### **2. Button Press Flow**

```
Physical Button Press
      ↓
UIManager.checkIfAnyButtonPressedWithDebounce()
      ↓
ButtonAction enum
      ↓
Main loop: handleButtonPress()
      ↓
If SELECT → handleManualDispenseRequest()
If other → UIManager.handleButtonActionAndUpdateSelection()
      ↓
DispenserController.dispensePillsFromCompartment()
      ↓
HardwareController + SensorManager (execute operation)
      ↓
UIManager.displaySuccessMessage() or displayFailureMessage()
```

### **3. Dispensing Operation Flow**

```
dispensePillsFromCompartment(compartment, count)
      ↓
moveRotaryDispenserToCompartmentNumber(compartment)
      ├→ HardwareController.runMotorAtNormalSpeed()
      ├→ delay (time-based movement)
      └→ HardwareController.stopMotorCompletely()
      ↓
For each pill:
  attemptToDispenseSinglePillWithRetries()
      ↓
  For each attempt (max 3):
    1. HardwareController.activateElectromagnetAndWaitForStabilization()
    2. HardwareController.moveServoToDispensingPositionAndWait()
    3. SensorManager.waitForPillDetectionWithTimeout()
    4. If detected: return true
    5. HardwareController.moveServoToRestPositionAndWait()
    6. HardwareController.deactivateElectromagnetWithDelay()
      ↓
Return success count
```

### **4. Homing Flow**

```
performHomingSequenceUntilSwitchActivated()
      ↓
HardwareController.runMotorAtHomingSpeed()
      ↓
SensorManager.waitForHomeSwitchActivationWithTimeout()
      ↓
HardwareController.stopMotorCompletely()
      ↓
SensorManager.resetEncoderPositionToZero()
      ↓
Return success/failure
```

---

## Module Responsibilities

### **Config.h** (Pure Configuration)
```
┌──────────────────────────────────────┐
│ No code, only #define statements     │
│                                      │
│ Dependencies: None                   │
│ Dependents: All other modules        │
└──────────────────────────────────────┘
```

### **ConfigurationSettings.h** (Pure Data)
```
┌──────────────────────────────────────┐
│ SystemConfiguration struct only      │
│                                      │
│ Dependencies: None                   │
│ Dependents: All controllers          │
└──────────────────────────────────────┘
```

### **SensorManager** (Input Layer)
```
┌──────────────────────────────────────┐
│ Reads: Home switch, IR sensor,       │
│        Encoder                       │
│                                      │
│ Dependencies: Config,                │
│               ConfigurationSettings  │
│ Dependents: DispenserController      │
│                                      │
│ Coupling: LOW (no outputs)           │
│ Cohesion: HIGH (all sensors)         │
└──────────────────────────────────────┘
```

### **HardwareController** (Output Layer)
```
┌──────────────────────────────────────┐
│ Controls: Motor, Servo,              │
│           Electromagnet, LED         │
│                                      │
│ Dependencies: Config,                │
│               ConfigurationSettings, │
│               ESP32Servo             │
│ Dependents: DispenserController      │
│                                      │
│ Coupling: LOW (no business logic)    │
│ Cohesion: HIGH (all actuators)       │
└──────────────────────────────────────┘
```

### **DispenserController** (Business Logic Layer)
```
┌──────────────────────────────────────┐
│ Operations: Homing, Movement,        │
│             Dispensing, Statistics   │
│                                      │
│ Dependencies: Config,                │
│               ConfigurationSettings, │
│               HardwareController,    │
│               SensorManager          │
│ Dependents: Main sketch              │
│                                      │
│ Coupling: MEDIUM (uses H/W + Sensors)│
│ Cohesion: HIGH (dispenser operations)│
└──────────────────────────────────────┘
```

### **BLEManager** (Communication Layer)
```
┌──────────────────────────────────────┐
│ Handles: BLE server, Commands,       │
│          Responses, Connection state │
│                                      │
│ Dependencies: Config,                │
│               ConfigurationSettings, │
│               BLE libraries          │
│ Dependents: Main sketch              │
│                                      │
│ Coupling: LOW (returns commands)     │
│ Cohesion: HIGH (all BLE operations)  │
└──────────────────────────────────────┘
```

### **UIManager** (Presentation Layer)
```
┌──────────────────────────────────────┐
│ Handles: LCD display, Button input,  │
│          Debouncing, Selection       │
│                                      │
│ Dependencies: Config,                │
│               ConfigurationSettings, │
│               LiquidCrystal          │
│ Dependents: Main sketch              │
│                                      │
│ Coupling: LOW (returns actions)      │
│ Cohesion: HIGH (all UI operations)   │
└──────────────────────────────────────┘
```

### **Main Sketch** (Orchestration Layer)
```
┌──────────────────────────────────────┐
│ Role: Glue code, minimal logic       │
│                                      │
│ Dependencies: All modules            │
│ Dependents: None (top level)         │
│                                      │
│ Coupling: HIGH (by design)           │
│ Cohesion: MEDIUM (coordination)      │
└──────────────────────────────────────┘
```

---

## Layered Architecture View

```
┌─────────────────────────────────────────────────────────┐
│                  Application Layer                       │
│  ┌────────────────────────────────────────────────┐    │
│  │  Main Arduino Sketch (.ino file)              │    │
│  │  • Initialization                               │    │
│  │  • Event routing                                │    │
│  │  • Handler functions                            │    │
│  └────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│                   Business Logic Layer                   │
│  ┌──────────────────┐                                   │
│  │ Dispenser        │  High-level operations             │
│  │ Controller       │  • Homing, Movement, Dispensing    │
│  │                  │  • Statistics, State management    │
│  └──────────────────┘                                   │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│              Communication & Presentation Layer          │
│  ┌─────────────┐        ┌──────────────┐               │
│  │ BLE         │        │ UI           │               │
│  │ Manager     │        │ Manager      │               │
│  │ • Commands  │        │ • LCD        │               │
│  │ • Responses │        │ • Buttons    │               │
│  └─────────────┘        └──────────────┘               │
└─────────────────────────────────────────────────────────┘
                          ↓
┌────────────────────────────────────────────────────────┐
│           Hardware Abstraction Layer                   │
│  ┌─────────────┐        ┌──────────────┐               │
│  │ Hardware    │        │ Sensor       │               │
│  │ Controller  │        │ Manager      │               │
│  │ • Motor     │        │ • Switches   │               │
│  │ • Servo     │        │ • IR Sensor  │               │
│  │ • Magnet    │        │ • Encoder    │               │
│  └─────────────┘        └──────────────┘               │
└────────────────────────────────────────────────────────┘
                          ↓
┌────────────────────────────────────────────────────────┐
│                   Configuration Layer                  │
│  ┌─────────────┐        ┌──────────────┐               │
│  │ Config.h    │        │Configuration │               │
│  │ • Pins      │        │Settings.h    │               │
│  │ • Constants │        │ • Parameters │               │
│  └─────────────┘        └──────────────┘               │
└────────────────────────────────────────────────────────┘
```

---

## Coupling & Cohesion Analysis

### **Coupling Matrix** (Lower is Better)

|                    | Config | Settings | Sensor | Hardware  | Dispenser | BLE  |  UI  | Main |
|--------------------|--------|----------|--------|-----------|-----------|------|-------|------|
| Config             | -      | ✅       | ✅     | ✅       | ✅       | ✅  | ✅    | ✅   |
| Settings           | ✅     | -        | ✅     | ✅       | ✅       | ✅  | ✅    | ✅   |
| SensorManager      | ✅     | ✅       | -      | ❌       | ❌       | ❌  | ❌    | ❌   |
| HardwareController | ✅     | ✅       | ❌     | -        | ❌       | ❌  | ❌    | ❌   |
| DispenserController| ✅     | ✅       | ✅     | ✅       | -        | ❌  | ❌    | ❌   |
| BLEManager         | ✅     | ✅       | ❌     | ❌       | ❌       | -   | ❌    | ❌   |
| UIManager          | ✅     | ✅       | ❌     | ❌       | ❌       | ❌  | -     | ❌   |
| Main Sketch        | ✅     | ✅       | ✅     | ✅       | ✅       | ✅  | ✅    | -    |

✅ = Depends on | ❌ = Independent

**Analysis**: 
- Configuration modules are depended on by all (expected)
- Sensor and Hardware are independent of each other (good!)
- BLE and UI are independent of business logic (excellent!)
- Only DispenserController orchestrates Hardware + Sensors
- Main sketch is only place where all modules meet

### **Cohesion Score** (Higher is Better)

| Module | Cohesion | Reason |
|--------|----------|--------|
| Config | ⭐⭐⭐⭐⭐ | Single purpose: pin definitions |
| ConfigurationSettings | ⭐⭐⭐⭐⭐ | Single purpose: tunable parameters |
| SensorManager | ⭐⭐⭐⭐⭐ | All sensor input in one place |
| HardwareController | ⭐⭐⭐⭐⭐ | All actuator control in one place |
| DispenserController | ⭐⭐⭐⭐⭐ | All dispensing operations in one place |
| BLEManager | ⭐⭐⭐⭐⭐ | All BLE operations in one place |
| UIManager | ⭐⭐⭐⭐⭐ | All UI operations in one place |
| Main Sketch | ⭐⭐⭐⭐ | Orchestration (naturally broader) |

---

## Design Patterns Used

### **1. Dependency Injection**
```cpp
DispenserController(SystemConfiguration* config,
                   HardwareController* hardware,
                   SensorManager* sensors)
```
Benefits: Testable, flexible, clear dependencies

### **2. Singleton Pattern** (for ISR access)
```cpp
SensorManager* globalSensorManagerInstance = nullptr;
BLEManager* globalBLEManagerInstance = nullptr;
```
Benefits: ISRs can access instance methods

### **3. Strategy Pattern** (via configuration)
```cpp
SystemConfiguration systemConfig;
// Behavior changes based on configuration values
```
Benefits: Runtime behavior modification

### **4. Observer Pattern** (implicit in event loop)
```cpp
// Main loop polls for events
if (bleManager->hasNewCommandAvailableToProcess()) { ... }
ButtonAction action = uiManager->checkIfAnyButtonPressedWithDebounce();
```
Benefits: Loose coupling between event sources and handlers

### **5. Facade Pattern**
```cpp
// DispenserController provides simple interface to complex operations
dispenserController->dispensePillsFromCompartment(3, 5);
// Hides: motor control, servo movement, magnet activation, sensor polling
```
Benefits: Simple high-level API

---

## Principles Followed

### **SOLID Principles**

✅ **S** - Single Responsibility Principle
   - Each module has one clear responsibility

✅ **O** - Open/Closed Principle
   - Extend behavior via configuration, not modification

✅ **L** - Liskov Substitution Principle
   - N/A (no inheritance used)

✅ **I** - Interface Segregation Principle
   - Modules expose only relevant methods

✅ **D** - Dependency Inversion Principle
   - High-level modules depend on abstractions (interfaces via pointers)

### **Additional Principles**

✅ **DRY** (Don't Repeat Yourself)
   - Common values in configuration
   - Reusable functions

✅ **KISS** (Keep It Simple, Stupid)
   - Simple, clear interfaces
   - No over-engineering

✅ **YAGNI** (You Aren't Gonna Need It)
   - Only implements required features
   - Extensible without bloat

✅ **Low Coupling**
   - Modules are independent
   - Minimal cross-dependencies

✅ **High Cohesion**
   - Related code grouped together
   - Clear module boundaries

---

## Testability

### **Unit Testing Possibilities**

```cpp
// Test SensorManager independently
void testSensorManager() {
    SystemConfiguration config;
    SensorManager sensors(&config);
    sensors.initializeAllSensors();
    
    bool homeActive = sensors.isHomePositionSwitchActivated();
    // Assert expected behavior
}

// Test HardwareController independently
void testHardwareController() {
    SystemConfiguration config;
    HardwareController hardware(&config);
    hardware.initializeAllHardwareActuators();
    
    hardware.runMotorAtHomingSpeed();
    delay(100);
    hardware.stopMotorCompletely();
    // Verify motor control
}

// Test DispenserController with mocks
void testDispenserController() {
    SystemConfiguration config;
    MockHardwareController mockHardware(&config);
    MockSensorManager mockSensors(&config);
    DispenserController dispenser(&config, &mockHardware, &mockSensors);
    
    bool success = dispenser.performHomingSequenceUntilSwitchActivated();
    // Assert expected behavior with mocks
}
```

---

## Extensibility Examples

### **Adding WiFi Support**

1. Create `WiFiManager.h` following same pattern
2. Add to main sketch:
```cpp
WiFiManager* wifiManager;
wifiManager = new WiFiManager(&systemConfig);
wifiManager->initializeWiFiConnection();
```
3. Handle WiFi events in main loop

### **Adding SD Card Logging**

1. Create `DataLogger.h`
2. Inject into DispenserController:
```cpp
DispenserController(SystemConfiguration* config,
                   HardwareController* hardware,
                   SensorManager* sensors,
                   DataLogger* logger)  // NEW
```
3. Log events in dispenser operations

### **Adding Web Server**

1. Create `WebServer.h`
2. Handle HTTP requests similar to BLE commands
3. Reuse existing DispenserController methods

---

**The architecture is production-ready, maintainable, and extensible! 🎉**

