/**
 * ============================================================================
 * ESP32 Pill Dispenser System - Main Application
 * ============================================================================
 * 
 * This is a modular pill dispensing system with the following features:
 * - Rotary compartment-based pill storage (5 compartments)
 * - Bluetooth Low Energy (BLE) control via iOS/Android
 * - LCD display with button-based manual control
 * - Automated homing and position tracking
 * - Multi-attempt dispensing with pill detection
 * 
 * Architecture:
 * - Config.h: Pin definitions and constants
 * - ConfigurationSettings.h: Tunable parameters
 * - SensorManager: Handles all sensor input and interrupts
 * - HardwareController: Controls motors, servos, and electromagnets
 * - DispenserController: High-level dispensing operations
 * - BLEManager: Bluetooth communication
 * - UIManager: LCD display and button handling
 * 
 * This main file orchestrates all modules with minimal coupling.
 */

#include <Arduino.h>

// Include all module headers
#include "Config.h"
#include "ConfigurationSettings.h"
#include "SensorManager.h"
#include "HardwareController.h"
#include "DispenserController.h"
#include "BLEManager.h"
#include "UIManager.h"

// ============================================================================
// Global System Configuration and Module Instances
// ============================================================================

// System configuration (modify settings here or in ConfigurationSettings.h)
SystemConfiguration systemConfig;

// Module instances
SensorManager* sensorManager;
HardwareController* hardwareController;
DispenserController* dispenserController;
BLEManager* bleManager;
UIManager* uiManager;

// ============================================================================
// Setup Function - Initialize all modules
// ============================================================================

void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    Serial.println("===========================================");
    Serial.println("ESP32 Pill Dispenser System");
    Serial.println("Initializing...");
    Serial.println("===========================================");
    
    // ========================================================================
    // Create module instances
    // ========================================================================
    
    sensorManager = new SensorManager(&systemConfig);
    hardwareController = new HardwareController(&systemConfig);
    dispenserController = new DispenserController(&systemConfig, hardwareController, sensorManager);
    bleManager = new BLEManager(&systemConfig);
    uiManager = new UIManager(&systemConfig);
    
    // ========================================================================
    // Initialize UI first to show status
    // ========================================================================
    
    uiManager->initializeLCDAndButtonPins();
    uiManager->displayInitializationMessage();
    
    // ========================================================================
    // Initialize sensors and hardware
    // ========================================================================
    
    sensorManager->initializeAllSensors();
    hardwareController->initializeAllHardwareActuators();
    dispenserController->initializeDispenserSystem();
    
    // ========================================================================
    // Attach sensor interrupts
    // ========================================================================
    
    // Set global instance pointer for ISR access
    globalSensorManagerInstance = sensorManager;
    
    // Attach encoder interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_FOR_ENCODER_CHANNEL_1), 
                    encoderInterruptServiceRoutine, 
                    CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_FOR_ENCODER_CHANNEL_2), 
                    encoderInterruptServiceRoutine, 
                    CHANGE);
    // NOTE: Home switch interrupt DISABLED - using polling instead to avoid unwanted triggers
    // attachInterrupt(digitalPinToInterrupt(PIN_FOR_HOME_POSITION_SWITCH), 
    //                 homeSwitchInterruptServiceRoutine, 
    //                 FALLING);
    
    Serial.println("Interrupts attached (home switch polling mode)");
    
    // ========================================================================
    // Initialize BLE communication
    // ========================================================================
    
    globalBLEManagerInstance = bleManager;
    bleManager->initializeBluetoothLEServer();
    
    // Hardware stabilization before homing
    delay(300);
    
    // ========================================================================
    // Perform homing sequence
    // ========================================================================
    
    uiManager->displayHomingInProgressMessage();
    bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
    
    if (homingSuccessful) {
        uiManager->displayHomingCompleteMessage();
        delay(systemConfig.delayAfterHomingCompleteMilliseconds);
    } else {
        uiManager->displayCustomMessageOnRow(1, "Homing FAILED!");
        delay(2000);
    }
    
    // ========================================================================
    // System ready
    // ========================================================================
    
    hardwareController->turnOnReadyStatusLED();
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
    
    Serial.println("===========================================");
    Serial.println("System Initialization Complete");
    Serial.println("Ready for operation");
    Serial.println("===========================================");
}

// ============================================================================
// Main Loop Function - Process events from all sources
// ============================================================================

