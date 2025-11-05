#ifndef HARDWARE_CONTROLLER_H
#define HARDWARE_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "Config.h"
#include "ConfigurationSettings.h"

/**
 * HardwareController Class
 * 
 * Responsible for all hardware actuation including:
 * - Stepper motor control (precise positioning and continuous rotation)
 * - Servo motor positioning
 * - Electromagnet activation
 * 
 * This class provides low-level hardware control without knowledge of
 * higher-level operations like "dispensing" or "homing" (low coupling).
 */
class HardwareController {
private:
    SystemConfiguration* systemConfiguration;
    Servo dispenserServoMotor;
    bool isElectromagnetCurrentlyActivated;
    
public:
    /**
     * Constructor
     * @param config Pointer to system configuration
     */
    HardwareController(SystemConfiguration* config) {
        systemConfiguration = config;
        isElectromagnetCurrentlyActivated = false;
    }
    
    void initializeAllHardwareActuators() {
        pinMode(PIN_FOR_STEPPER_DIR, OUTPUT);
        pinMode(PIN_FOR_STEPPER_STEP, OUTPUT);
        pinMode(PIN_FOR_STEPPER_EN, OUTPUT);
        
        digitalWrite(PIN_FOR_STEPPER_DIR, LOW);
        digitalWrite(PIN_FOR_STEPPER_STEP, LOW);
        digitalWrite(PIN_FOR_STEPPER_EN, HIGH);
        
        resetStepCounter();
        
        pinMode(PIN_FOR_ELECTROMAGNET_CONTROL, OUTPUT);
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, LOW);
        
