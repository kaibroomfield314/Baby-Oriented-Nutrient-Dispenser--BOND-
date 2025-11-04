#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// BLE (Bluetooth Low Energy) Configuration
// ============================================================================
#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_DEVICE_NAME         "PillDispenser"

// ============================================================================
// Motor Control Pin Definitions
// ============================================================================
#define PIN_FOR_MOTOR_SPEED_CONTROL_PWM     16    // ENA - PWM for motor speed
#define PIN_FOR_MOTOR_DIRECTION_INPUT_1     17    // IN1 - Motor direction control 1
#define PIN_FOR_MOTOR_DIRECTION_INPUT_2     18    // IN2 - Motor direction control 2

// ============================================================================
// Encoder Pin Definitions (for position tracking)
// ============================================================================
#define PIN_FOR_ENCODER_CHANNEL_1           39    // VN - Encoder channel 1
#define PIN_FOR_ENCODER_CHANNEL_2           34    // D34 - Encoder channel 2

// ============================================================================
// Sensor Pin Definitions
// ============================================================================
#define PIN_FOR_HOME_POSITION_SWITCH        12    // D12 - Home position limit switch
#define PIN_FOR_INFRARED_PILL_DETECTOR      35    // D35 - IR sensor for pill detection

// ============================================================================
// Actuator Pin Definitions
// ============================================================================
#define PIN_FOR_ELECTROMAGNET_CONTROL       15    // D15 - Electromagnet relay/transistor control
#define PIN_FOR_SERVO_MOTOR_SIGNAL          4     // D4 - Servo motor PWM signal

// ============================================================================
// User Interface Button Pin Definitions
// ============================================================================
#define PIN_FOR_COMPARTMENT_BUTTON_1        13    // D13 - Direct select compartment 1
#define PIN_FOR_COMPARTMENT_BUTTON_2        14    // D14 - Direct select compartment 2
#define PIN_FOR_COMPARTMENT_BUTTON_3        27    // D27 - Direct select compartment 3
#define PIN_FOR_COMPARTMENT_BUTTON_4        36    // VP - Direct select compartment 4
#define PIN_FOR_COMPARTMENT_BUTTON_5        33    // D33 - Direct select compartment 5
#define PIN_FOR_NAVIGATION_BACK_BUTTON      25    // D25 - Navigate back/decrement
#define PIN_FOR_NAVIGATION_SELECT_BUTTON    26    // D26 - Confirm selection/dispense

// ============================================================================
// LCD Display Pin Definitions (16x2 Character LCD)
// ============================================================================
#define PIN_FOR_LCD_REGISTER_SELECT         19    // D19 - RS (Register Select)
#define PIN_FOR_LCD_ENABLE_SIGNAL           5     // D5 - E (Enable)
#define PIN_FOR_LCD_DATA_BIT_4              21    // D21 - D4
#define PIN_FOR_LCD_DATA_BIT_5              22    // D22 - D5
#define PIN_FOR_LCD_DATA_BIT_6              23    // D23 - D6
#define PIN_FOR_LCD_DATA_BIT_7              32    // D32 - D7

// ============================================================================
// Status Indicator Pin Definitions
// ============================================================================
#define PIN_FOR_GREEN_STATUS_LED            2     // D2 - Green LED for ready status

// ============================================================================
// Hardware Constants
// ============================================================================
#define LCD_NUMBER_OF_COLUMNS               16
#define LCD_NUMBER_OF_ROWS                  2

// ============================================================================
// System Timeout Constants (milliseconds)
// ============================================================================
#define MAXIMUM_HOMING_TIMEOUT_MILLISECONDS 10000

#endif // CONFIG_H

