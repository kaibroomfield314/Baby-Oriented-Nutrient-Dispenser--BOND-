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
        
        // Initialize dispense counters
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            dispensedCountForEachCompartment[i] = 0;
        }
    }
    
    /**
     * Initialize the dispenser controller
     */
    void initializeDispenserSystem() {
        Serial.println("Dispenser controller initialized");
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
        int baseSpeed = systemConfiguration->motorHomingSpeedPWM;
        int speedIncrement = systemConfiguration->homingSpeedIncrementPerRetry;
        int baseTimeout = MAXIMUM_HOMING_TIMEOUT_MILLISECONDS;
        int timeoutIncrement = systemConfiguration->homingTimeoutIncrementPerRetry;
        
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            Serial.println("----------------------------------------");
            Serial.print("Homing Attempt ");
            Serial.print(attempt);
            Serial.print(" of ");
            Serial.println(maxAttempts);
            Serial.println("----------------------------------------");
            
            // Calculate speed for this attempt (escalating)
            int attemptSpeed = baseSpeed + ((attempt - 1) * speedIncrement);
            attemptSpeed = constrain(attemptSpeed, 0, 255);  // Keep within valid PWM range
            
            // Calculate timeout for this attempt (increasing with each retry)
            unsigned long attemptTimeout = baseTimeout + ((attempt - 1) * timeoutIncrement);
            
            Serial.print("[Attempt ");
            Serial.print(attempt);
            Serial.print("] Motor speed: ");
            Serial.print(attemptSpeed);
            Serial.print(" PWM | Timeout: ");
            Serial.print(attemptTimeout);
            Serial.println("ms");
            
			// Pre-check: Are we already at home? (only if system was previously homed)
			if (isSystemHomedAndReady && sensorManager->isHomePositionSwitchActivated()) {
				Serial.println("[Homing] Already at home switch - defining START here");
				sensorManager->resetEncoderPositionToZero();
				currentCompartmentNumber = 0; // At START (home)
				Serial.println("========================================");
				Serial.println("Homing completed successfully!");
				Serial.println("========================================");
				return true;
			}
            
            // If switch is pressed at startup, move off it first
            if (sensorManager->isHomePositionSwitchActivated()) {
                Serial.println("[Homing] Switch is pressed at startup - moving off home position first...");
                hardwareController->setMotorToRotateBackwardAtSpeed(attemptSpeed);
                delay(500);  // Move backward for 500ms to get off the switch
                hardwareController->stopMotorCompletely();
                delay(200);
                Serial.println("[Homing] Moved off home position, now starting forward homing sequence...");
            }
            
            // Start motor at calculated speed
            hardwareController->runMotorAtCustomHomingSpeed(attemptSpeed);
            Serial.print("[Attempt ");
            Serial.print(attempt);
            Serial.println("] Motor activated - checking for movement...");
            
            // Small delay to allow motor to start
            delay(50);
            
            // Wait for home switch activation with timeout
            bool homeSwitchActivated = sensorManager->waitForHomeSwitchActivationWithTimeout(attemptTimeout);
            
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
				currentCompartmentNumber = 0; // At START (home)
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
                
                // Try a small backward pulse to "reset" if stuck
                Serial.println("[Retry prep] Attempting small backward pulse to clear obstruction...");
                hardwareController->setMotorToRotateBackwardAtSpeed(attemptSpeed);
                delay(100);
                hardwareController->stopMotorCompletely();
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
        
        // POSITIONING CALCULATION:
        // START position = 0° (defined by home switch during homing)
        // Each container has a fixed position stored in containerPositionsInDegrees array
        
        // Get ABSOLUTE position of target container from array
        // Array is 0-indexed, so Container 1 = index 0, Container 2 = index 1, etc.
        float targetAbsoluteAngle = systemConfiguration->containerPositionsInDegrees[targetCompartmentNumber - 1];
        
        // Get ABSOLUTE position of current location
        float currentAbsoluteAngle = (currentCompartmentNumber == 0) ? 0.0 : 
                                     systemConfiguration->containerPositionsInDegrees[currentCompartmentNumber - 1];
        
        // Calculate RELATIVE movement needed (target - current)
        float angleToTravel = targetAbsoluteAngle - currentAbsoluteAngle;
        
        // Handle wraparound (always go forward)
        if (angleToTravel < 0) {
            angleToTravel += 360.0;
        }
        
        Serial.print("[Positioning] CURRENT: ");
        if (currentCompartmentNumber == 0) {
            Serial.print("START/HOME (0° absolute)");
        } else {
            Serial.print("Container ");
            Serial.print(currentCompartmentNumber);
            Serial.print(" (");
            Serial.print(currentAbsoluteAngle);
            Serial.print("° absolute = START + ");
            Serial.print(currentAbsoluteAngle);
            Serial.print("° offset)");
        }
        Serial.println();
        
        Serial.print("[Positioning] TARGET: Container ");
        Serial.print(targetCompartmentNumber);
        Serial.print(" (");
        Serial.print(targetAbsoluteAngle);
        Serial.print("° absolute = START + ");
        Serial.print(targetAbsoluteAngle);
        Serial.println("° offset)");
        
        Serial.print("[Positioning] MOVEMENT: Rotate ");
        Serial.print(angleToTravel);
        Serial.print("° forward from current position");
        Serial.println();
        
        // Calculate movement time based on angle
        // This is calibrated based on motor speed and mechanical system
        // Formula: time = (angle / degrees_per_second)
        // Default estimate: ~180 degrees per second (MUST be calibrated for your hardware!)
        float estimatedDegreesPerSecond = systemConfiguration->estimatedMotorDegreesPerSecond;
        unsigned long movementTimeMs = (unsigned long)((angleToTravel / estimatedDegreesPerSecond) * 1000.0);
        
        // Apply minimum and maximum movement times for safety
        movementTimeMs = constrain(movementTimeMs, 100, 5000);  // 100ms to 5s
        
        Serial.print("[Positioning] Estimated movement time: ");
        Serial.print(movementTimeMs);
        Serial.println("ms");
        Serial.print("[Positioning] Motor speed: ");
        Serial.print(systemConfiguration->motorRunningSpeedPWM);
        Serial.println(" PWM");
        
        // Start motor at normal running speed
        hardwareController->runMotorAtNormalSpeed();
        Serial.println("[Positioning] Motor started - rotating forward...");
        
        // Move for calculated time
        delay(movementTimeMs);
        
        // Stop motor
        hardwareController->stopMotorCompletely();
        Serial.println("[Positioning] Motor stopped");
        
        // Allow settling time for plate to stabilize
        Serial.print("[Positioning] Settling for ");
        Serial.print(systemConfiguration->delayAfterCompartmentMoveMilliseconds);
        Serial.println("ms...");
        delay(systemConfiguration->delayAfterCompartmentMoveMilliseconds);
        
        // UPDATE CURRENT POSITION - This is crucial!
        // All future movements will calculate from THIS position, not from START
        currentCompartmentNumber = targetCompartmentNumber;
        
        Serial.print("POSITIONED: Now at Container ");
        Serial.print(currentCompartmentNumber);
        Serial.print(" (");
        Serial.print(targetAbsoluteAngle);
        Serial.println("°)");
        
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

