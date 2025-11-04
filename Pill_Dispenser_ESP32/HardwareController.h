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
 * - DC motor control (speed and direction)
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
        // Configure motor control pins
        pinMode(PIN_FOR_MOTOR_SPEED_CONTROL_PWM, OUTPUT);
        pinMode(PIN_FOR_MOTOR_DIRECTION_INPUT_1, OUTPUT);
        pinMode(PIN_FOR_MOTOR_DIRECTION_INPUT_2, OUTPUT);
        
        // Configure electromagnet control pin
        pinMode(PIN_FOR_ELECTROMAGNET_CONTROL, OUTPUT);
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, LOW);
        
        // Configure status LED
        pinMode(PIN_FOR_GREEN_STATUS_LED, OUTPUT);
        digitalWrite(PIN_FOR_GREEN_STATUS_LED, LOW);
        
        // Attach and initialize servo
        dispenserServoMotor.attach(PIN_FOR_SERVO_MOTOR_SIGNAL);
        moveServoToRestPosition();
        
        Serial.println("Hardware actuators initialized");
    }
    
    // ========================================================================
    // Motor Control Methods
    // ========================================================================
    
    /**
     * Set motor to rotate forward at specified speed
     * @param speedPWM PWM value (0-255)
     */
    void setMotorToRotateForwardAtSpeed(int speedPWM) {
        digitalWrite(PIN_FOR_MOTOR_DIRECTION_INPUT_1, HIGH);
        digitalWrite(PIN_FOR_MOTOR_DIRECTION_INPUT_2, LOW);
        analogWrite(PIN_FOR_MOTOR_SPEED_CONTROL_PWM, speedPWM);
    }
    
    /**
     * Set motor to rotate backward at specified speed
     * @param speedPWM PWM value (0-255)
     */
    void setMotorToRotateBackwardAtSpeed(int speedPWM) {
        digitalWrite(PIN_FOR_MOTOR_DIRECTION_INPUT_1, LOW);
        digitalWrite(PIN_FOR_MOTOR_DIRECTION_INPUT_2, HIGH);
        analogWrite(PIN_FOR_MOTOR_SPEED_CONTROL_PWM, speedPWM);
    }
    
    /**
     * Stop the motor immediately
     */
    void stopMotorCompletely() {
        analogWrite(PIN_FOR_MOTOR_SPEED_CONTROL_PWM, 0);
        digitalWrite(PIN_FOR_MOTOR_DIRECTION_INPUT_1, LOW);
        digitalWrite(PIN_FOR_MOTOR_DIRECTION_INPUT_2, LOW);
    }
    
    /**
     * Run motor forward at homing speed (slower for accuracy)
     */
    void runMotorAtHomingSpeed() {
        setMotorToRotateForwardAtSpeed(systemConfiguration->motorHomingSpeedPWM);
    }
    
    /**
     * Run motor forward at custom homing speed (for retry attempts)
     * @param speedPWM Custom PWM speed value (0-255)
     */
    void runMotorAtCustomHomingSpeed(int speedPWM) {
        setMotorToRotateForwardAtSpeed(speedPWM);
    }
    
    /**
     * Run motor forward at normal running speed
     */
    void runMotorAtNormalSpeed() {
        setMotorToRotateForwardAtSpeed(systemConfiguration->motorRunningSpeedPWM);
    }
    
    // ========================================================================
    // Servo Control Methods
    // ========================================================================
    
    /**
     * Move servo to a specific angle
     * @param angleInDegrees Target angle (0-180)
     */
    void moveServoToAngleInDegrees(int angleInDegrees) {
        // Constrain angle to valid range
        angleInDegrees = constrain(angleInDegrees, 0, 180);
        
        dispenserServoMotor.write(angleInDegrees);
        Serial.print("Servo moved to angle: ");
        Serial.print(angleInDegrees);
        Serial.println(" degrees");
    }
    
    /**
     * Move servo to rest/home position
     */
    void moveServoToRestPosition() {
        int restAngle = systemConfiguration->servoRestPositionAngleInDegrees;
        moveServoToAngleInDegrees(restAngle);
    }
    
    /**
     * Move servo to dispensing position
     */
    void moveServoToDispensingPosition() {
        int dispensingAngle = systemConfiguration->servoDispensingAngleInDegrees;
        moveServoToAngleInDegrees(dispensingAngle);
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
    
    // ========================================================================
    // Electromagnet Control Methods
    // ========================================================================
    
    /**
     * Activate the electromagnet to pick up pills
     */
    void activateElectromagnetForPillPickup() {
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, HIGH);
        isElectromagnetCurrentlyActivated = true;
        Serial.println("Electromagnet activated");
    }
    
    /**
     * Deactivate the electromagnet to release pills
     */
    void deactivateElectromagnetToReleasePill() {
        digitalWrite(PIN_FOR_ELECTROMAGNET_CONTROL, LOW);
        isElectromagnetCurrentlyActivated = false;
        Serial.println("Electromagnet deactivated");
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

#endif // HARDWARE_CONTROLLER_H

