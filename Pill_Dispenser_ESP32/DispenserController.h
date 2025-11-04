#ifndef DISPENSER_CONTROLLER_H
#define DISPENSER_CONTROLLER_H

#include <Arduino.h>
#include "Config.h"
#include "ConfigurationSettings.h"
#include "HardwareController.h"
#include "SensorManager.h"

/**
 * DispenserController Class
 * 
 * Responsible for high-level dispensing operations including:
 * - System homing sequence
 * - Moving to specific compartments
 * - Multi-attempt pill dispensing
 * - Tracking dispense statistics
 * 
 * This class orchestrates hardware and sensors to perform complete operations.
 */
class DispenserController {
private:
    SystemConfiguration* systemConfiguration;
    HardwareController* hardwareController;
    SensorManager* sensorManager;
    
    // State tracking
    int currentCompartmentNumber;              // Current position: 0=home/start, 1-5=compartments
    bool isSystemHomedAndReady;                // True after successful homing
    int dispensedCountForEachCompartment[5];   // Max 5 compartments, adjust if needed
    
    // Position tracking in steps (absolute position from home)
    long currentPositionSteps;                 // Current absolute position in steps (0 = home position)
    long compartmentStepPositions[5];          // Absolute step positions for each compartment (calculated from degrees)
    
    /**
     * POSITIONING SYSTEM EXPLANATION:
     * 
     * START POSITION (Home):
     * - When home switch is pressed during homing → this defines position 0° (START)
     * - currentCompartmentNumber = 0
     * 
     * CONTAINER POSITIONS (stored in array in ConfigurationSettings.h):
     * - Each container has a custom position defined in containerPositionsInDegrees[] array
     * - Default positions (equal 72° spacing):
     *   • Container 1: containerPositionsInDegrees[0] = 0° (at START)
     *   • Container 2: containerPositionsInDegrees[1] = 72°
     *   • Container 3: containerPositionsInDegrees[2] = 144°
     *   • Container 4: containerPositionsInDegrees[3] = 216°
     *   • Container 5: containerPositionsInDegrees[4] = 288°
     * 
     * MOVEMENT CALCULATION:
     * - When container selected: get absolute_position from array
     * - Movement distance = target_absolute_position - current_absolute_position
     * - After movement: update currentCompartmentNumber to new position
     * 
     * EXAMPLE:
     * - After homing: currentCompartmentNumber = 0 (at START, 0°)
     * - Select Container 5: target = array[4] = 288°, travel = 288° - 0° = 288°
     * - After move: currentCompartmentNumber = 5 (now at 288°)
     * - Select Container 3: target = array[2] = 144°, travel = 144° - 288° = -144° → +216° (wraparound)
     * - After move: currentCompartmentNumber = 3 (now at 144°)
     *
     * This ensures ALL movements are calculated from CURRENT position, not always from START.
     * 
     * TO CUSTOMIZE: Edit containerPositionsInDegrees[] array in ConfigurationSettings.h
     */
    
public:
    /**
     * Constructor
     * @param config Pointer to system configuration
     * @param hardware Pointer to hardware controller
     * @param sensors Pointer to sensor manager
     */
    DispenserController(SystemConfiguration* config, 
                       HardwareController* hardware, 
                       SensorManager* sensors) {
        systemConfiguration = config;
        hardwareController = hardware;
        sensorManager = sensors;
        currentCompartmentNumber = 0;
        isSystemHomedAndReady = false;
        currentPositionSteps = 0;  // Start at unknown position until homed
        
        // Initialize dispense counters
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            dispensedCountForEachCompartment[i] = 0;
        }
        
