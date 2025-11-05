#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "Config.h"
#include "ConfigurationSettings.h"

/**
 * SensorManager Class
 * 
 * Responsible for all sensor input handling including:
 * - Home position switch detection
 * - IR pill sensor reading
 * - Rotary encoder position tracking with interrupts
 * 
 * This class has no dependencies on actuators or display systems (low coupling).
 */
class SensorManager {
private:
    SystemConfiguration* systemConfiguration;
    
    // Encoder state variables (must be volatile for ISR access)
    volatile long currentEncoderPositionCounter;
    volatile int lastEncoderChannelAState;
    
public:
    /**
     * Constructor
     * @param config Pointer to system configuration
     */
    SensorManager(SystemConfiguration* config) {
        systemConfiguration = config;
        currentEncoderPositionCounter = 0;
        lastEncoderChannelAState = 0;
    }
    
    /**
     * Initialize all sensor pins and attach interrupts
     */
    void initializeAllSensors() {
        // Configure home switch with internal pull-up resistor
        pinMode(PIN_FOR_HOME_POSITION_SWITCH, INPUT_PULLUP);
        
        // Configure IR pill detector as input
        pinMode(PIN_FOR_INFRARED_PILL_DETECTOR, INPUT);
        
        // Configure encoder channels as inputs
        pinMode(PIN_FOR_ENCODER_CHANNEL_1, INPUT);
        pinMode(PIN_FOR_ENCODER_CHANNEL_2, INPUT);
        
        // Attach interrupts for encoder (must be done in main setup, not constructor)
        // This is called from main setup after object creation
    }
    
    /**
     * Check if the home position switch is currently activated
     * @return true if home switch is pressed
     * Standard pull-up logic: HIGH when not pressed, LOW when pressed
     */
    bool isHomePositionSwitchActivated() {
        // LOW = pressed (standard pull-up resistor behavior)
        return digitalRead(PIN_FOR_HOME_POSITION_SWITCH) == LOW;
    }
    
    /**
     * Get raw switch pin reading for diagnostics
     * @return Raw digital pin state (HIGH=1, LOW=0)
     */
    int getRawHomeSwitchPinState() {
        return digitalRead(PIN_FOR_HOME_POSITION_SWITCH);
    }
    
    /**
     * Wait for home position switch activation with timeout
     * @param timeoutMilliseconds Maximum time to wait
     * @return true if home switch was activated, false if timeout occurred
     */
    bool waitForHomeSwitchActivationWithTimeout(unsigned long timeoutMilliseconds) {
        unsigned long startTimeMillis = millis();
        
        while (!isHomePositionSwitchActivated()) {
            if (millis() - startTimeMillis > timeoutMilliseconds) {
                Serial.println("ERROR: Homing timeout - switch not activated");
                return false;
            }
            delay(10);
        }
        
        return true;
    }
    
    /**
     * Check if a pill is currently detected by the IR sensor
     * @return true if pill is detected (LOW = detected based on typical IR sensor behavior)
     */
    bool isPillCurrentlyDetectedByInfraredSensor() {
        return digitalRead(PIN_FOR_INFRARED_PILL_DETECTOR) == LOW;
    }
    
    /**
     * Wait for pill detection with timeout
     * @return true if pill was detected within timeout period, false otherwise
     */
    bool waitForPillDetectionWithTimeout() {
        unsigned long startTimeMillis = millis();
        int timeoutMillis = systemConfiguration->pillDetectionTimeoutMilliseconds;
        int checkIntervalMillis = systemConfiguration->pillDetectionCheckIntervalMilliseconds;
        
        while (millis() - startTimeMillis < timeoutMillis) {
            if (isPillCurrentlyDetectedByInfraredSensor()) {
                return true;
            }
            delay(checkIntervalMillis);
        }
        
        return false;
    }
    
    /**
     * Get the current encoder position counter value
     * @return Current encoder position
     */
    long getCurrentEncoderPosition() {
        return currentEncoderPositionCounter;
    }
    
    /**
     * Reset the encoder position counter to zero
     */
    void resetEncoderPositionToZero() {
        currentEncoderPositionCounter = 0;
    }
    
    /**
     * Encoder interrupt service routine - Channel 1
     * Must be called from global ISR with IRAM_ATTR
     */
    void handleEncoderInterrupt() {
        int channelAState = digitalRead(PIN_FOR_ENCODER_CHANNEL_1);
        int channelBState = digitalRead(PIN_FOR_ENCODER_CHANNEL_2);
        
        if (channelAState != lastEncoderChannelAState) {
            if (channelBState != channelAState) {
                currentEncoderPositionCounter++;
            } else {
                currentEncoderPositionCounter--;
            }
        }
        lastEncoderChannelAState = channelAState;
    }
    
    /**
     * Home switch interrupt service routine
     * Must be called from global ISR with IRAM_ATTR
     */
    void handleHomeSwitchInterrupt() {
    }
};

// Global pointer for ISR access (needed because ISRs can't be class members)
SensorManager* globalSensorManagerInstance = nullptr;

/**
 * Global ISR wrapper functions (must have IRAM_ATTR for ESP32)
 */
void IRAM_ATTR encoderInterruptServiceRoutine() {
    if (globalSensorManagerInstance != nullptr) {
        globalSensorManagerInstance->handleEncoderInterrupt();
    }
}

void IRAM_ATTR homeSwitchInterruptServiceRoutine() {
    if (globalSensorManagerInstance != nullptr) {
        globalSensorManagerInstance->handleHomeSwitchInterrupt();
    }
}

#endif // SENSOR_MANAGER_H

