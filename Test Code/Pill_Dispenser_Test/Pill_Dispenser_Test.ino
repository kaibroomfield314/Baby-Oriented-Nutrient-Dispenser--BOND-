/**
 * ============================================================================
 * ESP32 Pill Dispenser Test Code - 50 Pill Dispensing Test
 * ============================================================================
 * 
 * This test program performs a 50-pill dispensing test when button 7 is pressed.
 * 
 * Test Flow:
 * 1. Initialize system (sensors, hardware, homing)
 * 2. Wait for button 7 press
 * 3. Move to compartment 1 (home position)
 * 4. Dispense 50 pills in a row (one attempt per pill, no retries)
 * 5. Record and display results for each round
 * 
 * Results Format:
 * Round X: 1 pill dispensed, Y pills detected
 */

#include <Arduino.h>

// Include header files from main project
// Note: Adjust path if needed (e.g., "../1.Pill_Dispenser_ESP32/Config.h")
#include "../1.Pill_Dispenser_ESP32/Config.h"
#include "../1.Pill_Dispenser_ESP32/ConfigurationSettings.h"
#include "../1.Pill_Dispenser_ESP32/SensorManager.h"
#include "../1.Pill_Dispenser_ESP32/HardwareController.h"
#include "../1.Pill_Dispenser_ESP32/DispenserController.h"

// ============================================================================
// Global System Configuration and Module Instances
// ============================================================================

SystemConfiguration systemConfig;
SensorManager* sensorManager;
HardwareController* hardwareController;
DispenserController* dispenserController;

// ============================================================================
// Test State Variables
// ============================================================================

struct TestResult {
    int roundNumber;
    int pillsDispensed;  // Always 1 (single attempt)
    int pillsDetected;    // Count from IR sensor
};

TestResult testResults[50];  // Store results for 50 rounds
bool testInProgress = false;

// Button state tracking
unsigned long lastButtonCheckTime = 0;
bool lastButtonState = HIGH;
const unsigned long BUTTON_DEBOUNCE_MS = 200;

// ============================================================================
// Function Declarations
// ============================================================================

void setup();
void loop();
bool checkButton7Pressed();
void performTest();
int attemptToDispenseSinglePillOnce();
int countPillsDetected(unsigned long durationMs);
void printTestResults();
void printSummary();

// ============================================================================
// Setup Function
// ============================================================================

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000);  // Wait for serial to initialize
    
    Serial.println("===========================================");
    Serial.println("ESP32 Pill Dispenser Test Code");
    Serial.println("50-Pill Dispensing Test");
    Serial.println("===========================================");
    Serial.println();
    Serial.println("Press Button 7 (D26) to start the test...");
    Serial.println();
    
    // ========================================================================
    // Create module instances
    // ========================================================================
    
    sensorManager = new SensorManager(&systemConfig);
    hardwareController = new HardwareController(&systemConfig);
    dispenserController = new DispenserController(&systemConfig, hardwareController, sensorManager);
    
    // ========================================================================
    // Initialize sensors and hardware
    // ========================================================================
    
    sensorManager->initializeAllSensors();
    hardwareController->initializeAllHardwareActuators();
    dispenserController->initializeDispenserSystem();
    
    // ========================================================================
    // Attach sensor interrupts
    // ========================================================================
    
    globalSensorManagerInstance = sensorManager;
    
    attachInterrupt(digitalPinToInterrupt(PIN_FOR_ENCODER_CHANNEL_1), 
                    encoderInterruptServiceRoutine, 
                    CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_FOR_ENCODER_CHANNEL_2), 
                    encoderInterruptServiceRoutine, 
                    CHANGE);
    
    Serial.println("Interrupts attached");
    
    // ========================================================================
    // Configure button 7 pin
    // ========================================================================
    
    pinMode(PIN_FOR_NAVIGATION_SELECT_BUTTON, INPUT_PULLUP);
    lastButtonState = digitalRead(PIN_FOR_NAVIGATION_SELECT_BUTTON);
    
    // Hardware stabilization before homing
    delay(300);
    
    // ========================================================================
    // Perform homing sequence
    // ========================================================================
    
    Serial.println("========================================");
    Serial.println("Performing homing sequence...");
    Serial.println("========================================");
    
    bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
    
    if (homingSuccessful) {
        Serial.println("Homing completed successfully!");
    } else {
        Serial.println("WARNING: Homing failed! Test may not be accurate.");
    }
    
    Serial.println();
    Serial.println("===========================================");
    Serial.println("System Ready");
    Serial.println("Press Button 7 to start 50-pill test");
    Serial.println("===========================================");
    Serial.println();
}

