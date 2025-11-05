#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "Config.h"
#include "ConfigurationSettings.h"

/**
 * Command structure for parsed BLE commands
 */
struct BLECommand {
    enum CommandType {
        NONE,
        DISPENSE,
        STATUS,
        RESET,
        HOME
    };
    
    CommandType commandType;
    int compartmentNumber;
    int pillCount;
    
    BLECommand() : commandType(NONE), compartmentNumber(0), pillCount(1) {}
};

/**
 * Forward declarations for callback classes
 */
class BLEConnectionCallbacks;
class BLECharacteristicWriteCallbacks;

/**
 * BLEManager Class
 * 
 * Responsible for all Bluetooth Low Energy communication including:
 * - BLE server initialization and configuration
 * - Connection state management
 * - Command parsing from connected devices
 * - Response notifications to connected devices
 * 
 * This class has minimal dependencies and returns commands for execution
 * by the main loop, maintaining low coupling.
 */
class BLEManager {
private:
    SystemConfiguration* systemConfiguration;
    BLEServer* bluetoothLEServer;
    BLECharacteristic* commandCharacteristic;
    bool isDeviceCurrentlyConnectedViaBluetooth;
    bool wasDeviceConnectedInPreviousLoop;
    BLECommand mostRecentCommandReceived;
    bool hasNewCommandToProcess;
    
    friend class BLEConnectionCallbacks;
    friend class BLECharacteristicWriteCallbacks;
    
public:
    /**
     * Constructor
     * @param config Pointer to system configuration
     */
    BLEManager(SystemConfiguration* config) {
        systemConfiguration = config;
        bluetoothLEServer = nullptr;
        commandCharacteristic = nullptr;
        isDeviceCurrentlyConnectedViaBluetooth = false;
        wasDeviceConnectedInPreviousLoop = false;
        hasNewCommandToProcess = false;
    }
    
    /**
     * Initialize BLE server and start advertising
     */
    void initializeBluetoothLEServer();
    
    /**
     * Update connection state and handle reconnection
     * Call this in main loop
     */
    void updateConnectionStateInMainLoop() {
        if (!isDeviceCurrentlyConnectedViaBluetooth && wasDeviceConnectedInPreviousLoop) {
            delay(systemConfiguration->bleReconnectionDelayMilliseconds);
            bluetoothLEServer->startAdvertising();
            wasDeviceConnectedInPreviousLoop = isDeviceCurrentlyConnectedViaBluetooth;
        }
        
        if (isDeviceCurrentlyConnectedViaBluetooth && !wasDeviceConnectedInPreviousLoop) {
            wasDeviceConnectedInPreviousLoop = isDeviceCurrentlyConnectedViaBluetooth;
        }
    }
    
    /**
     * Check if a device is currently connected
     * @return true if BLE device is connected
     */
    bool isBluetoothDeviceConnected() {
        return isDeviceCurrentlyConnectedViaBluetooth;
    }
    
    /**
     * Check if there is a new command to process
     * @return true if new command is available
     */
    bool hasNewCommandAvailableToProcess() {
        return hasNewCommandToProcess;
    }
    
    /**
     * Get the most recent command (and mark as processed)
     * @return BLECommand structure with command details
     */
    BLECommand getAndClearMostRecentCommand() {
        hasNewCommandToProcess = false;
        return mostRecentCommandReceived;
    }
    
    /**
     * Send success response to connected device
     * @param message Success message to send
     */
    void sendSuccessResponseToConnectedDevice(String message) {
        if (commandCharacteristic != nullptr && isDeviceCurrentlyConnectedViaBluetooth) {
            String response = "{status:OK, message:\"" + message + "\"}";
            commandCharacteristic->setValue(response.c_str());
            commandCharacteristic->notify();
        }
    }
    
    void sendErrorResponseToConnectedDevice(String errorMessage) {
        if (commandCharacteristic != nullptr && isDeviceCurrentlyConnectedViaBluetooth) {
            String response = "{status:ERROR, message:\"" + errorMessage + "\"}";
            commandCharacteristic->setValue(response.c_str());
            commandCharacteristic->notify();
        }
    }
    
    void sendDispenseResultToConnectedDevice(int successCount, int requestedCount) {
        if (commandCharacteristic != nullptr && isDeviceCurrentlyConnectedViaBluetooth) {
            String response = "{status:OK, dispensed:" + String(successCount) + 
                            ", requested:" + String(requestedCount) + "}";
            commandCharacteristic->setValue(response.c_str());
            commandCharacteristic->notify();
        }
    }
    