void loop() {
    // ========================================================================
    // Button 6 - Homing Trigger
    // ========================================================================
    
    // Check if button 6 is pressed to trigger homing
    static unsigned long lastHomingButtonTime = 0;
    static bool lastHomingButtonState = HIGH;
    bool currentHomingButtonState = digitalRead(PIN_FOR_NAVIGATION_BACK_BUTTON);
    
    // Detect button press (transition from HIGH to LOW)
    if (lastHomingButtonState == HIGH && currentHomingButtonState == LOW) {
        // Check debounce (prevent multiple triggers)
        if (millis() - lastHomingButtonTime > systemConfig.homingButtonDebounceMilliseconds) {
            lastHomingButtonTime = millis();
            
            Serial.println("========================================");
            Serial.println("BUTTON 6 PRESSED - INITIATING HOMING");
            Serial.println("========================================");
            
            uiManager->displayHomingInProgressMessage();
            
            bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
            
            if (homingSuccessful) {
                uiManager->displayHomingCompleteMessage();
                delay(systemConfig.statusMessageDisplayTimeMilliseconds);
            } else {
                uiManager->displayCustomMessageOnRow(1, "Homing FAILED!");
                delay(systemConfig.errorMessageDisplayTimeMilliseconds);
            }
            
            // Update LCD to show ready status
            uiManager->displayReadyStatusWithCompartmentSelection(
                uiManager->getCurrentlySelectedCompartmentNumber(),
                bleManager->isBluetoothDeviceConnected()
            );
        }
    }
    
    lastHomingButtonState = currentHomingButtonState;
    
    // ========================================================================
    // Handle BLE connection state changes
    // ========================================================================
    
    bleManager->updateConnectionStateInMainLoop();
    
    // ========================================================================
    // Process BLE commands if available
    // ========================================================================
    
    if (bleManager->hasNewCommandAvailableToProcess()) {
        BLECommand command = bleManager->getAndClearMostRecentCommand();
        
        switch (command.commandType) {
            case BLECommand::DISPENSE:
                handleBLEDispenseCommand(command);
                break;
                
            case BLECommand::STATUS:
                handleBLEStatusCommand();
                break;
                
            case BLECommand::RESET:
                handleBLEResetCommand();
                break;
                
            case BLECommand::HOME:
                handleBLEHomeCommand();
                break;
                
            default:
                Serial.println("Unknown BLE command type");
                break;
        }
    }
    
    // ========================================================================
    // Process button inputs
    // ========================================================================
    
    ButtonAction buttonPressed = uiManager->checkIfAnyButtonPressedWithDebounce();
    
    if (buttonPressed != NO_BUTTON_PRESSED) {
        handleButtonPress(buttonPressed);
    }
    
    // ========================================================================
    // Small delay to prevent tight loop
    // ========================================================================
    
    delay(10);
}

// ============================================================================
// BLE Command Handler Functions
// ============================================================================

/**
 * Handle dispense command from BLE
 */
