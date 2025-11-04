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
    // ========================================================================
    // Motor Speed Settings (PWM values: 0-255 standard, ESP32 can use 0-255 or higher with custom resolution)
    // ========================================================================
    int motorHomingSpeedPWM = 255;              // Speed during homing (needs higher torque to overcome static friction)
    int motorRunningSpeedPWM = 255;             // Normal running speed for compartment moves
    // If motor doesn't run or behaves erratically, reduce to 255 or lower.
    
    // ========================================================================
    // Servo Positioning Settings (angles in degrees: 0-180)
    // ========================================================================
    int servoRestPositionAngleInDegrees = 0;           // Home/rest position
    int servoDispensingAngleInDegrees = 180;            // Position to release pill
    int servoMovementDelayMilliseconds = 500;          // Wait time after servo movement
    
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

	// Container Positions (degrees from START position = switch position)
    // ARRAY-BASED: Each container can have custom position
    // Index 0 = Container 1, Index 1 = Container 2, etc.
	float containerPositionsInDegrees[5] = {
		88.0,       // Container 1: absolute angle from START
		276.0,      // Container 2: absolute angle from START
		450.0,  	// Container 3: absolute angle from START
		630.0,	    // Container 4: absolute angle from START
		850.0  	// Container 5: absolute angle from START
	};
    // To customize: Simply change the angle values above for any container
    // Example: {0.0, 72.0, 144.0, 216.0, 288.0} for equal spacing
    
    // ========================================================================
    // Homing Sequence Settings
    // ========================================================================
    int delayAfterHomingSwitchActivationMilliseconds = 100;    // Settling time after hitting home
    int delayAfterHomingCompleteMilliseconds = 1000;           // Display delay before ready
    int homingRetryAttempts = 1;                               // Number of homing attempts (200 PWM works reliably)
    int homingSpeedIncrementPerRetry = 0;                      // No speed increment needed (already at 200 PWM)
    int homingTimeoutIncrementPerRetry = 0;                    // No timeout increment needed
    
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
    int approximateMovementDelayMilliseconds = 500;            // Basic time-based movement (for systems without precise encoder control)
    float estimatedMotorDegreesPerSecond = 180.0;             // Estimated rotation speed at normal PWM (calibrate based on testing)
    // To calibrate: Time how long it takes to rotate 360° at motorRunningSpeedPWM, then: degrees_per_sec = 360 / time_in_seconds
    
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