// ============================================================================
// Main Loop Function
// ============================================================================

void loop() {
    // Check for button 7 press (only if test is not in progress)
    if (!testInProgress && checkButton7Pressed()) {
        Serial.println();
        Serial.println("========================================");
        Serial.println("BUTTON 7 PRESSED - Starting Test");
        Serial.println("========================================");
        Serial.println();
        
        testInProgress = true;
        performTest();
        testInProgress = false;
        
        Serial.println();
        Serial.println("===========================================");
        Serial.println("Test Complete - Press Button 7 to run again");
        Serial.println("===========================================");
        Serial.println();
    }
    
    delay(10);  // Small delay to prevent tight loop
}

// ============================================================================
// Button Detection Function
// ============================================================================

/**
 * Check if button 7 is pressed with debouncing
 * @return true if button was just pressed (edge detection)
 */
bool checkButton7Pressed() {
    unsigned long currentTime = millis();
    
    // Debounce check
    if (currentTime - lastButtonCheckTime < BUTTON_DEBOUNCE_MS) {
        return false;
    }
    
    bool currentButtonState = digitalRead(PIN_FOR_NAVIGATION_SELECT_BUTTON);
    
    // Edge detection: button pressed (transition from HIGH to LOW with pull-up)
    if (lastButtonState == HIGH && currentButtonState == LOW) {
        lastButtonState = currentButtonState;
        lastButtonCheckTime = currentTime;
        return true;
    }
    
    lastButtonState = currentButtonState;
    return false;
}

// ============================================================================
// Test Execution Function
// ============================================================================

/**
 * Perform the 50-pill dispensing test
 */
void performTest() {
    // Step 1: Move to compartment 1
    Serial.println("========================================");
    Serial.println("STEP 1: Moving to Compartment 1");
    Serial.println("========================================");
    
    bool moveSuccess = dispenserController->moveRotaryDispenserToCompartmentNumber(1);
    
    if (!moveSuccess) {
        Serial.println("ERROR: Failed to move to compartment 1!");
        return;
    }
    
    Serial.println("Successfully moved to compartment 1");
    Serial.println();
    
    // Step 2: Dispense 50 pills
    Serial.println("========================================");
    Serial.println("STEP 2: Dispensing 50 pills");
    Serial.println("========================================");
    Serial.println();
    
    for (int round = 1; round <= 50; round++) {
        Serial.print("Round ");
        Serial.print(round);
        Serial.print(": ");
        
        // Attempt to dispense one pill (single attempt, no retries)
        int pillsDetected = attemptToDispenseSinglePillOnce();
        
        // Record result
        testResults[round - 1].roundNumber = round;
        testResults[round - 1].pillsDispensed = 1;  // Always 1 (we attempt once)
        testResults[round - 1].pillsDetected = pillsDetected;
        
        // Print result for this round
        Serial.print("1 pill dispensed, ");
        Serial.print(pillsDetected);
        Serial.println(" pills detected");
        
        // Small delay between pills
        if (round < 50) {
            delay(500);  // 500ms delay between pills
        }
    }
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Dispensing complete!");
    Serial.println("========================================");
    Serial.println();
    
    // Step 3: Print detailed results
    printTestResults();
    
    // Step 4: Print summary statistics
    printSummary();
}

// ============================================================================
// Single-Attempt Pill Dispensing Function
// ============================================================================

/**
 * Attempt to dispense a single pill (one attempt only, no retries)
 * @return Number of pills detected during the dispensing process
 */