void handleBLEDispenseCommand(BLECommand command) {
    Serial.print("BLE Dispense command: Compartment ");
    Serial.print(command.compartmentNumber);
    Serial.print(", Count ");
    Serial.println(command.pillCount);
    
    // Validate compartment number
    if (command.compartmentNumber < 1 || 
        command.compartmentNumber > systemConfig.numberOfCompartmentsInDispenser) {
        bleManager->sendErrorResponseToConnectedDevice("Invalid compartment number");
        return;
    }
    
    // Update LCD
    uiManager->displayDispensingInProgressMessage(command.compartmentNumber);
    
    // Perform dispensing
    int successCount = dispenserController->dispensePillsFromCompartment(
        command.compartmentNumber,
        command.pillCount
    );
    
    // Send result back via BLE
    bleManager->sendDispenseResultToConnectedDevice(successCount, command.pillCount);
    
    // Handle failure case: all attempts failed
    if (successCount == 0) {
        Serial.println("----------------------------------------");
        Serial.println("ERROR: FAILED DISPENSING");
        Serial.print("Manual override to redispense pill from compartment ");
        Serial.print(command.compartmentNumber);
        Serial.println(". Or check pill levels.");

        uiManager->displayFailureMessage();
        delay(systemConfig.errorMessageDisplayTimeMilliseconds);

        // Show guidance in brief LCD messages (16x2 constraints)
        uiManager->clearLCDDisplay();
        uiManager->displayCustomMessageOnRow(0, String("Override slot ") + String(command.compartmentNumber));
        uiManager->displayCustomMessageOnRow(1, "Check pill levels");
        delay(systemConfig.statusMessageDisplayTimeMilliseconds);

        // Re-home automatically
        uiManager->displayHomingInProgressMessage();
        bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
        if (homingSuccessful) {
            uiManager->displayHomingCompleteMessage();
            delay(systemConfig.statusMessageDisplayTimeMilliseconds);
        } else {
            uiManager->displayCustomMessageOnRow(1, "Homing FAILED!");
            delay(systemConfig.errorMessageDisplayTimeMilliseconds);
        }
    }

    // Update LCD
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

/**
 * Handle status request from BLE
 */
void handleBLEStatusCommand() {
    Serial.println("BLE Status command");
    
    // Collect statistics from all compartments
    int compartmentCounts[5];
    for (int i = 0; i < systemConfig.numberOfCompartmentsInDispenser; i++) {
        compartmentCounts[i] = dispenserController->getDispenseCountForCompartment(i + 1);
    }
    
    // Send status via BLE
    bleManager->sendStatisticsStatusToConnectedDevice(
        compartmentCounts, 
        systemConfig.numberOfCompartmentsInDispenser
    );
    
    // Also print to serial for debugging
    dispenserController->printDispenserStatistics();
}

/**
 * Handle reset statistics command from BLE
 */
void handleBLEResetCommand() {
    Serial.println("BLE Reset command");
    
    dispenserController->resetAllDispenseStatistics();
    bleManager->sendSuccessResponseToConnectedDevice("Statistics reset");
    
    // Update LCD
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

/**
 * Handle home command from BLE
 */
void handleBLEHomeCommand() {
    Serial.println("BLE Home command");
    
    uiManager->displayHomingInProgressMessage();
    
    bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
    
    if (homingSuccessful) {
        uiManager->displayHomingCompleteMessage();
        bleManager->sendSuccessResponseToConnectedDevice("Homing complete");
        delay(systemConfig.statusMessageDisplayTimeMilliseconds);
    } else {
        bleManager->sendErrorResponseToConnectedDevice("Homing failed");
        uiManager->displayFailureMessage();
        delay(systemConfig.errorMessageDisplayTimeMilliseconds);
    }
    
    // Update LCD
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

// ============================================================================
// Button Handler Functions
// ============================================================================

/**
 * Handle button press events
 */
void handleButtonPress(ButtonAction action) {
    Serial.print("Button pressed: ");
    Serial.println(action);
    
    if (action == NAVIGATION_SELECT_PRESSED) {
        // Dispense from currently selected compartment
        handleManualDispenseRequest();
    } else {
        // Update compartment selection
        uiManager->handleButtonActionAndUpdateSelection(
            action, 
            systemConfig.numberOfCompartmentsInDispenser
        );
        
        // Update LCD display
        uiManager->displayReadyStatusWithCompartmentSelection(
            uiManager->getCurrentlySelectedCompartmentNumber(),
            bleManager->isBluetoothDeviceConnected()
        );
    }
}

/**
 * Handle manual dispense request from button press
 */
void handleManualDispenseRequest() {
    int selectedCompartment = uiManager->getCurrentlySelectedCompartmentNumber();
    
    Serial.print("Manual dispense from compartment ");
    Serial.println(selectedCompartment);
    
    // Update LCD
    uiManager->displayDispensingInProgressMessage(selectedCompartment);
    
    // Perform dispensing (single pill)
    int successCount = dispenserController->dispensePillsFromCompartment(selectedCompartment, 1);
    
    // Show result
    if (successCount > 0) {
        uiManager->displaySuccessMessage();
        delay(systemConfig.successMessageDisplayTimeMilliseconds);
    } else {
        Serial.println("----------------------------------------");
        Serial.println("ERROR: FAILED DISPENSING");
        Serial.print("Check pill levels & Manual override to redispense pill from compartment ");
        Serial.print(selectedCompartment);

        uiManager->displayFailureMessage();
        delay(systemConfig.errorMessageDisplayTimeMilliseconds);

        uiManager->clearLCDDisplay();
        uiManager->displayCustomMessageOnRow(0, "Check pill levels");
        uiManager->displayCustomMessageOnRow(1, String("Override slot ") + String(selectedCompartment));
        delay(systemConfig.statusMessageDisplayTimeMilliseconds);

        // Re-home automatically
        uiManager->displayHomingInProgressMessage();
        bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
        if (homingSuccessful) {
            uiManager->displayHomingCompleteMessage();
            delay(systemConfig.statusMessageDisplayTimeMilliseconds);
        } else {
            uiManager->displayCustomMessageOnRow(1, "Homing FAILED!");
            delay(systemConfig.errorMessageDisplayTimeMilliseconds);
        }
    }
    
    // Return to ready display
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}