    void sendStatisticsStatusToConnectedDevice(int* compartmentCounts, int numberOfCompartments) {
        if (commandCharacteristic != nullptr && isDeviceCurrentlyConnectedViaBluetooth) {
            String response = "{status:OK, compartments:[";
            for (int i = 0; i < numberOfCompartments; i++) {
                response += String(compartmentCounts[i]);
                if (i < numberOfCompartments - 1) response += ",";
            }
            response += "]}";
            commandCharacteristic->setValue(response.c_str());
            commandCharacteristic->notify();
        }
    }
    
    /**
     * Parse incoming BLE command string
     * @param commandString Raw command string from BLE
     */
    void parseBLECommandAndExtractParameters(String commandString) {
        mostRecentCommandReceived = BLECommand();
        
        if (commandString.startsWith("DISPENSE:")) {
            mostRecentCommandReceived.commandType = BLECommand::DISPENSE;
            
            int firstColonPosition = commandString.indexOf(':');
            int secondColonPosition = commandString.indexOf(':', firstColonPosition + 1);
            
            String compartmentString = commandString.substring(
                firstColonPosition + 1,
                secondColonPosition > 0 ? secondColonPosition : commandString.length()
            );
            mostRecentCommandReceived.compartmentNumber = compartmentString.toInt();
            
            if (secondColonPosition > 0) {
                String countString = commandString.substring(secondColonPosition + 1);
                mostRecentCommandReceived.pillCount = countString.toInt();
                if (mostRecentCommandReceived.pillCount < 1) {
                    mostRecentCommandReceived.pillCount = 1;
                }
            }
            
            hasNewCommandToProcess = true;
        }
        else if (commandString == "STATUS") {
            mostRecentCommandReceived.commandType = BLECommand::STATUS;
            hasNewCommandToProcess = true;
        }
        else if (commandString == "RESET") {
            mostRecentCommandReceived.commandType = BLECommand::RESET;
            hasNewCommandToProcess = true;
        }
        else if (commandString == "HOME") {
            mostRecentCommandReceived.commandType = BLECommand::HOME;
            hasNewCommandToProcess = true;
        }
        else {
            Serial.println("ERROR: Unknown BLE command");
            sendErrorResponseToConnectedDevice("Unknown command: " + commandString);
        }
    }
};

// Global pointer for callback access
BLEManager* globalBLEManagerInstance = nullptr;

/**
 * BLE Server Connection Callbacks
 * Handles connect/disconnect events
 */
class BLEConnectionCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        if (globalBLEManagerInstance != nullptr) {
            globalBLEManagerInstance->isDeviceCurrentlyConnectedViaBluetooth = true;
        }
    }
    
    void onDisconnect(BLEServer* pServer) {
        if (globalBLEManagerInstance != nullptr) {
            globalBLEManagerInstance->isDeviceCurrentlyConnectedViaBluetooth = false;
        }
    }
};

/**
 * BLE Characteristic Write Callbacks
 * Handles incoming command data
 */
class BLECharacteristicWriteCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        if (globalBLEManagerInstance != nullptr) {
            String receivedValue = pCharacteristic->getValue();
            
            if (receivedValue.length() > 0) {
                String commandString = String(receivedValue.c_str());
                globalBLEManagerInstance->parseBLECommandAndExtractParameters(commandString);
            }
        }
    }
};

/**
 * Initialize BLE server (implementation must be after callback class definitions)
 */
void BLEManager::initializeBluetoothLEServer() {
    BLEDevice::init(BLE_DEVICE_NAME);
    
    bluetoothLEServer = BLEDevice::createServer();
    bluetoothLEServer->setCallbacks(new BLEConnectionCallbacks());
    
    BLEService* pillDispenserService = bluetoothLEServer->createService(BLE_SERVICE_UUID);
    
    commandCharacteristic = pillDispenserService->createCharacteristic(
        BLE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    commandCharacteristic->setCallbacks(new BLECharacteristicWriteCallbacks());
    commandCharacteristic->addDescriptor(new BLE2902());
    
    pillDispenserService->start();
    
    BLEAdvertising* advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(systemConfiguration->bleMinimumConnectionIntervalPreference);
    advertising->setMinPreferred(systemConfiguration->bleMaximumConnectionIntervalPreference);
    
    BLEDevice::startAdvertising();
}

#endif // BLE_MANAGER_H

