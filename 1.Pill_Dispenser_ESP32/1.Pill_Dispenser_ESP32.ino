/**
 * ESP32 Pill Dispenser System - Main Application
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

SystemConfiguration systemConfig;
SensorManager* sensorManager;
HardwareController* hardwareController;
DispenserController* dispenserController;
BLEManager* bleManager;
UIManager* uiManager;

void setup() {
    Serial.begin(115200);
    
    sensorManager = new SensorManager(&systemConfig);
    hardwareController = new HardwareController(&systemConfig);
    dispenserController = new DispenserController(&systemConfig, hardwareController, sensorManager);
    bleManager = new BLEManager(&systemConfig);
    uiManager = new UIManager(&systemConfig);
    
    uiManager->initializeLCDAndButtonPins();
    uiManager->displayInitializationMessage();
    
    sensorManager->initializeAllSensors();
    hardwareController->initializeAllHardwareActuators();
    dispenserController->initializeDispenserSystem();
    
    globalSensorManagerInstance = sensorManager;
    
    attachInterrupt(digitalPinToInterrupt(PIN_FOR_ENCODER_CHANNEL_1), 
                    encoderInterruptServiceRoutine, 
                    CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_FOR_ENCODER_CHANNEL_2), 
                    encoderInterruptServiceRoutine, 
                    CHANGE);
    
    globalBLEManagerInstance = bleManager;
    bleManager->initializeBluetoothLEServer();
    
    delay(300);
    
    uiManager->displayHomingInProgressMessage();
    bool homingSuccessful = dispenserController->performHomingWithRetryAndEscalation();
    
    if (homingSuccessful) {
        uiManager->displayHomingCompleteMessage();
        delay(systemConfig.delayAfterHomingCompleteMilliseconds);
    } else {
        uiManager->displayCustomMessageOnRow(1, "Homing FAILED!");
        delay(2000);
    }
    
    hardwareController->performServoHomingSequence();
    
    hardwareController->turnOnReadyStatusLED();
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

void loop() {
    static unsigned long lastHomingButtonTime = 0;
    static bool lastHomingButtonState = HIGH;
    static unsigned long buttonPressStartTime = 0;
    bool currentHomingButtonState = digitalRead(PIN_FOR_NAVIGATION_BACK_BUTTON);
    
    if (lastHomingButtonState == HIGH && currentHomingButtonState == LOW) {
        buttonPressStartTime = millis();
    }
    
    if (lastHomingButtonState == LOW && currentHomingButtonState == HIGH) {
        unsigned long pressDuration = millis() - buttonPressStartTime;
        
        if (millis() - lastHomingButtonTime > systemConfig.homingButtonDebounceMilliseconds) {
            lastHomingButtonTime = millis();
            
            if (pressDuration >= 3000) {
                uiManager->displayCustomMessageOnRow(0, "Calibration...");
                uiManager->displayCustomMessageOnRow(1, "Measuring...");
                
                bool calibrationSuccessful = dispenserController->calibrateFullRotationTiming();
                
                if (calibrationSuccessful) {
                    uiManager->displayCustomMessageOnRow(0, "Calibration OK");
                    uiManager->displayCustomMessageOnRow(1, "Check Serial");
                    delay(3000);
                } else {
                    uiManager->displayCustomMessageOnRow(0, "Calibration");
                    uiManager->displayCustomMessageOnRow(1, "FAILED!");
                    delay(3000);
                }
            } else {
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
            
            uiManager->displayReadyStatusWithCompartmentSelection(
                uiManager->getCurrentlySelectedCompartmentNumber(),
                bleManager->isBluetoothDeviceConnected()
            );
        }
    }
    
    lastHomingButtonState = currentHomingButtonState;
    
    bleManager->updateConnectionStateInMainLoop();
    
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
    
    ButtonAction buttonPressed = uiManager->checkIfAnyButtonPressedWithDebounce();
    
    if (buttonPressed != NO_BUTTON_PRESSED) {
        handleButtonPress(buttonPressed);
    }
    
    delay(10);
}

void handleBLEDispenseCommand(BLECommand command) {
    if (command.compartmentNumber < 1 || 
        command.compartmentNumber > systemConfig.numberOfCompartmentsInDispenser) {
        bleManager->sendErrorResponseToConnectedDevice("Invalid compartment number");
        return;
    }
    
    uiManager->displayDispensingInProgressMessage(command.compartmentNumber);
    
    int successCount = dispenserController->dispensePillsFromCompartment(
        command.compartmentNumber,
        command.pillCount
    );
    
    bleManager->sendDispenseResultToConnectedDevice(successCount, command.pillCount);
    
    if (successCount == 0) {
        Serial.println("ERROR: Failed to dispense from compartment " + String(command.compartmentNumber));
        
        uiManager->displayFailureMessage();
        delay(systemConfig.errorMessageDisplayTimeMilliseconds);

        uiManager->clearLCDDisplay();
        uiManager->displayCustomMessageOnRow(0, String("Override slot ") + String(command.compartmentNumber));
        uiManager->displayCustomMessageOnRow(1, "Check pill levels");
        delay(systemConfig.statusMessageDisplayTimeMilliseconds);

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

    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

void handleBLEStatusCommand() {
    int compartmentCounts[5];
    for (int i = 0; i < systemConfig.numberOfCompartmentsInDispenser; i++) {
        compartmentCounts[i] = dispenserController->getDispenseCountForCompartment(i + 1);
    }
    
    bleManager->sendStatisticsStatusToConnectedDevice(
        compartmentCounts, 
        systemConfig.numberOfCompartmentsInDispenser
    );
}

void handleBLEResetCommand() {
    dispenserController->resetAllDispenseStatistics();
    bleManager->sendSuccessResponseToConnectedDevice("Statistics reset");
    
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

void handleBLEHomeCommand() {
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
    
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}

void handleButtonPress(ButtonAction action) {
    if (action == NAVIGATION_SELECT_PRESSED) {
        handleManualDispenseRequest();
    } else {
        uiManager->handleButtonActionAndUpdateSelection(
            action, 
            systemConfig.numberOfCompartmentsInDispenser
        );
        
        uiManager->displayReadyStatusWithCompartmentSelection(
            uiManager->getCurrentlySelectedCompartmentNumber(),
            bleManager->isBluetoothDeviceConnected()
        );
    }
}

void handleManualDispenseRequest() {
    int selectedCompartment = uiManager->getCurrentlySelectedCompartmentNumber();
    
    uiManager->displayDispensingInProgressMessage(selectedCompartment);
    
    int successCount = dispenserController->dispensePillsFromCompartment(selectedCompartment, 1);
    
    if (successCount > 0) {
        uiManager->displaySuccessMessage();
        delay(systemConfig.successMessageDisplayTimeMilliseconds);
    } else {
        Serial.println("ERROR: Failed to dispense from compartment " + String(selectedCompartment));

        uiManager->displayFailureMessage();
        delay(systemConfig.errorMessageDisplayTimeMilliseconds);

        uiManager->clearLCDDisplay();
        uiManager->displayCustomMessageOnRow(0, "Check pill levels");
        uiManager->displayCustomMessageOnRow(1, String("Override slot ") + String(selectedCompartment));
        delay(systemConfig.statusMessageDisplayTimeMilliseconds);

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
    
    uiManager->displayReadyStatusWithCompartmentSelection(
        uiManager->getCurrentlySelectedCompartmentNumber(),
        bleManager->isBluetoothDeviceConnected()
    );
}
