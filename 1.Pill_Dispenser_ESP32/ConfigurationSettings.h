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
    // Stepper Motor Settings
    // ========================================================================
    // 
    // HOW STEPPER MOTORS WORK:
    // - Stepper motors move in discrete steps (not continuous like DC motors)
    // - Speed = step pulse frequency (how fast pulses are sent)
    // - Speed is controlled by delay between step pulses: speed = 1 / delay_between_steps
    // - Lower delay = faster movement, higher delay = slower movement
    // - Each step pulse moves motor by: 360° / (stepsPerRevolution * microstepping * gearRatio)
    //
    // STEPPER MOTOR SPEED CONTROL:
    // - Stepper motors use step pulse frequency to control speed
    // - Speed = 1 / (delay_between_steps_in_microseconds)
    // - We use direct step delay values (in microseconds) - this is how stepper motors work
    // - Lower delay = higher step frequency = faster movement
    // - Higher delay = lower step frequency = slower movement
    // - User calibrates these values based on motor/driver capabilities and mechanical load
    //
    
    // Stepper motor mechanical configuration (CALIBRATE THESE!)
    int stepperStepsPerRevolution = 200;        // Steps per full rotation (200 for 1.8° motor, 400 for 0.9°)
    int stepperMicrostepping = 1;               // Microstepping setting on driver (1, 2, 4, 8, 16)
    float stepperGearRatio = 1.0;               // Gear reduction ratio (1.0 if no gear reduction)
    
    // Step pulse timing - SYMMETRIC PULSE WIDTH (from test code)
    // Uses symmetric pulse pattern: HIGH for stepPulseWidth, LOW for stepPulseWidth
    // Total time per step = 2 × stepPulseWidth
    // Speed calculation: Steps/sec = 1,000,000 / (2 × stepPulseWidth)
    // Example: 15000μs pulse width = 30,000μs per step = ~33 steps/sec = ~10 RPM (for 200 steps/rev motor)
    // This provides slower, smoother movement compared to fast pulses with delays
    int stepperStepPulseWidthMicroseconds = 15000;   // Pulse width (HIGH and LOW time) - from test code
    
    // Legacy delay parameters (kept for compatibility, but not used with symmetric pulse timing)
    // These are now calculated from stepPulseWidth
    int stepperHomingStepDelayMicroseconds = 15000;   // Not used directly, kept for compatibility
    int stepperRunningStepDelayMicroseconds = 15000;  // Not used directly, kept for compatibility
    
    // Safety limits for step pulse width (to prevent motor damage)
    int stepperMinStepPulseWidthMicroseconds = 10000;   // Minimum safe pulse width (fastest speed) - adjust based on motor/driver limits
    int stepperMaxStepPulseWidthMicroseconds = 50000;  // Maximum pulse width (slowest speed) - for very slow movements
    
    // Calculate total steps per revolution with microstepping and gear ratio
    // stepsPerRevolution = stepperStepsPerRevolution * stepperMicrostepping * stepperGearRatio
    
    // ========================================================================
    // Servo Positioning Settings (microseconds for precise control)
    // ========================================================================
    // Servo uses microseconds for precise control (typical range: 500-2500μs)
    // These values are calculated as MIN_SAFE and MAX_SAFE (from test code)
    int servoMinMicroseconds = 150;          // Minimum servo position (adjust for your servo)
    int servoMaxMicroseconds = 2100;        // Maximum servo position (adjust for your servo)
    int servoEndMarginMicroseconds = 0;     // Back off from hard stops to avoid stall
    
    // Calculated safe endpoints (computed in setup)
    // MIN_SAFE = servoMinMicroseconds + servoEndMarginMicroseconds
    // MAX_SAFE = servoMaxMicroseconds - servoEndMarginMicroseconds
    
    // Servo motion parameters (from test code)
    int servoStepMicroseconds = 60;         // Step size for servo movement (larger = faster motion)
    int servoStepDelayMilliseconds = 1;     // Delay between steps (smaller = faster)
    
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

	// Container Positions (degrees from START position = switch position)
    // ARRAY-BASED: Each container can have custom position
    // Index 0 = Container 1, Index 1 = Container 2, etc.
    // Evenly spaced: 360° / 5 compartments = 72° spacing
	float containerPositionsInDegrees[5] = {
		0.0,        // Container 1: at home/START position (0°)
		72.0,       // Container 2: 72° from START (evenly spaced)
		144.0,      // Container 3: 144° from START (evenly spaced)
		216.0,      // Container 4: 216° from START (evenly spaced)
		288.0       // Container 5: 288° from START (evenly spaced)
	};
    // These positions are evenly spaced: 0°, 72°, 144°, 216°, 288°
    // To customize: Simply change the angle values above for any container
    // Movement times (at 1253ms per 360° rotation):
    // - Compartment 1: 0ms (at home)
    // - Compartment 2: ~251ms (72°)
    // - Compartment 3: ~501ms (144°)
    // - Compartment 4: ~752ms (216°)
    // - Compartment 5: ~1002ms (288°)
    
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

