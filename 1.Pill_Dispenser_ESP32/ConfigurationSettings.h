#ifndef CONFIGURATION_SETTINGS_H
#define CONFIGURATION_SETTINGS_H

/**
 * SystemConfiguration Structure
 * 
 * This structure contains all tunable parameters for the pill dispenser system.
 * Modify these values to adjust system behavior without searching through code.
 * All timing values are in milliseconds unless otherwise specified.
 */
struct SystemConfiguration {
    int stepperStepsPerRevolution = 200;
    int stepperMicrostepping = 1;
    float stepperGearRatio = 1.0;
    
    int stepperStepPulseWidthMicroseconds = 15000;
    
    int stepperHomingStepDelayMicroseconds = 15000;
    int stepperRunningStepDelayMicroseconds = 15000;
    
    int stepperMinStepPulseWidthMicroseconds = 10000;
    int stepperMaxStepPulseWidthMicroseconds = 50000;
    
    int servoMinMicroseconds = 150;
    int servoMaxMicroseconds = 2100;
    int servoEndMarginMicroseconds = 0;
    
    int servoStepMicroseconds = 60;
    int servoStepDelayMilliseconds = 1;
    
    int servoMovementDelayMilliseconds = 500;  // Wait time after servo movement completes
    
    // ========================================================================
    // Pill Detection Settings
    // ========================================================================
    int pillDetectionTimeoutMilliseconds = 2000;       // Max time to wait for pill detection
    int pillDetectionCheckIntervalMilliseconds = 10;   // Polling interval for IR sensor
    
    // ========================================================================
    // Electromagnet Settings
    // ========================================================================
    int electromagnetActivationDelayMilliseconds = 200;    // Stabilization time after activation
    int electromagnetDeactivationDelayMilliseconds = 200;  // Wait time before deactivation
    
    // ========================================================================
    // Button Input Settings
    // ========================================================================
    int buttonDebounceDelayMilliseconds = 200;          // Debounce time for button presses
    int homingButtonDebounceMilliseconds = 1000;       // Debounce time for homing button (button 6)
    
    // ========================================================================
    // Auto-Homing Settings
    // ========================================================================
    bool autoHomeAfterDispense = true;                 // Automatically home after successful pill dispense
    
    // ========================================================================
    // Dispenser Mechanical Settings
    // ========================================================================
    int maximumDispenseAttempts = 3;                   // Retry attempts if pill not detected
    int numberOfCompartmentsInDispenser = 5;           // Total compartments in rotary dispenser

	float containerPositionsInDegrees[5] = {
		0.0,
		65.0,
		144.0,
		216.0,
		288.0
	};
    
    // ========================================================================
    // Homing Sequence Settings
    // ========================================================================
    int delayAfterHomingSwitchActivationMilliseconds = 100;    // Settling time after hitting home
    int delayAfterHomingCompleteMilliseconds = 1000;           // Display delay before ready
    int homingRetryAttempts = 1;                               // Number of homing attempts
    int homingDelayDecrementPerRetry = 0;                      // Decrease delay (increase speed) per retry (0 = same speed)
    int homingTimeoutIncrementPerRetry = 0;                    // Increase timeout per retry (0 = same timeout)
    
    // ========================================================================
    // Dispensing Operation Delays
    // ========================================================================
    int delayBetweenDispenseAttemptsMilliseconds = 2000;        // Wait between retry attempts
    int delayBetweenMultipleDispensesMilliseconds = 1000;       // Wait when dispensing multiple pills
    int delayAfterCompartmentMoveMilliseconds = 200;           // Settling time after rotation
    
    // ========================================================================
    // Movement Calculation Settings
    // ========================================================================
    float encoderPositionMultiplierForCompartment = 50.0;      // Encoder scaling factor (tune based on hardware)
    // NOTE: Time-based movement removed - now using step-based positioning for precision
    
    // ========================================================================
    // UI Display Settings
    // ========================================================================
    int successMessageDisplayTimeMilliseconds = 1500;          // How long to show "Success!" message
    int errorMessageDisplayTimeMilliseconds = 1500;            // How long to show "Failed!" message
    int statusMessageDisplayTimeMilliseconds = 1000;           // General status message duration
    
    // ========================================================================
    // BLE Communication Settings
    // ========================================================================
    int bleReconnectionDelayMilliseconds = 500;                // Delay before restarting advertising
    int bleMinimumConnectionIntervalPreference = 0x06;         // BLE connection interval (units of 1.25ms)
    int bleMaximumConnectionIntervalPreference = 0x12;         // BLE connection interval (units of 1.25ms)
};

#endif // CONFIGURATION_SETTINGS_H