        // Calculate compartment step positions from degrees
        calculateCompartmentStepPositions();
    }
    
    /**
     * Initialize the dispenser controller
     */
    void initializeDispenserSystem() {
        Serial.println("Dispenser controller initialized");
    }
    
    /**
     * Calculate compartment step positions from degree positions
     * Converts containerPositionsInDegrees[] to absolute step positions
     */
    void calculateCompartmentStepPositions() {
        Serial.println("========================================");
        Serial.println("[Position Tracking] Calculating compartment step positions");
        Serial.println("========================================");
        
        // Calculate total steps per revolution
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        
        Serial.print("Total steps per revolution: ");
        Serial.println(totalStepsPerRevolution);
        
        // Convert each compartment position from degrees to steps
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            float angleInDegrees = systemConfiguration->containerPositionsInDegrees[i];
            compartmentStepPositions[i] = (long)((angleInDegrees / 360.0) * totalStepsPerRevolution);
            
            Serial.print("Compartment ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(angleInDegrees);
            Serial.print("° = ");
            Serial.print(compartmentStepPositions[i]);
            Serial.println(" steps from home");
        }
        
        Serial.println("========================================");
    }
    
    /**
     * Get current absolute position in steps
     * @return Current position in steps from home (0 = home position)
     */
    long getCurrentPositionSteps() {
        return currentPositionSteps;
    }
    
    /**
     * Get current position in degrees
     * @return Current position in degrees from home (0° = home position)
     */
    float getCurrentPositionDegrees() {
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        return (currentPositionSteps / totalStepsPerRevolution) * 360.0;
    }
    
    /**
     * Reset position tracking to home (0 steps)
     * Called when homing switch is activated
     */
    void resetPositionToHome() {
        Serial.println("[Position Tracking] Resetting position to HOME (0 steps)");
        currentPositionSteps = 0;
        currentCompartmentNumber = 0;
    }
    
    /**
     * Update position tracking after movement
     * @param stepsMoved Number of steps moved (positive = forward, negative = backward)
     */
    void updatePositionAfterMovement(long stepsMoved) {
        currentPositionSteps += stepsMoved;
        
        // Handle wraparound (keep position within one revolution for display)
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        
        // Keep position in reasonable range (but allow it to go beyond for tracking)
        // Position can be negative or > 360° for absolute tracking
        
        Serial.print("[Position Tracking] Position updated: ");
        Serial.print(stepsMoved);
        Serial.print(" steps, new position: ");
        Serial.print(currentPositionSteps);
        Serial.print(" steps (");
        Serial.print(getCurrentPositionDegrees());
        Serial.println("°)");
    }
    
    // ========================================================================
    // Homing Operations
    // ========================================================================
    
    // performHomingSequenceUntilSwitchActivated removed (use performHomingWithRetryAndEscalation)
    
    /**
     * Perform homing with retry attempts and escalating motor speed
     * This method tries multiple times with increasing motor speed to overcome static friction
     * @return true if homing successful, false if all attempts failed
     */
    bool performHomingWithRetryAndEscalation() {
        Serial.println("========================================");
        Serial.println("Starting homing with retry capability");
        Serial.println("========================================");
        
        int maxAttempts = systemConfiguration->homingRetryAttempts;
        int baseDelay = systemConfiguration->stepperHomingStepDelayMicroseconds;
        int delayDecrement = systemConfiguration->homingDelayDecrementPerRetry;  // Decrease delay = increase speed
        int baseTimeout = MAXIMUM_HOMING_TIMEOUT_MILLISECONDS;
        int timeoutIncrement = systemConfiguration->homingTimeoutIncrementPerRetry;
        
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            Serial.println("----------------------------------------");
            Serial.print("Homing Attempt ");
            Serial.print(attempt);
            Serial.print(" of ");
            Serial.println(maxAttempts);
            Serial.println("----------------------------------------");
            
            // Calculate step delay for this attempt (decreasing delay = faster speed for retries)
            int attemptDelay = baseDelay - ((attempt - 1) * delayDecrement);
            // Ensure delay doesn't go below minimum safe value
            int minDelay = systemConfiguration->stepperMinStepDelayMicroseconds;
            attemptDelay = constrain(attemptDelay, minDelay, baseDelay);
            
            // Calculate timeout for this attempt (increasing with each retry)
            unsigned long attemptTimeout = baseTimeout + ((attempt - 1) * timeoutIncrement);
            
            Serial.print("[Attempt ");
            Serial.print(attempt);
            Serial.print("] Step delay: ");
            Serial.print(attemptDelay);
            Serial.print(" μs | Timeout: ");
            Serial.print(attemptTimeout);
            Serial.println(" ms");
            
			// Pre-check: Are we already at home? (only if system was previously homed)
			if (isSystemHomedAndReady && sensorManager->isHomePositionSwitchActivated()) {
				Serial.println("[Homing] Already at home switch - defining START here");
				sensorManager->resetEncoderPositionToZero();
				resetPositionToHome();  // Reset position tracking to 0 steps
				Serial.println("========================================");
				Serial.println("Homing completed successfully!");
				Serial.println("========================================");
				return true;
			}
            
            // If switch is pressed at startup, move off it first
            if (sensorManager->isHomePositionSwitchActivated()) {
                Serial.println("[Homing] Switch is pressed at startup - moving off home position first...");
                // Move backward (anticlockwise) by a small angle to get off the switch
                // Since we home forward (clockwise), we need to move backward first
                long stepsMoved = hardwareController->moveStepperBackwardBySteps(
                    hardwareController->calculateStepsForAngle(10.0), 
                    attemptDelay
                );
                updatePositionAfterMovement(stepsMoved);  // Track the movement
                delay(200);
                Serial.println("[Homing] Moved off home position, now starting forward (clockwise) homing sequence...");
            }
            
            // Start continuous forward rotation for homing (clockwise to reach switch)
            // Enable stepper and set direction once before the loop
            Serial.println("========================================");
            Serial.print("[Homing DEBUG] Attempt ");
            Serial.print(attempt);
            Serial.println(" - Starting continuous forward rotation");
            Serial.println("========================================");
            Serial.print("[Homing DEBUG] Step delay: ");
            Serial.print(attemptDelay);
            Serial.print(" μs, Timeout: ");
            Serial.print(attemptTimeout);
            Serial.println(" ms");
            
            // Enable stepper motor in forward direction (clockwise)
            hardwareController->enableStepperMotor(true);  // true = forward/clockwise
            
            // Check initial switch state
            bool initialSwitchState = sensorManager->isHomePositionSwitchActivated();
            Serial.print("[Homing DEBUG] Initial home switch state: ");
            Serial.println(initialSwitchState ? "ACTIVATED (LOW)" : "NOT ACTIVATED (HIGH)");
            
            // Continuous rotation loop: step motor while waiting for switch activation
            unsigned long startTimeMillis = millis();
            bool homeSwitchActivated = false;
            unsigned long lastLogTime = 0;
            unsigned long stepCount = 0;
            
            Serial.println("[Homing DEBUG] Entering continuous stepping loop...");
            
            while (!sensorManager->isHomePositionSwitchActivated()) {
                // Check for timeout
                unsigned long elapsed = millis() - startTimeMillis;
                if (elapsed > attemptTimeout) {
                    Serial.print("[Homing DEBUG] TIMEOUT after ");
                    Serial.print(elapsed);
                    Serial.print(" ms (");
                    Serial.print(stepCount);
                    Serial.println(" steps generated)");
                    Serial.println("[Homing] TIMEOUT after ");
                    Serial.print(elapsed);
                    Serial.println("ms - home switch never activated");
                    break;
                }
                
                // Generate one step at the specified delay (motor already enabled and direction set)
                hardwareController->rotateStepperForwardContinuous(attemptDelay);
                stepCount++;
                
                // Log progress every 500ms
                if (elapsed - lastLogTime >= 500) {
                    Serial.print("[Homing DEBUG] Stepping... elapsed: ");
                    Serial.print(elapsed);
                    Serial.print(" ms, steps: ");
                    Serial.print(stepCount);
                    Serial.print(", switch: ");
                    Serial.println(sensorManager->isHomePositionSwitchActivated() ? "ACTIVATED" : "NOT ACTIVATED");
                    lastLogTime = elapsed;
                }
            }
            
            Serial.print("[Homing DEBUG] Loop exited. Total steps: ");
            Serial.println(stepCount);
            
            // Check if switch was activated (not timeout)
            if (sensorManager->isHomePositionSwitchActivated()) {
                unsigned long finalElapsed = millis() - startTimeMillis;
                Serial.println("========================================");
                Serial.print("[Homing] *** SUCCESS! *** Switch activated after ");
                Serial.print(finalElapsed);
                Serial.println("ms");
                Serial.println("========================================");
                homeSwitchActivated = true;
            }
            
            // Stop motor
            Serial.print("[Attempt ");
            Serial.print(attempt);
            Serial.println("] Stopping motor...");
            hardwareController->stopMotorCompletely();
            
            if (homeSwitchActivated) {
                // SUCCESS!
                Serial.println("----------------------------------------");
                Serial.print("[Attempt ");
                Serial.print(attempt);
                Serial.println("] SUCCESS! Home switch activated!");
                Serial.println("----------------------------------------");
                
                // Allow time for motor to settle
                Serial.print("[Homing] Settling delay: ");
                Serial.print(systemConfiguration->delayAfterHomingSwitchActivationMilliseconds);
                Serial.println("ms");
                delay(systemConfiguration->delayAfterHomingSwitchActivationMilliseconds);
                
				// Define START at the switch position (no additional offset)
				sensorManager->resetEncoderPositionToZero();
				resetPositionToHome();  // Reset position tracking to 0 steps (HOME)
				isSystemHomedAndReady = true;
                
                Serial.println("========================================");
                Serial.print("Homing completed successfully on attempt ");
                Serial.println(attempt);
                Serial.println("========================================");
                return true;
            }
            
            // This attempt failed
            Serial.println("----------------------------------------");
            Serial.print("[Attempt ");
            Serial.print(attempt);
            Serial.println("] FAILED - timeout reached");
            Serial.println("----------------------------------------");
            
            // If not the last attempt, prepare for retry
            if (attempt < maxAttempts) {
                Serial.print("Waiting 500ms before retry attempt ");
                Serial.print(attempt + 1);
                Serial.println("...");
                delay(500);
                
                // Try a small forward pulse to "reset" if stuck
                Serial.println("[Retry prep] Attempting small forward pulse to clear obstruction...");
                long stepsMoved = hardwareController->moveStepperForwardBySteps(
                    hardwareController->calculateStepsForAngle(5.0), 
                    attemptDelay
                );
                updatePositionAfterMovement(stepsMoved);  // Track the movement
                delay(200);
            }
        }
        
        // All attempts failed
        Serial.println("========================================");
        Serial.println("ERROR: All homing attempts failed!");
        Serial.print("Tried ");
        Serial.print(maxAttempts);
        Serial.println(" attempts with escalating speed");
        Serial.println("========================================");
        isSystemHomedAndReady = false;
        return false;
    }
    
    /**
     * Check if system has been homed and is ready
     * @return true if system is homed
     */
    bool isDispenserSystemHomed() {
        return isSystemHomedAndReady;
    }
    
    /**
     * Force homing if not already homed
     */
    void ensureSystemIsHomed() {
        if (!isSystemHomedAndReady) {
            performHomingWithRetryAndEscalation();
        }
    }
    
    /**
     * Calibration function: Measure full rotation time using homing switch
     * This function:
     * 1. Homes to switch (activates switch)
     * 2. Starts timer
     * 3. Rotates forward 360° (one full rotation)
     * 4. Waits for switch activation again (back at home)
     * 5. Calculates timing data and logs results
     * 
     * @return true if calibration successful, false if timeout or error
     */
    bool calibrateFullRotationTiming() {
        Serial.println("========================================");
        Serial.println("CALIBRATION: Full Rotation Timing");
        Serial.println("========================================");
        Serial.println("This will measure the time for one complete 360° rotation");
        Serial.println("using the homing switch as a reference point.");
        Serial.println("");
        
        // Step 1: Ensure we're at home position
        Serial.println("[Calibration] Step 1: Homing to switch...");
        if (!performHomingWithRetryAndEscalation()) {
            Serial.println("[Calibration] ERROR: Failed to home before calibration");
            return false;
        }
        
        // Verify switch is activated
        if (!sensorManager->isHomePositionSwitchActivated()) {
            Serial.println("[Calibration] ERROR: Switch not activated after homing");
            return false;
        }
        
        Serial.println("[Calibration] Successfully homed - switch is activated");
        delay(500);  // Settle time
        
        // Step 2: Move off switch (backward) so we can detect it again
        Serial.println("[Calibration] Step 2: Moving backward off switch...");
        int stepDelay = systemConfiguration->stepperHomingStepDelayMicroseconds;
        long stepsToMoveOff = hardwareController->calculateStepsForAngle(10.0);
        long stepsMoved = hardwareController->moveStepperBackwardBySteps(stepsToMoveOff, stepDelay);
        updatePositionAfterMovement(stepsMoved);
        
        // Wait a bit and verify switch is not activated
        delay(200);
        if (sensorManager->isHomePositionSwitchActivated()) {
            Serial.println("[Calibration] WARNING: Switch still activated after moving off");
        }
        
        Serial.println("[Calibration] Moved off switch, ready for rotation");
        delay(500);
        
        // Step 3: Start timer and rotate forward 360°
        Serial.println("[Calibration] Step 3: Starting full rotation (360°)...");
        Serial.println("[Calibration] Timer started - rotating forward...");
        
        unsigned long rotationStartTime = millis();
        unsigned long stepCount = 0;
        
        // Calculate steps for 360° rotation
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        long stepsForFullRotation = (long)totalStepsPerRevolution;
        
        Serial.print("[Calibration] Rotating ");
        Serial.print(stepsForFullRotation);
        Serial.print(" steps (360°) at ");
        Serial.print(stepDelay);
        Serial.println(" μs delay");
        
        // Enable motor for forward rotation
        hardwareController->enableStepperMotor(true);  // Forward
        
        // Rotate forward until switch is activated again
        bool switchActivated = false;
        unsigned long lastLogTime = 0;
        
        while (!sensorManager->isHomePositionSwitchActivated()) {
            // Generate step
            hardwareController->rotateStepperForwardContinuous(stepDelay);
            stepCount++;
            
            // Check for timeout (should complete in reasonable time)
            unsigned long elapsed = millis() - rotationStartTime;
            if (elapsed > 30000) {  // 30 second timeout
                Serial.println("[Calibration] ERROR: Timeout - full rotation took too long");
                hardwareController->stopMotorCompletely();
                return false;
            }
            
            // Log progress every second
            if (elapsed - lastLogTime >= 1000) {
                Serial.print("[Calibration] Rotating... elapsed: ");
                Serial.print(elapsed);
                Serial.print(" ms, steps: ");
                Serial.print(stepCount);
                Serial.print(", switch: ");
                Serial.println(sensorManager->isHomePositionSwitchActivated() ? "ACTIVATED" : "NOT ACTIVATED");
                lastLogTime = elapsed;
            }
        }
        
        // Step 4: Switch activated - stop timer
        unsigned long rotationEndTime = millis();
        unsigned long fullRotationTimeMs = rotationEndTime - rotationStartTime;
        
        hardwareController->stopMotorCompletely();
        
        // Step 5: Calculate timing data
        float timePerDegree = fullRotationTimeMs / 360.0;
        float stepsPerSecond = (stepCount * 1000.0) / fullRotationTimeMs;
        
        Serial.println("");
        Serial.println("========================================");
        Serial.println("CALIBRATION RESULTS");
        Serial.println("========================================");
        Serial.print("Full rotation time: ");
        Serial.print(fullRotationTimeMs);
        Serial.print(" ms (");
        Serial.print(fullRotationTimeMs / 1000.0);
        Serial.println(" seconds)");
        Serial.print("Steps taken: ");
        Serial.print(stepCount);
        Serial.print(" steps (expected: ");
        Serial.print(stepsForFullRotation);
        Serial.println(" steps)");
        Serial.print("Time per degree: ");
        Serial.print(timePerDegree);
        Serial.println(" ms/degree");
        Serial.print("Steps per second: ");
        Serial.print(stepsPerSecond);
        Serial.println(" steps/sec");
        Serial.print("Step delay used: ");
        Serial.print(stepDelay);
        Serial.println(" μs");
        Serial.println("");
        
        // Calculate time to each compartment
        Serial.println("Calculated time to each compartment:");
        Serial.println("----------------------------------------");
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            float compartmentAngle = systemConfiguration->containerPositionsInDegrees[i];
            float timeToCompartment = compartmentAngle * timePerDegree;
            
            Serial.print("Compartment ");
            Serial.print(i + 1);
            Serial.print(" (");
            Serial.print(compartmentAngle);
            Serial.print("°): ");
            Serial.print(timeToCompartment);
            Serial.print(" ms (");
            Serial.print(timeToCompartment / 1000.0);
            Serial.println(" seconds)");
        }
        Serial.println("========================================");
        
        // Reset position to home after calibration
        resetPositionToHome();
        
        Serial.println("[Calibration] Calibration complete - position reset to home");
        return true;
    }
    
    // ========================================================================
    // Compartment Movement Operations
    // ========================================================================
    
    /**
     * Move rotary dispenser to specific compartment number
     * @param targetCompartmentNumber Compartment to move to (1-based)
     * @return true if movement successful, false if invalid compartment
     */
    bool moveRotaryDispenserToCompartmentNumber(int targetCompartmentNumber) {
        // Ensure system is homed before movement
        ensureSystemIsHomed();
        
        // Validate compartment number
        if (targetCompartmentNumber < 1 || 
            targetCompartmentNumber > systemConfiguration->numberOfCompartmentsInDispenser) {
            Serial.print("ERROR: Invalid compartment number: ");
            Serial.println(targetCompartmentNumber);
            return false;
        }
        
        Serial.println("========================================");
        Serial.print("POSITIONING: Moving to compartment ");
        Serial.println(targetCompartmentNumber);
        Serial.println("========================================");
        
        // If already at target, no need to move
        if (currentCompartmentNumber == targetCompartmentNumber) {
            Serial.print("Already at compartment ");
            Serial.print(targetCompartmentNumber);
            Serial.println(" - no movement needed");
            return true;
        }
        
        // EXACT STEP-BASED POSITIONING:
        // Use absolute step positions to move to exact compartment location
        
        // Get target absolute step position for the compartment
        long targetStepPosition = compartmentStepPositions[targetCompartmentNumber - 1];
        
        // Get current absolute step position
        long currentStepPosition = currentPositionSteps;
        
        Serial.println("========================================");
        Serial.println("[Positioning] EXACT STEP-BASED POSITIONING");
        Serial.println("========================================");
        Serial.print("[Positioning] CURRENT: ");
        Serial.print(currentStepPosition);
        Serial.print(" steps (");
        Serial.print(getCurrentPositionDegrees());
        Serial.print("°)");
        if (currentCompartmentNumber == 0) {
            Serial.print(" - HOME");
        } else {
            Serial.print(" - Compartment ");
            Serial.print(currentCompartmentNumber);
        }
        Serial.println();
        
        Serial.print("[Positioning] TARGET: ");
        Serial.print(targetStepPosition);
        Serial.print(" steps (");
        Serial.print(systemConfiguration->containerPositionsInDegrees[targetCompartmentNumber - 1]);
        Serial.print("°) - Compartment ");
        Serial.println(targetCompartmentNumber);
        
        // Calculate steps needed to move (target - current)
        long stepsToMove = targetStepPosition - currentStepPosition;
        
        // Calculate total steps per revolution for wraparound handling
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        
        // Handle wraparound - choose shortest path
        // If movement is more than half a revolution, go the other way
        if (abs(stepsToMove) > (totalStepsPerRevolution / 2)) {
            if (stepsToMove > 0) {
                stepsToMove -= (long)totalStepsPerRevolution;  // Go backward instead
            } else {
                stepsToMove += (long)totalStepsPerRevolution;  // Go forward instead
            }
        }
        
        Serial.print("[Positioning] MOVEMENT: ");
        Serial.print(abs(stepsToMove));
        Serial.print(" steps ");
        Serial.print((stepsToMove > 0) ? "FORWARD" : "BACKWARD");
        Serial.print(" (");
        Serial.print((abs(stepsToMove) / totalStepsPerRevolution) * 360.0);
        Serial.println("°)");
        
        // If already at target position (within tolerance), no movement needed
        if (abs(stepsToMove) < 5) {  // 5 steps tolerance
            Serial.println("[Positioning] Already at target position (within tolerance)");
            currentCompartmentNumber = targetCompartmentNumber;
            return true;
        }
        
        // Get step delay for movement
        int stepDelay = systemConfiguration->stepperRunningStepDelayMicroseconds;
        Serial.print("[Positioning] Step delay: ");
        Serial.print(stepDelay);
        Serial.println(" μs");
        
        Serial.println("[Positioning] Moving to exact step position...");
        
        // Move exact number of steps
        long stepsMoved = 0;
        if (stepsToMove > 0) {
            // Move forward
            stepsMoved = hardwareController->moveStepperForwardBySteps(stepsToMove, stepDelay);
        } else {
            // Move backward
            stepsMoved = hardwareController->moveStepperBackwardBySteps(abs(stepsToMove), stepDelay);
        }
        
        // Update position tracking
        updatePositionAfterMovement(stepsMoved);
        
        Serial.println("[Positioning] Motor movement complete");
        
        // Allow settling time for plate to stabilize
        Serial.print("[Positioning] Settling for ");
        Serial.print(systemConfiguration->delayAfterCompartmentMoveMilliseconds);
        Serial.println("ms...");
        delay(systemConfiguration->delayAfterCompartmentMoveMilliseconds);
        
        // UPDATE CURRENT POSITION
        currentCompartmentNumber = targetCompartmentNumber;
        
        Serial.print("POSITIONED: Now at Container ");
        Serial.print(currentCompartmentNumber);
        Serial.print(" (");
        Serial.print(currentPositionSteps);
        Serial.print(" steps, ");
        Serial.print(getCurrentPositionDegrees());
        Serial.println("°)");
        Serial.println("========================================");
        
        return true;
    }
    
    /**
     * Get current compartment position
     * @return Current compartment number (0 = home/unknown)
     */
    int getCurrentCompartmentNumber() {
        return currentCompartmentNumber;
    }
    
    // ========================================================================
    // Pill Dispensing Operations
    // ========================================================================
    
    /**
     * Attempt to dispense a single pill with multiple retry attempts
     * @return true if pill successfully dispensed and detected, false otherwise
     */
    bool attemptToDispenseSinglePillWithRetries() {
        Serial.println("Beginning pill dispense operation...");
        
        int maxAttempts = systemConfiguration->maximumDispenseAttempts;
        
        for (int attemptNumber = 1; attemptNumber <= maxAttempts; attemptNumber++) {
            Serial.print("Dispense attempt ");
            Serial.print(attemptNumber);
            Serial.print(" of ");
            Serial.println(maxAttempts);
            
            // Step 1: Activate electromagnet to pick up pill
            hardwareController->activateElectromagnetAndWaitForStabilization();
            
            // Step 2: Move servo to dispensing position
            hardwareController->moveServoToDispensingPositionAndWait();
            
            // Step 3: Check if pill was detected
            bool pillDetected = sensorManager->waitForPillDetectionWithTimeout();
            
            if (pillDetected) {
                Serial.println("SUCCESS: Pill dispensed and detected!");
                
                // Return servo to rest position
                hardwareController->moveServoToRestPositionAndWait();
                
                // Deactivate electromagnet
                hardwareController->deactivateElectromagnetWithDelay();
                
                return true;
            }
            
            // Pill not detected - reset for next attempt
            Serial.print("Attempt ");
            Serial.print(attemptNumber);
            Serial.println(" failed - no pill detected");
            
            // Return servo to rest position
            hardwareController->moveServoToRestPositionAndWait();
            
            // Deactivate electromagnet
            hardwareController->deactivateElectromagnetWithDelay();
            
            // Wait before next attempt
            if (attemptNumber < maxAttempts) {
                delay(systemConfiguration->delayBetweenDispenseAttemptsMilliseconds);
            }
        }
        
        return false;
    }
    
    /**
     * Dispense pills from a specific compartment
     * @param compartmentNumber Target compartment (1-based)
     * @param numberOfPillsToDispense How many pills to dispense
     * @return Number of pills successfully dispensed
     */
    int dispensePillsFromCompartment(int compartmentNumber, int numberOfPillsToDispense) {
        // Move to target compartment
        if (!moveRotaryDispenserToCompartmentNumber(compartmentNumber)) {
            return 0;  // Failed to move to compartment
        }
        
        int successfulDispenseCount = 0;
        
        // Attempt to dispense requested number of pills
        for (int pillNumber = 0; pillNumber < numberOfPillsToDispense; pillNumber++) {
            if (attemptToDispenseSinglePillWithRetries()) {
                successfulDispenseCount++;
                
                // Update statistics
                if (compartmentNumber >= 1 && 
                    compartmentNumber <= systemConfiguration->numberOfCompartmentsInDispenser) {
                    dispensedCountForEachCompartment[compartmentNumber - 1]++;
                }
            }
            
            // Delay between multiple pills
            if (pillNumber < numberOfPillsToDispense - 1) {
                delay(systemConfiguration->delayBetweenMultipleDispensesMilliseconds);
            }
        }
        
        Serial.print("Dispensed ");
        Serial.print(successfulDispenseCount);
        Serial.print(" of ");
        Serial.print(numberOfPillsToDispense);
        Serial.println(" requested pills");
        
        // AUTO-HOME after successful dispense (if enabled)
        if (systemConfiguration->autoHomeAfterDispense && successfulDispenseCount > 0) {
            Serial.println("========================================");
            Serial.println("[Auto-Home] Pill(s) dispensed successfully");
            Serial.println("[Auto-Home] Returning to home position...");
            Serial.println("========================================");
            
            bool homingSuccess = performHomingWithRetryAndEscalation();
            
            if (homingSuccess) {
                Serial.println("[Auto-Home] Successfully returned to home position");
            } else {
                Serial.println("[Auto-Home] WARNING: Homing failed! Position may be inaccurate.");
                Serial.println("[Auto-Home] Press Button 6 to manually re-home.");
            }
        }
        
        return successfulDispenseCount;
    }
    
    // ========================================================================
    // Statistics and Status Operations
    // ========================================================================
    
    /**
     * Get dispense count for a specific compartment
     * @param compartmentNumber Target compartment (1-based)
     * @return Number of pills dispensed from that compartment
     */
    int getDispenseCountForCompartment(int compartmentNumber) {
        if (compartmentNumber >= 1 && 
            compartmentNumber <= systemConfiguration->numberOfCompartmentsInDispenser) {
            return dispensedCountForEachCompartment[compartmentNumber - 1];
        }
        return 0;
    }
    
    /**
     * Reset all dispense statistics to zero
     */
    void resetAllDispenseStatistics() {
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            dispensedCountForEachCompartment[i] = 0;
        }
        Serial.println("All dispense statistics reset to zero");
    }
    
    /**
     * Get total number of pills dispensed across all compartments
     * @return Total dispense count
     */
    int getTotalDispenseCount() {
        int total = 0;
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            total += dispensedCountForEachCompartment[i];
        }
        return total;
    }
    
    /**
     * Print current statistics to Serial
     */
    void printDispenserStatistics() {
        Serial.println("=== Dispenser Statistics ===");
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            Serial.print("Compartment ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(dispensedCountForEachCompartment[i]);
            Serial.println(" pills");
        }
        Serial.print("Total: ");
        Serial.print(getTotalDispenseCount());
        Serial.println(" pills");
        Serial.println("===========================");
    }
};

#endif // DISPENSER_CONTROLLER_H

