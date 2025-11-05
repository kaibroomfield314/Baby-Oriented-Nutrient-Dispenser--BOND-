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
    }
    
    /**
     * Calculate compartment step positions from degree positions
     * Converts containerPositionsInDegrees[] to absolute step positions
     */
    void calculateCompartmentStepPositions() {
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            float angleInDegrees = systemConfiguration->containerPositionsInDegrees[i];
            compartmentStepPositions[i] = (long)((angleInDegrees / 360.0) * totalStepsPerRevolution);
        }
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
        currentPositionSteps = 0;
        currentCompartmentNumber = 0;
    }
    
    /**
     * Update position tracking after movement
     * @param stepsMoved Number of steps moved (positive = forward, negative = backward)
     */
    void updatePositionAfterMovement(long stepsMoved) {
        currentPositionSteps += stepsMoved;
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
        hardwareController->moveServoToRestPositionAndWait();
        
        int maxAttempts = systemConfiguration->homingRetryAttempts;
        int basePulseWidth = systemConfiguration->stepperStepPulseWidthMicroseconds;
        int baseDelay = basePulseWidth * 2;
        int delayDecrement = systemConfiguration->homingDelayDecrementPerRetry;
        int baseTimeout = MAXIMUM_HOMING_TIMEOUT_MILLISECONDS;
        int timeoutIncrement = systemConfiguration->homingTimeoutIncrementPerRetry;
        
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            int attemptDelay = baseDelay - ((attempt - 1) * delayDecrement);
            int minPulseWidth = systemConfiguration->stepperMinStepPulseWidthMicroseconds;
            int minDelay = minPulseWidth * 2;
            attemptDelay = constrain(attemptDelay, minDelay, baseDelay);
            
            unsigned long attemptTimeout = baseTimeout + ((attempt - 1) * timeoutIncrement);
            
			if (isSystemHomedAndReady && sensorManager->isHomePositionSwitchActivated()) {
				sensorManager->resetEncoderPositionToZero();
				resetPositionToHome();
				return true;
			}
            
            if (sensorManager->isHomePositionSwitchActivated()) {
                long stepsMoved = hardwareController->moveStepperBackwardBySteps(
                    hardwareController->calculateStepsForAngle(10.0), 
                    attemptDelay
                );
                updatePositionAfterMovement(stepsMoved);
                delay(200);
            }
            
            hardwareController->enableStepperMotor(true);
            
            unsigned long startTimeMillis = millis();
            bool homeSwitchActivated = false;
            
            while (!sensorManager->isHomePositionSwitchActivated()) {
                if (millis() - startTimeMillis > attemptTimeout) {
                    Serial.println("ERROR: Homing timeout");
                    break;
                }
                
                hardwareController->rotateStepperForwardContinuous(attemptDelay);
            }
            
            if (sensorManager->isHomePositionSwitchActivated()) {
                homeSwitchActivated = true;
            }
            
            hardwareController->stopMotorCompletely();
            
            if (homeSwitchActivated) {
                delay(systemConfiguration->delayAfterHomingSwitchActivationMilliseconds);
                
				sensorManager->resetEncoderPositionToZero();
				resetPositionToHome();
				isSystemHomedAndReady = true;
                
                return true;
            }
            
            if (attempt < maxAttempts) {
                delay(500);
                
                long stepsMoved = hardwareController->moveStepperForwardBySteps(
                    hardwareController->calculateStepsForAngle(5.0), 
                    attemptDelay
                );
                updatePositionAfterMovement(stepsMoved);
                delay(200);
            }
        }
        
        Serial.println("ERROR: All homing attempts failed");
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
    
    bool calibrateFullRotationTiming() {
        if (!performHomingWithRetryAndEscalation()) {
            Serial.println("ERROR: Failed to home before calibration");
            return false;
        }
        
        if (!sensorManager->isHomePositionSwitchActivated()) {
            Serial.println("ERROR: Switch not activated after homing");
            return false;
        }
        
        delay(500);
        
        int stepDelay = systemConfiguration->stepperStepPulseWidthMicroseconds * 2;
        long stepsToMoveOff = hardwareController->calculateStepsForAngle(10.0);
        long stepsMoved = hardwareController->moveStepperBackwardBySteps(stepsToMoveOff, stepDelay);
        updatePositionAfterMovement(stepsMoved);
        
        delay(200);
        delay(500);
        
        unsigned long rotationStartTime = millis();
        unsigned long stepCount = 0;
        
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        long stepsForFullRotation = (long)totalStepsPerRevolution;
        
        hardwareController->enableStepperMotor(true);
        
        while (!sensorManager->isHomePositionSwitchActivated()) {
            hardwareController->rotateStepperForwardContinuous(stepDelay);
            stepCount++;
            
            if (millis() - rotationStartTime > 30000) {
                Serial.println("ERROR: Calibration timeout");
                hardwareController->stopMotorCompletely();
                return false;
            }
        }
        
        unsigned long rotationEndTime = millis();
        unsigned long fullRotationTimeMs = rotationEndTime - rotationStartTime;
        
        hardwareController->stopMotorCompletely();
        
        float timePerDegree = fullRotationTimeMs / 360.0;
        
        Serial.println("CALIBRATION RESULTS:");
        Serial.print("Full rotation: ");
        Serial.print(fullRotationTimeMs);
        Serial.print(" ms (");
        Serial.print(fullRotationTimeMs / 1000.0);
        Serial.println(" seconds)");
        Serial.print("Time per degree: ");
        Serial.print(timePerDegree);
        Serial.println(" ms/degree");
        
        for (int i = 0; i < systemConfiguration->numberOfCompartmentsInDispenser; i++) {
            float compartmentAngle = systemConfiguration->containerPositionsInDegrees[i];
            float timeToCompartment = compartmentAngle * timePerDegree;
            
            Serial.print("Compartment ");
            Serial.print(i + 1);
            Serial.print(" (");
            Serial.print(compartmentAngle);
            Serial.print("°): ");
            Serial.print(timeToCompartment);
            Serial.println(" ms");
        }
        
        resetPositionToHome();
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
        ensureSystemIsHomed();
        
        if (targetCompartmentNumber < 1 || 
            targetCompartmentNumber > systemConfiguration->numberOfCompartmentsInDispenser) {
            Serial.println("ERROR: Invalid compartment number");
            return false;
        }
        
        if (currentCompartmentNumber == targetCompartmentNumber) {
            return true;
        }
        
        long targetStepPosition = compartmentStepPositions[targetCompartmentNumber - 1];
        long currentStepPosition = currentPositionSteps;
        long stepsToMove = targetStepPosition - currentStepPosition;
        
        float totalStepsPerRevolution = systemConfiguration->stepperStepsPerRevolution * 
                                        systemConfiguration->stepperMicrostepping * 
                                        systemConfiguration->stepperGearRatio;
        
        if (abs(stepsToMove) > (totalStepsPerRevolution / 2)) {
            if (stepsToMove > 0) {
                stepsToMove -= (long)totalStepsPerRevolution;
            } else {
                stepsToMove += (long)totalStepsPerRevolution;
            }
        }
        
        if (abs(stepsToMove) < 5) {
            currentCompartmentNumber = targetCompartmentNumber;
            return true;
        }
        
        int stepDelay = systemConfiguration->stepperStepPulseWidthMicroseconds * 2;
        
        long stepsMoved = 0;
        if (stepsToMove > 0) {
            stepsMoved = hardwareController->moveStepperForwardBySteps(stepsToMove, stepDelay);
        } else {
            stepsMoved = hardwareController->moveStepperBackwardBySteps(abs(stepsToMove), stepDelay);
        }
        
        updatePositionAfterMovement(stepsMoved);
        delay(systemConfiguration->delayAfterCompartmentMoveMilliseconds);
        
        currentCompartmentNumber = targetCompartmentNumber;
        
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
     * Attempt to dispense pills and count IR sensor activations
     * @return Number of pills detected (IR sensor activations) during the dispensing attempt
     */
    int attemptToDispenseAndCountPills() {
        int maxAttempts = systemConfiguration->maximumDispenseAttempts;
        
        for (int attemptNumber = 1; attemptNumber <= maxAttempts; attemptNumber++) {
            hardwareController->activateElectromagnetAndWaitForStabilization();
            
            int startPosition = hardwareController->getCurrentServoPosition();
            hardwareController->moveServoFromCurrentToMax();
            
            unsigned long waitStartTime = millis();
            const unsigned long waitDurationMs = 2000;
            int checkIntervalMs = systemConfiguration->pillDetectionCheckIntervalMilliseconds;
            int pillCount = 0;
            bool lastSensorState = false;
            bool currentSensorState = false;
            
            delay(50);
            lastSensorState = sensorManager->isPillCurrentlyDetectedByInfraredSensor();
            
            while (millis() - waitStartTime < waitDurationMs) {
                currentSensorState = sensorManager->isPillCurrentlyDetectedByInfraredSensor();
                
                if (!lastSensorState && currentSensorState) {
                    pillCount++;
                }
                
                lastSensorState = currentSensorState;
                delay(checkIntervalMs);
            }
            
            hardwareController->moveServoToMicroseconds(startPosition);
            delay(systemConfiguration->servoMovementDelayMilliseconds);
            
            if (pillCount > 0) {
                hardwareController->deactivateElectromagnetWithDelay();
                return pillCount;
            }
            
            hardwareController->deactivateElectromagnetWithDelay();
            
            if (attemptNumber < maxAttempts) {
                delay(systemConfiguration->delayBetweenDispenseAttemptsMilliseconds);
            }
        }
        
        return 0;
    }
    
    /**
     * Dispense pills from a specific compartment
     * @param compartmentNumber Target compartment (1-based)
     * @param numberOfPillsToDispense How many pills to dispense
     * @return Number of pills successfully dispensed (counted by IR sensor)
     */
    int dispensePillsFromCompartment(int compartmentNumber, int numberOfPillsToDispense) {
        // Move to target compartment
        if (!moveRotaryDispenserToCompartmentNumber(compartmentNumber)) {
            return 0;  // Failed to move to compartment
        }
        
        int totalPillsDetected = 0;
        
        // Attempt to dispense requested number of pills
        for (int pillNumber = 0; pillNumber < numberOfPillsToDispense; pillNumber++) {
            int pillsDetected = attemptToDispenseAndCountPills();
            
            if (pillsDetected > 0) {
                totalPillsDetected += pillsDetected;
                
                // Update statistics for each pill detected
                for (int i = 0; i < pillsDetected; i++) {
                    if (compartmentNumber >= 1 && 
                        compartmentNumber <= systemConfiguration->numberOfCompartmentsInDispenser) {
                        dispensedCountForEachCompartment[compartmentNumber - 1]++;
                    }
                }
            }
            
            // Delay between multiple pills
            if (pillNumber < numberOfPillsToDispense - 1) {
                delay(systemConfiguration->delayBetweenMultipleDispensesMilliseconds);
            }
        }
        
        if (systemConfiguration->autoHomeAfterDispense && totalPillsDetected > 0) {
            performHomingWithRetryAndEscalation();
        }
        
        return totalPillsDetected;
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
    
    void printDispenserStatistics() {
    }
};

#endif // DISPENSER_CONTROLLER_H

