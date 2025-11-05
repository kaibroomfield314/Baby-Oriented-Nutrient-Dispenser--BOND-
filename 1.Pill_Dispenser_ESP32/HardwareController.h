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
    
    /**
     * Initialize all hardware actuator pins and attach servo
     */
    void initializeAllHardwareActuators() {
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] INITIALIZING HARDWARE");
        Serial.println("========================================");
        
        // Configure stepper motor control pins
        pinMode(PIN_FOR_STEPPER_DIR, OUTPUT);
        pinMode(PIN_FOR_STEPPER_STEP, OUTPUT);
        pinMode(PIN_FOR_STEPPER_EN, OUTPUT);
        
        Serial.println("[Stepper DEBUG] Stepper pins configured as OUTPUT:");
        Serial.print("[Stepper DEBUG]   DIR pin (D16): ");
        Serial.println(PIN_FOR_STEPPER_DIR);
        Serial.print("[Stepper DEBUG]   STEP pin (D17): ");
        Serial.println(PIN_FOR_STEPPER_STEP);
        Serial.print("[Stepper DEBUG]   EN pin (D18): ");
        Serial.println(PIN_FOR_STEPPER_EN);
        
        // Initialize stepper motor pins (disabled by default)
        digitalWrite(PIN_FOR_STEPPER_DIR, LOW);
        digitalWrite(PIN_FOR_STEPPER_STEP, LOW);
        digitalWrite(PIN_FOR_STEPPER_EN, HIGH);  // HIGH = disabled (typical stepper driver logic)
        
        Serial.println("[Stepper DEBUG] Initial pin states set:");
        Serial.println("[Stepper DEBUG]   DIR: LOW");
        Serial.println("[Stepper DEBUG]   STEP: LOW");
        Serial.println("[Stepper DEBUG]   EN: HIGH (DISABLED)");
        
        // Verify pin states
        int dirState = digitalRead(PIN_FOR_STEPPER_DIR);
        int stepState = digitalRead(PIN_FOR_STEPPER_STEP);
        int enState = digitalRead(PIN_FOR_STEPPER_EN);
        
        Serial.print("[Stepper DEBUG] Verified pin states - DIR:");
        Serial.print(dirState);
        Serial.print(" STEP:");
        Serial.print(stepState);
        Serial.print(" EN:");
        Serial.print(enState);
        Serial.println();
        
        // Reset step counter
        resetStepCounter();
        
        // Configure electromagnet control pin
        pinMode(PIN_FOR_ELECTROMAGNET_CONTROL, OUTPUT);
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, LOW);
        
        // Configure status LED
        pinMode(PIN_FOR_GREEN_STATUS_LED, OUTPUT);
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, LOW);
        
        // Attach and initialize servo with min/max microseconds (from test code)
        int minSafe = getServoMinSafe();
        int maxSafe = getServoMaxSafe();
        dispenserServoMotor.attach(PIN_FOR_SERVO_MOTOR_SIGNAL, minSafe, maxSafe);
        moveServoToRestPosition();
        
        Serial.println("[Stepper DEBUG] Hardware initialization complete");
        Serial.println("========================================");
        Serial.println("Hardware actuators initialized");
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
        
        int validatedWidth = constrain(pulseWidthMicroseconds, minWidth, maxWidth);
        
        if (validatedWidth != pulseWidthMicroseconds) {
            Serial.print("[Stepper DEBUG] Step pulse width ");
            Serial.print(pulseWidthMicroseconds);
            Serial.print(" μs adjusted to ");
            Serial.print(validatedWidth);
            Serial.print(" μs (limits: ");
            Serial.print(minWidth);
            Serial.print("-");
            Serial.print(maxWidth);
            Serial.println(" μs)");
        }
        
        return validatedWidth;
    }
    
    /**
     * Validate and constrain step delay to safe limits (legacy function, kept for compatibility)
     * With symmetric pulse timing, this validates the pulse width instead
     * @param stepDelayMicroseconds Requested delay in microseconds
     * @return Validated delay within safe limits (for compatibility, but not used directly)
     */
    int validateStepDelay(int stepDelayMicroseconds) {
        // With symmetric pulse timing, validate the pulse width instead
        // Delay is 2 × pulse width, so we validate pulse width = delay / 2
        int pulseWidth = stepDelayMicroseconds / 2;
        int validatedWidth = validateStepPulseWidth(pulseWidth);
        return validatedWidth * 2;  // Return as delay for compatibility
    }
    
    /**
     * Calculate number of steps needed for a given angle in degrees
     * @param angleInDegrees Target angle (can be > 360 for multiple rotations)
     * @return Number of steps needed
     */
    long calculateStepsForAngle(float angleInDegrees) {
        // Calculate total steps per revolution with microstepping and gear ratio
        int baseSteps = systemConfiguration->stepperStepsPerRevolution;
        int microstepping = systemConfiguration->stepperMicrostepping;
        float gearRatio = systemConfiguration->stepperGearRatio;
        float totalStepsPerRevolution = baseSteps * microstepping * gearRatio;
        
        // Calculate steps for given angle
        long steps = (long)((angleInDegrees / 360.0) * totalStepsPerRevolution);
        
        Serial.print("[Stepper DEBUG] Angle ");
        Serial.print(angleInDegrees);
        Serial.print("° → ");
        Serial.print(steps);
        Serial.print(" steps (baseSteps=");
        Serial.print(baseSteps);
        Serial.print(", microstepping=");
        Serial.print(microstepping);
        Serial.print(", gearRatio=");
        Serial.print(gearRatio);
        Serial.print(", totalSteps/rev=");
        Serial.print(totalStepsPerRevolution);
        Serial.println(")");
        
        return steps;
    }
    
    // Step counter for debugging (reset before operations)
    static unsigned long stepPulseCount;
    
    /**
     * Generate a single step pulse
     * STEP pin: HIGH→LOW transition triggers step on most stepper drivers
     * Uses symmetric pulse timing (like test code): HIGH for step_us, LOW for step_us
     * This provides slower, smoother movement compared to fast pulses with delays
     */
    void generateStepPulse() {
        stepPulseCount++;
        
        // Log every 100th step to avoid flooding serial output
        if (stepPulseCount % 100 == 0) {
            Serial.print("[Stepper DEBUG] Step pulse #");
            Serial.print(stepPulseCount);
            Serial.print(" - STEP pin: HIGH");
            Serial.flush();
        }
        
        // Symmetric pulse timing (integrated from test code)
        // HIGH for step_us microseconds, then LOW for step_us microseconds
        int stepPulseWidth = systemConfiguration->stepperStepPulseWidthMicroseconds;
        
        digitalWrite(PIN_FOR_STEPPER_STEP, HIGH);
        delayMicroseconds(stepPulseWidth);
        digitalWrite(PIN_FOR_STEPPER_STEP, LOW);
        
        if (stepPulseCount % 100 == 0) {
            Serial.print(" → LOW (pulse width: ");
            Serial.print(stepPulseWidth);
            Serial.println(" μs)");
        }
        
        delayMicroseconds(stepPulseWidth);  // Symmetric LOW time
    }
    
    /**
     * Reset step pulse counter (call before starting a new movement)
     */
    void resetStepCounter() {
        stepPulseCount = 0;
        Serial.println("[Stepper DEBUG] Step counter reset");
    }
    
    /**
     * Enable stepper motor (prepare for movement)
     * @param directionForward true for forward, false for backward
     */
    void enableStepperMotor(bool directionForward) {
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] ENABLING MOTOR");
        Serial.println("========================================");
        
        // Set direction first (before enabling)
        int dirPinState = directionForward ? HIGH : LOW;
        digitalWrite(PIN_FOR_STEPPER_DIR, dirPinState);
        delayMicroseconds(5);  // Small delay for direction to settle
        
        Serial.print("[Stepper DEBUG] DIR pin (D16) set to: ");
        Serial.print(dirPinState == HIGH ? "HIGH" : "LOW");
        Serial.print(" (");
        Serial.print(directionForward ? "FORWARD/clockwise" : "BACKWARD/anticlockwise");
        Serial.println(")");
        
        // Enable stepper motor (LOW = enabled for most drivers)
        // If your driver uses HIGH = enabled, invert this logic
        digitalWrite(PIN_FOR_STEPPER_EN, LOW);
        
        // Small delay to ensure signals are stable
        delayMicroseconds(10);
        
        Serial.print("[Stepper DEBUG] EN pin (D18) set to: LOW (ENABLED)");
        Serial.println();
        Serial.print("[Stepper DEBUG] STEP pin (D17) ready for pulses");
        Serial.println();
        
        // Read actual pin states for verification
        int actualDir = digitalRead(PIN_FOR_STEPPER_DIR);
        int actualEn = digitalRead(PIN_FOR_STEPPER_EN);
        int actualStep = digitalRead(PIN_FOR_STEPPER_STEP);
        
        Serial.print("[Stepper DEBUG] Pin states verified - DIR:");
        Serial.print(actualDir);
        Serial.print(" EN:");
        Serial.print(actualEn);
        Serial.print(" STEP:");
        Serial.print(actualStep);
        Serial.println();
        
        Serial.println("[Stepper DEBUG] Motor enabled and ready for stepping");
        Serial.println("========================================");
    }
    
    /**
     * Generate a step pulse with timing (for continuous rotation loops)
     * With symmetric pulse timing, generateStepPulse() already handles the timing,
     * so this function just calls it (no additional delay needed)
     * @param stepDelay Not used with symmetric pulse timing (kept for compatibility)
     */
    void stepWithDelay(int stepDelay) {
        // With symmetric pulse timing, the delay is built into generateStepPulse()
        // Each step takes: stepPulseWidth HIGH + stepPulseWidth LOW = 2 × stepPulseWidth
        generateStepPulse();
        
        // Log every 1000 steps for continuous rotation
        if (stepPulseCount % 1000 == 0 && stepPulseCount > 0) {
            Serial.print("[Stepper DEBUG] Continuous rotation: ");
            Serial.print(stepPulseCount);
            Serial.print(" steps completed, pulse width=");
            Serial.print(systemConfiguration->stepperStepPulseWidthMicroseconds);
            Serial.println(" μs");
        }
    }
    
    /**
     * Rotate stepper motor forward by a specific angle
     * @param angleInDegrees Angle to rotate (can be > 360 for multiple rotations)
     * @param stepDelayMicroseconds Delay between step pulses in microseconds (lower = faster)
     */
    void rotateStepperForwardByAngle(float angleInDegrees, int stepDelayMicroseconds) {
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] STARTING FORWARD ROTATION");
        Serial.println("========================================");
        
        resetStepCounter();
        
        // Enable stepper motor
        enableStepperMotor(true);  // true = forward
        
        // Calculate steps (symmetric pulse timing built into generateStepPulse)
        long steps = calculateStepsForAngle(angleInDegrees);
        
        Serial.print("[Stepper DEBUG] Movement parameters: ");
        Serial.print(angleInDegrees);
        Serial.print("°, ");
        Serial.print(steps);
        Serial.print(" steps, pulse width ");
        Serial.print(systemConfiguration->stepperStepPulseWidthMicroseconds);
        Serial.println(" μs");
        
        Serial.println("[Stepper DEBUG] Starting step pulse generation...");
        
        // Generate step pulses (symmetric timing built into generateStepPulse)
        unsigned long startTime = millis();
        for (long i = 0; i < steps; i++) {
            generateStepPulse();  // Includes symmetric HIGH/LOW timing
            
            // Log progress every 10% or every 100 steps
            if ((i + 1) % 100 == 0 || (i + 1) == steps || (steps > 10 && (i + 1) % (steps / 10) == 0)) {
                float progress = ((float)(i + 1) / steps) * 100.0;
                Serial.print("[Stepper DEBUG] Progress: ");
                Serial.print(progress);
                Serial.print("% (");
                Serial.print(i + 1);
                Serial.print("/");
                Serial.print(steps);
                Serial.println(" steps)");
            }
        }
        
        unsigned long elapsed = millis() - startTime;
        Serial.print("[Stepper DEBUG] Completed ");
        Serial.print(steps);
        Serial.print(" steps in ");
        Serial.print(elapsed);
        Serial.println(" ms");
        
        // Small settling delay
        delayMicroseconds(100);
        
        Serial.println("[Stepper DEBUG] Forward rotation complete");
        Serial.println("========================================");
    }
    
    /**
     * Move stepper motor forward by exact number of steps
     * @param steps Number of steps to move (positive = forward)
     * @param stepDelayMicroseconds Delay between step pulses in microseconds
     * @return Number of steps actually moved
     */
    long moveStepperForwardBySteps(long steps, int stepDelayMicroseconds) {
        if (steps <= 0) {
            Serial.println("[Stepper DEBUG] No forward movement needed (steps <= 0)");
            return 0;
        }
        
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] STARTING FORWARD MOVEMENT BY STEPS");
        Serial.println("========================================");
        
        resetStepCounter();
        
        // Enable stepper motor
        enableStepperMotor(true);  // true = forward
        
        Serial.print("[Stepper DEBUG] Movement: ");
        Serial.print(steps);
        Serial.print(" steps forward, pulse width ");
        Serial.print(systemConfiguration->stepperStepPulseWidthMicroseconds);
        Serial.println(" μs");
        
        Serial.println("[Stepper DEBUG] Starting step pulse generation...");
        
        // Generate step pulses (symmetric timing built into generateStepPulse)
        unsigned long startTime = millis();
        for (long i = 0; i < steps; i++) {
            generateStepPulse();  // Includes symmetric HIGH/LOW timing
            
            // Log progress every 100 steps
            if ((i + 1) % 100 == 0 || (i + 1) == steps) {
                float progress = ((float)(i + 1) / steps) * 100.0;
                Serial.print("[Stepper DEBUG] Progress: ");
                Serial.print(progress);
                Serial.print("% (");
                Serial.print(i + 1);
                Serial.print("/");
                Serial.print(steps);
                Serial.println(" steps)");
            }
        }
        
        unsigned long elapsed = millis() - startTime;
        Serial.print("[Stepper DEBUG] Completed ");
        Serial.print(steps);
        Serial.print(" steps forward in ");
        Serial.print(elapsed);
        Serial.println(" ms");
        
        delayMicroseconds(100);
        
        Serial.println("[Stepper DEBUG] Forward movement complete");
        Serial.println("========================================");
        
        return steps;  // Return steps moved (positive for forward)
    }
    
    /**
     * Move stepper motor backward by exact number of steps
     * @param steps Number of steps to move (positive value, will move backward)
     * @param stepDelayMicroseconds Delay between step pulses in microseconds
     * @return Number of steps actually moved (negative for backward)
     */
    long moveStepperBackwardBySteps(long steps, int stepDelayMicroseconds) {
        if (steps <= 0) {
            Serial.println("[Stepper DEBUG] No backward movement needed (steps <= 0)");
            return 0;
        }
        
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] STARTING BACKWARD MOVEMENT BY STEPS");
        Serial.println("========================================");
        
        resetStepCounter();
        
        // Enable stepper motor
        enableStepperMotor(false);  // false = backward
        
        Serial.print("[Stepper DEBUG] Movement: ");
        Serial.print(steps);
        Serial.print(" steps backward, pulse width ");
        Serial.print(systemConfiguration->stepperStepPulseWidthMicroseconds);
        Serial.println(" μs");
        
        Serial.println("[Stepper DEBUG] Starting step pulse generation...");
        
        // Generate step pulses (symmetric timing built into generateStepPulse)
        unsigned long startTime = millis();
        for (long i = 0; i < steps; i++) {
            generateStepPulse();  // Includes symmetric HIGH/LOW timing
            
            // Log progress every 100 steps
            if ((i + 1) % 100 == 0 || (i + 1) == steps) {
                float progress = ((float)(i + 1) / steps) * 100.0;
                Serial.print("[Stepper DEBUG] Progress: ");
                Serial.print(progress);
                Serial.print("% (");
                Serial.print(i + 1);
                Serial.print("/");
                Serial.print(steps);
                Serial.println(" steps)");
            }
        }
        
        unsigned long elapsed = millis() - startTime;
        Serial.print("[Stepper DEBUG] Completed ");
        Serial.print(steps);
        Serial.print(" steps backward in ");
        Serial.print(elapsed);
        Serial.println(" ms");
        
        delayMicroseconds(100);
        
        Serial.println("[Stepper DEBUG] Backward movement complete");
        Serial.println("========================================");
        
        return -steps;  // Return negative for backward movement
    }
    
    /**
     * Rotate stepper motor backward by a specific angle
     * @param angleInDegrees Angle to rotate (can be > 360 for multiple rotations)
     * @param stepDelayMicroseconds Delay between step pulses in microseconds (lower = faster)
     */
    void rotateStepperBackwardByAngle(float angleInDegrees, int stepDelayMicroseconds) {
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] STARTING BACKWARD ROTATION");
        Serial.println("========================================");
        
        resetStepCounter();
        
        // Enable stepper motor
        enableStepperMotor(false);  // false = backward
        
        // Calculate steps (symmetric pulse timing built into generateStepPulse)
        long steps = calculateStepsForAngle(angleInDegrees);
        
        Serial.print("[Stepper DEBUG] Movement parameters: ");
        Serial.print(angleInDegrees);
        Serial.print("°, ");
        Serial.print(steps);
        Serial.print(" steps, pulse width ");
        Serial.print(systemConfiguration->stepperStepPulseWidthMicroseconds);
        Serial.println(" μs");
        
        Serial.println("[Stepper DEBUG] Starting step pulse generation...");
        
        // Generate step pulses (symmetric timing built into generateStepPulse)
        unsigned long startTime = millis();
        for (long i = 0; i < steps; i++) {
            generateStepPulse();  // Includes symmetric HIGH/LOW timing
            
            // Log progress every 10% or every 100 steps
            if ((i + 1) % 100 == 0 || (i + 1) == steps || (steps > 10 && (i + 1) % (steps / 10) == 0)) {
                float progress = ((float)(i + 1) / steps) * 100.0;
                Serial.print("[Stepper DEBUG] Progress: ");
                Serial.print(progress);
                Serial.print("% (");
                Serial.print(i + 1);
                Serial.print("/");
                Serial.print(steps);
                Serial.println(" steps)");
            }
        }
        
        unsigned long elapsed = millis() - startTime;
        Serial.print("[Stepper DEBUG] Completed ");
        Serial.print(steps);
        Serial.print(" steps in ");
        Serial.print(elapsed);
        Serial.println(" ms");
        
        // Small settling delay
        delayMicroseconds(100);
        
        Serial.println("[Stepper DEBUG] Backward rotation complete");
        Serial.println("========================================");
    }
    
    /**
     * Rotate stepper motor forward continuously (for homing - until stopped externally)
     * Note: Call enableStepperMotor(true) once before the loop, then call this repeatedly
     * @param stepDelayMicroseconds Delay between step pulses in microseconds (lower = faster)
     */
    void rotateStepperForwardContinuous(int stepDelayMicroseconds) {
        // Validate delay (only log first time to avoid flooding)
        static int lastLoggedDelay = -1;
        int validatedDelay = validateStepDelay(stepDelayMicroseconds);
        
        if (stepDelayMicroseconds != lastLoggedDelay) {
            Serial.print("[Stepper DEBUG] Continuous FORWARD rotation - delay: ");
            Serial.print(validatedDelay);
            Serial.println(" μs");
            lastLoggedDelay = stepDelayMicroseconds;
        }
        
        // Generate step pulse with proper timing
        stepWithDelay(validatedDelay);
    }
    
    /**
     * Rotate stepper motor backward continuously (for homing - until stopped externally)
     * Note: Call enableStepperMotor(false) once before the loop, then call this repeatedly
     * @param stepDelayMicroseconds Delay between step pulses in microseconds (lower = faster)
     */
    void rotateStepperBackwardContinuous(int stepDelayMicroseconds) {
        // Validate delay (only log first time to avoid flooding)
        static int lastLoggedDelay = -1;
        int validatedDelay = validateStepDelay(stepDelayMicroseconds);
        
        if (stepDelayMicroseconds != lastLoggedDelay) {
            Serial.print("[Stepper DEBUG] Continuous BACKWARD rotation - delay: ");
            Serial.print(validatedDelay);
            Serial.println(" μs");
            lastLoggedDelay = stepDelayMicroseconds;
        }
        
        // Generate step pulse with proper timing
        stepWithDelay(validatedDelay);
    }
    
    /**
     * Stop the stepper motor immediately (disable motor)
     */
    void stopMotorCompletely() {
        Serial.println("========================================");
        Serial.println("[Stepper DEBUG] STOPPING MOTOR");
        Serial.println("========================================");
        
        Serial.print("[Stepper DEBUG] Total steps generated before stop: ");
        Serial.println(stepPulseCount);
        
        // Disable stepper motor (HIGH = disabled)
        digitalWrite(PIN_FOR_STEPPER_EN, HIGH);
        
        // Ensure STEP pin is LOW
        digitalWrite(PIN_FOR_STEPPER_STEP, LOW);
        
        // Read pin states for verification
        int actualEn = digitalRead(PIN_FOR_STEPPER_EN);
        int actualStep = digitalRead(PIN_FOR_STEPPER_STEP);
        int actualDir = digitalRead(PIN_FOR_STEPPER_DIR);
        
        Serial.print("[Stepper DEBUG] Pin states after stop - EN:");
        Serial.print(actualEn);
        Serial.print(" (should be HIGH=disabled) STEP:");
        Serial.print(actualStep);
        Serial.print(" (should be LOW) DIR:");
        Serial.print(actualDir);
        Serial.println();
        
        Serial.println("[Stepper DEBUG] Motor disabled and stopped");
        Serial.println("========================================");
    }
    
    // ========================================================================
    // Legacy Motor Control Methods (mapped to stepper for compatibility)
    // NOTE: These methods are kept for backward compatibility but now use direct timing
    // ========================================================================
    
    /**
     * Set motor to rotate forward at specified step delay (for compatibility)
     * Note: This enables the motor and sets direction - use once before loop
     * @param stepDelayMicroseconds Delay between step pulses in microseconds
     */
    void setMotorToRotateForwardAtSpeed(int stepDelayMicroseconds) {
        enableStepperMotor(true);  // Enable and set forward direction
        rotateStepperForwardContinuous(stepDelayMicroseconds);
    }
    
    /**
     * Set motor to rotate backward at specified step delay (for compatibility)
     * Note: This enables the motor and sets direction - use once before loop
     * @param stepDelayMicroseconds Delay between step pulses in microseconds
     */
    void setMotorToRotateBackwardAtSpeed(int stepDelayMicroseconds) {
        enableStepperMotor(false);  // Enable and set backward direction
        rotateStepperBackwardContinuous(stepDelayMicroseconds);
    }
    
    /**
     * Run motor forward at homing speed (slower for accuracy)
     * Uses stepperHomingStepDelayMicroseconds from configuration
     */
    void runMotorAtHomingSpeed() {
        enableStepperMotor(true);
        rotateStepperForwardContinuous(systemConfiguration->stepperHomingStepDelayMicroseconds);
    }
    
    /**
     * Run motor forward at custom homing delay (for retry attempts)
     * @param stepDelayMicroseconds Custom step delay in microseconds
     */
    void runMotorAtCustomHomingSpeed(int stepDelayMicroseconds) {
        enableStepperMotor(true);
        rotateStepperForwardContinuous(stepDelayMicroseconds);
    }
    
    /**
     * Run motor forward at normal running speed
     * Uses stepperRunningStepDelayMicroseconds from configuration
     * Note: For precise positioning, use rotateStepperForwardByAngle() instead
     */
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
    
    /**
     * Move servo to a specific position in microseconds (from test code)
     * @param targetMicroseconds Target position in microseconds
     */
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
    
    /**
     * Perform servo homing sequence - move to natural minimum position (150 microseconds)
     * This is the servo's natural minimum position found during testing
     * The electromagnet is NOT activated during this sequence
     */
    void performServoHomingSequence() {
        Serial.println("========================================");
        Serial.println("[Servo Homing] Starting servo homing sequence");
        Serial.println("========================================");
        
        // Ensure electromagnet is OFF (should be off by default, but ensure it)
        if (isElectromagnetCurrentlyActivated) {
            Serial.println("[Servo Homing] WARNING: Electromagnet was on, turning off...");
            deactivateElectromagnetToReleasePill();
        }
        
        // Move servo to natural minimum position (150 microseconds)
        // This is the servo's natural minimum position found during testing
        const int naturalMinimumMicroseconds = 150;
        
        Serial.print("[Servo Homing] Moving servo to natural minimum position: ");
        Serial.print(naturalMinimumMicroseconds);
        Serial.println(" μs");
        
        moveServoToMicroseconds(naturalMinimumMicroseconds);
        delay(systemConfiguration->servoMovementDelayMilliseconds);
        
        // Verify position
        int currentPosition = getCurrentServoPosition();
        Serial.print("[Servo Homing] Servo homing complete. Current position: ");
        Serial.print(currentPosition);
        Serial.println(" μs");
        
        Serial.println("========================================");
        Serial.println("[Servo Homing] Servo homing sequence complete");
        Serial.println("========================================");
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
    
    /**
     * Move servo from current position to maximum position (full 180 degrees)
     * Returns the target position reached
     * This is used for pill dispensing - call moveServoToMicroseconds() separately to return
     * @return The target position reached in microseconds
     */
    int moveServoFromCurrentToMax() {
        int minSafe = getServoMinSafe();
        int maxSafe = getServoMaxSafe();
        
        // Get current position
        int startPosition = getCurrentServoPosition();
        
        // Move to maximum position (full 180 degrees from minimum)
        // This ensures maximum travel range for pill dispensing
        int targetPosition = maxSafe;
        
        Serial.print("[Servo] Moving from current position (");
        Serial.print(startPosition);
        Serial.print(" μs) to maximum position (");
        Serial.print(targetPosition);
        Serial.print(" μs) - Range: ");
        Serial.print(targetPosition - startPosition);
        Serial.println(" μs");
        
        // Move to target position
        moveServoToMicroseconds(targetPosition);
        delay(systemConfiguration->servoMovementDelayMilliseconds);
        
        return targetPosition;
    }
    
    /**
     * Move servo from current position to maximum position (full 180 degrees),
     * then return to original position
     * This is used for pill dispensing
     * @return The original start position (for returning later)
     */
    int moveServoFromCurrentToMaxAndReturn() {
        int startPosition = getCurrentServoPosition();
        
        // Move to max position
        moveServoFromCurrentToMax();
        
        // Return to original position
        Serial.print("[Servo] Returning to original position (");
        Serial.print(startPosition);
        Serial.println(" μs)");
        moveServoToMicroseconds(startPosition);
        delay(systemConfiguration->servoMovementDelayMilliseconds);
        
        return startPosition;
    }
    
    /**
     * Move servo from current position to maximum position (or 180 degrees worth),
     * then return to original position, and wait for movement to complete
     */
    void moveServoFromCurrentToMaxAndReturnAndWait() {
        moveServoFromCurrentToMaxAndReturn();
        delay(systemConfiguration->servoMovementDelayMilliseconds);
    }
    
    // ========================================================================
    // Electromagnet Control Methods
    // ========================================================================
    
    /**
     * Activate the electromagnet to pick up pills
     */
    void activateElectromagnetForPillPickup() {
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, HIGH);
        isElectromagnetCurrentlyActivated = true;
    }
    
    /**
     * Deactivate the electromagnet to release pills
     */
    void deactivateElectromagnetToReleasePill() {
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, LOW);
        isElectromagnetCurrentlyActivated = false;
    }
    
    /**
     * Activate electromagnet and wait for stabilization
     */
    void activateElectromagnetAndWaitForStabilization() {
        activateElectromagnetForPillPickup();
        delay(systemConfiguration->electromagnetActivationDelayMilliseconds);
    }
    
    /**
     * Deactivate electromagnet with delay
     */
    void deactivateElectromagnetWithDelay() {
        deactivateElectromagnetToReleasePill();
        delay(systemConfiguration->electromagnetDeactivationDelayMilliseconds);
    }
    
    /**
     * Check if electromagnet is currently activated
     * @return true if electromagnet is on
     */
    bool isElectromagnetActive() {
        return isElectromagnetCurrentlyActivated;
    }
    
    // ========================================================================
    // Status LED Control Methods
    // ========================================================================
    
    /**
     * Turn on the green status LED to indicate ready state
     */
    void turnOnReadyStatusLED() {
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, HIGH);
    }
    
    /**
     * Turn off the green status LED
     */
    void turnOffReadyStatusLED() {
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, LOW);
    }
    
    /**
     * Toggle the status LED state
     */
    void toggleStatusLED() {
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, !digitalRead(PIN_FOR_GREEN_STATUS_LED));
    }
};

// Static member definition (required for C++)
unsigned long HardwareController::stepPulseCount = 0;

#endif // HARDWARE_CONTROLLER_H