int attemptToDispenseSinglePillOnce() {
    // Step 1: Activate electromagnet to pick up pill
    hardwareController->activateElectromagnetAndWaitForStabilization();
    
    // Step 2: Move servo to minimum position (rest position)
    hardwareController->moveServoToRestPositionAndWait();
    
    // Step 3: Move servo to maximum position (dispensing position)
    hardwareController->moveServoToMaxPositionAndWait();
    
    // Step 4: Wait 1000ms at maximum position and count pills detected
    int pillsDetected = countPillsDetected(1000);
    
    // Step 5: Move servo back to minimum position
    hardwareController->moveServoToRestPositionAndWait();
    
    // Step 6: Deactivate electromagnet
    hardwareController->deactivateElectromagnetWithDelay();
    
    return pillsDetected;
}

// ============================================================================
// Pill Detection Counting Function
// ============================================================================

/**
 * Count pills detected by IR sensor during a specified duration
 * Counts HIGH→LOW transitions (pill enters detection zone)
 * @param durationMs Duration to monitor in milliseconds
 * @return Number of pills detected
 */
int countPillsDetected(unsigned long durationMs) {
    unsigned long startTime = millis();
    int pillCount = 0;
    bool lastSensorState = HIGH;  // Start with HIGH (no pill detected)
    bool currentSensorState;
    
    // Check interval for polling (from config)
    int checkIntervalMs = systemConfig.pillDetectionCheckIntervalMilliseconds;
    
    while (millis() - startTime < durationMs) {
        currentSensorState = sensorManager->isPillCurrentlyDetectedByInfraredSensor();
        
        // Count transition from HIGH (no pill) to LOW (pill detected)
        // This represents a pill entering the detection zone
        if (lastSensorState == HIGH && currentSensorState == LOW) {
            pillCount++;
        }
        
        lastSensorState = currentSensorState;
        delay(checkIntervalMs);
    }
    
    return pillCount;
}

// ============================================================================
// Result Printing Functions
// ============================================================================

/**
 * Print detailed results for all 50 rounds
 */
void printTestResults() {
    Serial.println("========================================");
    Serial.println("DETAILED TEST RESULTS");
    Serial.println("========================================");
    Serial.println();
    
    for (int i = 0; i < 50; i++) {
        Serial.print("Round ");
        Serial.print(testResults[i].roundNumber);
        Serial.print(": ");
        Serial.print(testResults[i].pillsDispensed);
        Serial.print(" pill dispensed, ");
        Serial.print(testResults[i].pillsDetected);
        Serial.println(" pills detected");
    }
    
    Serial.println();
}

/**
 * Print summary statistics
 */
void printSummary() {
    Serial.println("========================================");
    Serial.println("SUMMARY STATISTICS");
    Serial.println("========================================");
    
    int totalPillsDispensed = 0;
    int totalPillsDetected = 0;
    int roundsWith1Pill = 0;
    int roundsWith0Pills = 0;
    int roundsWith2PlusPills = 0;
    
    for (int i = 0; i < 50; i++) {
        totalPillsDispensed += testResults[i].pillsDispensed;
        totalPillsDetected += testResults[i].pillsDetected;
        
        if (testResults[i].pillsDetected == 0) {
            roundsWith0Pills++;
        } else if (testResults[i].pillsDetected == 1) {
            roundsWith1Pill++;
        } else {
            roundsWith2PlusPills++;
        }
    }
    
    Serial.print("Total pills dispensed: ");
    Serial.println(totalPillsDispensed);
    Serial.print("Total pills detected: ");
    Serial.println(totalPillsDetected);
    Serial.println();
    Serial.print("Rounds with 0 pills detected: ");
    Serial.println(roundsWith0Pills);
    Serial.print("Rounds with 1 pill detected: ");
    Serial.println(roundsWith1Pill);
    Serial.print("Rounds with 2+ pills detected: ");
    Serial.println(roundsWith2PlusPills);
    Serial.println();
    
    float detectionRate = (float)totalPillsDetected / totalPillsDispensed * 100.0;
    Serial.print("Detection rate: ");
    Serial.print(detectionRate);
    Serial.println("%");
    Serial.println("========================================");
}