        pinMode(PIN_FOR_GREEN_STATUS_LED, OUTPUT);
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, LOW);
        
        int minSafe = getServoMinSafe();
        int maxSafe = getServoMaxSafe();
        dispenserServoMotor.attach(PIN_FOR_SERVO_MOTOR_SIGNAL, minSafe, maxSafe);
        moveServoToRestPosition();
    }
    
    // ========================================================================
    // Stepper Motor Control Methods
    // ========================================================================
    
    /**
     * Validate and constrain step pulse width to safe limits
     * @param pulseWidthMicroseconds Requested pulse width in microseconds
     * @return Validated pulse width within safe limits
     */
    int validateStepPulseWidth(int pulseWidthMicroseconds) {
        int minWidth = systemConfiguration->stepperMinStepPulseWidthMicroseconds;
        int maxWidth = systemConfiguration->stepperMaxStepPulseWidthMicroseconds;
        return constrain(pulseWidthMicroseconds, minWidth, maxWidth);
    }
    
    int validateStepDelay(int stepDelayMicroseconds) {
        int pulseWidth = stepDelayMicroseconds / 2;
        int validatedWidth = validateStepPulseWidth(pulseWidth);
        return validatedWidth * 2;
    }
    
    /**
     * Calculate number of steps needed for a given angle in degrees
     * @param angleInDegrees Target angle (can be > 360 for multiple rotations)
     * @return Number of steps needed
     */
    long calculateStepsForAngle(float angleInDegrees) {
        int baseSteps = systemConfiguration->stepperStepsPerRevolution;
        int microstepping = systemConfiguration->stepperMicrostepping;
        float gearRatio = systemConfiguration->stepperGearRatio;
        float totalStepsPerRevolution = baseSteps * microstepping * gearRatio;
        return (long)((angleInDegrees / 360.0) * totalStepsPerRevolution);
    }
    
    static unsigned long stepPulseCount;
    
    void generateStepPulse() {
        stepPulseCount++;
        int stepPulseWidth = systemConfiguration->stepperStepPulseWidthMicroseconds;
        
        digitalWrite(PIN_FOR_STEPPER_STEP, HIGH);
        delayMicroseconds(stepPulseWidth);
        digitalWrite(PIN_FOR_STEPPER_STEP, LOW);
        delayMicroseconds(stepPulseWidth);
    }
    
    void resetStepCounter() {
        stepPulseCount = 0;
    }
    
    void enableStepperMotor(bool directionForward) {
        int dirPinState = directionForward ? HIGH : LOW;
        digitalWrite(PIN_FOR_STEPPER_DIR, dirPinState);
        delayMicroseconds(5);
        
        digitalWrite(PIN_FOR_STEPPER_EN, LOW);
        delayMicroseconds(10);
    }
    
    void stepWithDelay(int stepDelay) {
        generateStepPulse();
    }
    
    void rotateStepperForwardByAngle(float angleInDegrees, int stepDelayMicroseconds) {
        resetStepCounter();
        enableStepperMotor(true);
        
        long steps = calculateStepsForAngle(angleInDegrees);
        
        for (long i = 0; i < steps; i++) {
            generateStepPulse();
        }
        
        delayMicroseconds(100);
    }
    
    long moveStepperForwardBySteps(long steps, int stepDelayMicroseconds) {
        if (steps <= 0) {
            return 0;
        }
        
        resetStepCounter();
        enableStepperMotor(true);
        
        for (long i = 0; i < steps; i++) {
            generateStepPulse();
        }
        
        delayMicroseconds(100);
        return steps;
    }
    
    long moveStepperBackwardBySteps(long steps, int stepDelayMicroseconds) {
        if (steps <= 0) {
            return 0;
        }
        
        resetStepCounter();
        enableStepperMotor(false);
        
        for (long i = 0; i < steps; i++) {
            generateStepPulse();
        }
        
        delayMicroseconds(100);
        return -steps;
    }
    
    void rotateStepperBackwardByAngle(float angleInDegrees, int stepDelayMicroseconds) {
        resetStepCounter();
        enableStepperMotor(false);
        
        long steps = calculateStepsForAngle(angleInDegrees);
        
        for (long i = 0; i < steps; i++) {
            generateStepPulse();
        }
        
        delayMicroseconds(100);
    }
    
    void rotateStepperForwardContinuous(int stepDelayMicroseconds) {
        int validatedDelay = validateStepDelay(stepDelayMicroseconds);
        stepWithDelay(validatedDelay);
    }
    
    void rotateStepperBackwardContinuous(int stepDelayMicroseconds) {
        int validatedDelay = validateStepDelay(stepDelayMicroseconds);
        stepWithDelay(validatedDelay);
    }
    
    void stopMotorCompletely() {
        digitalWrite(PIN_FOR_STEPPER_EN, HIGH);
        digitalWrite(PIN_FOR_STEPPER_STEP, LOW);
    }
    
    void setMotorToRotateForwardAtSpeed(int stepDelayMicroseconds) {
        enableStepperMotor(true);
        rotateStepperForwardContinuous(stepDelayMicroseconds);
    }
    
    void setMotorToRotateBackwardAtSpeed(int stepDelayMicroseconds) {
        enableStepperMotor(false);
        rotateStepperBackwardContinuous(stepDelayMicroseconds);
    }
    
    void runMotorAtHomingSpeed() {
        enableStepperMotor(true);
        rotateStepperForwardContinuous(systemConfiguration->stepperHomingStepDelayMicroseconds);
    }
    
    void runMotorAtCustomHomingSpeed(int stepDelayMicroseconds) {
        enableStepperMotor(true);
        rotateStepperForwardContinuous(stepDelayMicroseconds);
    }
    
    void runMotorAtNormalSpeed() {
        enableStepperMotor(true);
        rotateStepperForwardContinuous(systemConfiguration->stepperRunningStepDelayMicroseconds);
    }
    
    // ========================================================================
    // Servo Control Methods
    // ========================================================================
    
    /**
     * Get calculated safe servo endpoints
     */
    int getServoMinSafe() {
        return systemConfiguration->servoMinMicroseconds + systemConfiguration->servoEndMarginMicroseconds;
    }
    
    int getServoMaxSafe() {
        return systemConfiguration->servoMaxMicroseconds - systemConfiguration->servoEndMarginMicroseconds;
    }
    
    void moveServoToMicroseconds(int targetMicroseconds) {
        int minSafe = getServoMinSafe();
        int maxSafe = getServoMaxSafe();
        
        // Constrain to safe range
        targetMicroseconds = constrain(targetMicroseconds, minSafe, maxSafe);
        
        // Ensure servo is attached
        if (!dispenserServoMotor.attached()) {
            dispenserServoMotor.attach(PIN_FOR_SERVO_MOTOR_SIGNAL, minSafe, maxSafe);
        }
        
        // Get current position
        int current = dispenserServoMotor.readMicroseconds();
        if (current < minSafe || current > maxSafe) {
            current = minSafe;  // Clamp to known good start
            dispenserServoMotor.writeMicroseconds(current);
            delay(5);
        }
        
        // Determine step direction and size
        int step = (targetMicroseconds >= current) ? systemConfiguration->servoStepMicroseconds : -systemConfiguration->servoStepMicroseconds;
        
        // Move in steps to target position
        int us = current;
        while ((step > 0 && us < targetMicroseconds) || (step < 0 && us > targetMicroseconds)) {
            dispenserServoMotor.writeMicroseconds(us);
            delay(systemConfiguration->servoStepDelayMilliseconds);
            
            // Check if next step would overshoot
            int nextUs = us + step;
            if ((step > 0 && nextUs > targetMicroseconds) || (step < 0 && nextUs < targetMicroseconds)) {
                break;  // Stop before overshooting
            }
            us = nextUs;
        }
        
        // Ensure we end exactly at target position (in case step size didn't divide evenly)
        dispenserServoMotor.writeMicroseconds(targetMicroseconds);
        delay(systemConfiguration->servoStepDelayMilliseconds);
        
    }
    
    /**
     * Move servo to rest/home position (minimum position)
     */
    void moveServoToRestPosition() {
        int minSafe = getServoMinSafe();
        moveServoToMicroseconds(minSafe);
    }
    
    void performServoHomingSequence() {
        if (isElectromagnetCurrentlyActivated) {
            deactivateElectromagnetToReleasePill();
        }
        
        const int naturalMinimumMicroseconds = 150;
        moveServoToMicroseconds(naturalMinimumMicroseconds);
        delay(systemConfiguration->servoMovementDelayMilliseconds);
    }
    
    /**
     * Move servo to maximum position (for dispensing)
     */
    void moveServoToMaxPosition() {
        int maxSafe = getServoMaxSafe();
        moveServoToMicroseconds(maxSafe);
    }
    
    /**
     * Move servo to maximum position and wait for movement to complete
     */
    void moveServoToMaxPositionAndWait() {
        moveServoToMaxPosition();
        delay(systemConfiguration->servoMovementDelayMilliseconds);
    }
    
    /**
     * Servo full arc sweep: min → max → min (for pill dispensing)
     * This sweeps the full range to ensure pill release
     */
    void servoFullArcSweep() {
        int minSafe = getServoMinSafe();
        int maxSafe = getServoMaxSafe();
        
        // Ensure we start at minimum
        moveServoToMicroseconds(minSafe);
        
        // Sweep to maximum
        moveServoToMicroseconds(maxSafe);
        
        // Wait at maximum
        delay(500);
        
        // Sweep back to minimum
        moveServoToMicroseconds(minSafe);
    }
    
    /**
     * Move servo to dispensing position (sweeps full arc: min → max → min)
     * This ensures pill release by sweeping the full range
     */
    void moveServoToDispensingPosition() {
        servoFullArcSweep();
    }
    
    /**
     * Move servo to dispensing position and wait for movement to complete
     */
    void moveServoToDispensingPositionAndWait() {
        moveServoToDispensingPosition();
        delay(systemConfiguration->servoMovementDelayMilliseconds);
    }
    
    /**
     * Move servo to rest position and wait for movement to complete
     */
    void moveServoToRestPositionAndWait() {
        moveServoToRestPosition();
        delay(systemConfiguration->servoMovementDelayMilliseconds);
    }
    
    /**
     * Get current servo position in microseconds
     * @return Current servo position in microseconds
     */
    int getCurrentServoPosition() {
        if (!dispenserServoMotor.attached()) {
            int minSafe = getServoMinSafe();
            int maxSafe = getServoMaxSafe();
            dispenserServoMotor.attach(PIN_FOR_SERVO_MOTOR_SIGNAL, minSafe, maxSafe);
        }
        int current = dispenserServoMotor.readMicroseconds();
        int minSafe = getServoMinSafe();
        int maxSafe = getServoMaxSafe();
        
        // Clamp to safe range if outside
        if (current < minSafe || current > maxSafe) {
            return minSafe;
        }
        return current;
    }
    
    int moveServoFromCurrentToMax() {
        int maxSafe = getServoMaxSafe();
        int targetPosition = maxSafe;
        
        moveServoToMicroseconds(targetPosition);
        delay(systemConfiguration->servoMovementDelayMilliseconds);
        
        return targetPosition;
    }
    
    int moveServoFromCurrentToMaxAndReturn() {
        int startPosition = getCurrentServoPosition();
        
        moveServoFromCurrentToMax();
        
        moveServoToMicroseconds(startPosition);
        delay(systemConfiguration->servoMovementDelayMilliseconds);
        
        return startPosition;
    }
    
    void moveServoFromCurrentToMaxAndReturnAndWait() {
        moveServoFromCurrentToMaxAndReturn();
        delay(systemConfiguration->servoMovementDelayMilliseconds);
    }
    
    void activateElectromagnetForPillPickup() {
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, HIGH);
        isElectromagnetCurrentlyActivated = true;
    }
    
    void deactivateElectromagnetToReleasePill() {
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, LOW);
        isElectromagnetCurrentlyActivated = false;
    }
    
    void activateElectromagnetAndWaitForStabilization() {
        activateElectromagnetForPillPickup();
        delay(systemConfiguration->electromagnetActivationDelayMilliseconds);
    }
    
    void deactivateElectromagnetWithDelay() {
        deactivateElectromagnetToReleasePill();
        delay(systemConfiguration->electromagnetDeactivationDelayMilliseconds);
    }
    
    bool isElectromagnetActive() {
        return isElectromagnetCurrentlyActivated;
    }
    
    void turnOnReadyStatusLED() {
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, HIGH);
    }
    
    void turnOffReadyStatusLED() {
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, LOW);
    }
    
    void toggleStatusLED() {
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, !digitalRead(PIN_FOR_GREEN_STATUS_LED));
    }
};

// Static member definition (required for C++)
unsigned long HardwareController::stepPulseCount = 0;

#endif // HARDWARE_CONTROLLER_H

