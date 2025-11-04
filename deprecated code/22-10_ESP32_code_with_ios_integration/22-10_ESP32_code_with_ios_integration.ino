#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>
#include <LiquidCrystal.h>

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Pin Definitions
#define MOTOR_ENA 16    // PWM for motor speed
#define MOTOR_IN1 17    // Motor direction 1
#define MOTOR_IN2 18    // Motor direction 2
#define ENCODER_C1 36   // VN - Encoder channel 1
#define ENCODER_C2 34   // D34 - Encoder channel 2
#define HOME_SWITCH 12  // D12 - Home position switch
#define IR_SENSOR 35    // D35 - Pill detection sensor
#define MAGNET_PIN 15   // D15 - Electromagnet control
#define SERVO_PIN 4     // D4 - Servo motor signal

// Button Pins
#define BTN_1 13        // D13
#define BTN_2 14        // D14
#define BTN_3 27        // D27
#define BTN_4 36        // VP (shared with encoder, be careful)
#define BTN_5 33        // D33
#define BTN_BACK 25     // D25
#define BTN_SELECT 26   // D26

// LCD Pins (16x2)
#define LCD_RS 19       // D19
#define LCD_E 5         // D5
#define LCD_D4 21       // D21
#define LCD_D5 22       // D22
#define LCD_D6 23       // D23
#define LCD_D7 32       // D32

// RGB LED (if needed for status)
#define RGB_GREEN 2     // D2

// System Constants
#define NUM_COMPARTMENTS 5
#define ANGLE_PER_COMPARTMENT (360.0 / NUM_COMPARTMENTS)
#define MOTOR_SPEED 200
#define MAX_DISPENSE_ATTEMPTS 3
#define PILL_DETECT_TIMEOUT 2000
#define DEBOUNCE_DELAY 50

// Global Variables
Servo dispenserServo;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;

volatile long encoderPosition = 0;
volatile int lastEncoderA = 0;
int currentCompartment = 0;
bool isHomed = false;

unsigned long lastButtonPress = 0;
int selectedCompartment = 1;
int dispensedCount[NUM_COMPARTMENTS] = {0};

// Function Prototypes
void homeMotor();
void moveToCompartment(int compartment);
bool dispensePill();
void activateMagnet(bool state);
void moveServo(int angle);
bool detectPill();
void updateLCD();
void handleButtons();
void IRAM_ATTR encoderISR();
void IRAM_ATTR homeSwitchISR();

// BLE Callbacks
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("BLE Device Connected");
        lcd.setCursor(0, 1);
        lcd.print("BLE: Connected  ");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("BLE Device Disconnected");
        lcd.setCursor(0, 1);
        lcd.print("BLE: Waiting... ");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        String value = pCharacteristic->getValue();
        
        if (value.length() > 0) {
            String command = String(value.c_str());
            Serial.println("Received: " + command);
            
            if (command.startsWith("DISPENSE:")) {
                // Parse DISPENSE:COMPARTMENT:COUNT or DISPENSE:COMPARTMENT
                int firstColon = command.indexOf(':');
                int secondColon = command.indexOf(':', firstColon + 1);
                
                int compartment = command.substring(firstColon + 1, 
                    secondColon > 0 ? secondColon : command.length()).toInt();
                int count = 1;
                
                if (secondColon > 0) {
                    count = command.substring(secondColon + 1).toInt();
                }
                
                if (compartment >= 1 && compartment <= NUM_COMPARTMENTS) {
                    lcd.clear();
                    lcd.print("Dispensing...");
                    lcd.setCursor(0, 1);
                    lcd.print("Slot ");
                    lcd.print(compartment);
                    
                    moveToCompartment(compartment);
                    
                    int successCount = 0;
                    for (int i = 0; i < count; i++) {
                        if (dispensePill()) {
                            successCount++;
                            dispensedCount[compartment - 1]++;
                        }
                        delay(500);
                    }
                    
                    String response = "Dispensed: " + String(successCount) + "/" + String(count);
                    pCharacteristic->setValue(response.c_str());
                    pCharacteristic->notify();
                    
                    updateLCD();
                }
            }
            else if (command == "STATUS") {
                String status = "Status:";
                for (int i = 0; i < NUM_COMPARTMENTS; i++) {
                    status += " C" + String(i + 1) + ":" + String(dispensedCount[i]);
                }
                pCharacteristic->setValue(status.c_str());
                pCharacteristic->notify();
            }
            else if (command == "RESET") {
                for (int i = 0; i < NUM_COMPARTMENTS; i++) {
                    dispensedCount[i] = 0;
                }
                pCharacteristic->setValue("Statistics Reset");
                pCharacteristic->notify();
                updateLCD();
            }
            else if (command == "HOME") {
                homeMotor();
                pCharacteristic->setValue("Homing Complete");
                pCharacteristic->notify();
            }
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Pill Dispenser Initializing...");
    
    // Initialize LCD
    lcd.begin(16, 2);
    lcd.print("Pill Dispenser");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    // Configure Motor Pins
    pinMode(MOTOR_ENA, OUTPUT);
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    
    // Configure Sensor Pins
    pinMode(HOME_SWITCH, INPUT_PULLUP);
    pinMode(IR_SENSOR, INPUT);
    pinMode(ENCODER_C1, INPUT);
    pinMode(ENCODER_C2, INPUT);
    
    // Configure Control Pins
    pinMode(MAGNET_PIN, OUTPUT);
    digitalWrite(MAGNET_PIN, LOW);
    
    pinMode(RGB_GREEN, OUTPUT);
    digitalWrite(RGB_GREEN, LOW);
    
    // Configure Button Pins
    pinMode(BTN_1, INPUT_PULLUP);
    pinMode(BTN_2, INPUT_PULLUP);
    pinMode(BTN_3, INPUT_PULLUP);
    pinMode(BTN_5, INPUT_PULLUP);
    pinMode(BTN_BACK, INPUT_PULLUP);
    pinMode(BTN_SELECT, INPUT_PULLUP);
    
    // Attach Servo
    dispenserServo.attach(SERVO_PIN);
    dispenserServo.write(0);
    
    // Attach Interrupts
    attachInterrupt(digitalPinToInterrupt(ENCODER_C1), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_C2), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(HOME_SWITCH), homeSwitchISR, FALLING);
    
    // Initialize BLE
    BLEDevice::init("PillDispenser");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    Serial.println("BLE Service Started. Waiting for connections...");
    
    // Home the motor
    delay(1000);
    homeMotor();
    
    // Ready
    digitalWrite(RGB_GREEN, HIGH);
    updateLCD();
}

void loop() {
    // Handle BLE connection status
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);
        pServer->startAdvertising();
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    // Handle button presses
    handleButtons();
    
    delay(10);
}

void homeMotor() {
    Serial.println("Homing motor...");
    lcd.clear();
    lcd.print("Homing...");
    
    // Rotate until home switch is activated
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
    analogWrite(MOTOR_ENA, MOTOR_SPEED / 2);
    
    unsigned long startTime = millis();
    while (digitalRead(HOME_SWITCH) == HIGH) {
        if (millis() - startTime > 10000) {
            Serial.println("Homing timeout!");
            break;
        }
        delay(10);
    }
    
    // Stop motor
    analogWrite(MOTOR_ENA, 0);
    delay(100);
    
    // Reset encoder position
    encoderPosition = 0;
    currentCompartment = 0;
    isHomed = true;
    
    Serial.println("Homing complete");
    lcd.setCursor(0, 1);
    lcd.print("Home: OK");
    delay(1000);
}

void moveToCompartment(int compartment) {
    if (!isHomed) {
        homeMotor();
    }
    
    if (compartment < 1 || compartment > NUM_COMPARTMENTS) {
        Serial.println("Invalid compartment");
        return;
    }
    
    Serial.print("Moving to compartment ");
    Serial.println(compartment);
    
    float targetAngle = (compartment - 1) * ANGLE_PER_COMPARTMENT * 50;
    
    // Simple position control (you may need to tune this based on your encoder)
    // This is a basic implementation - adjust based on your encoder resolution
    
    int direction = 1; // Assume forward
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
    analogWrite(MOTOR_ENA, MOTOR_SPEED);
    
    // Wait for approximate position (needs encoder calibration)
    delay(500);
    
    // Stop motor
    analogWrite(MOTOR_ENA, 0);
    currentCompartment = compartment;
    
    delay(200);
}

bool dispensePill() {
    Serial.println("Dispensing pill...");
    
    for (int attempt = 0; attempt < MAX_DISPENSE_ATTEMPTS; attempt++) {
        // Activate magnet
        activateMagnet(true);
        delay(200);
        
        // Move servo to release position
        moveServo(90);
        delay(300);
        
        // Check if pill detected
        if (detectPill()) {
            Serial.println("Pill dispensed successfully");
            
            // Return servo to home
            moveServo(0);
            delay(200);
            
            // Deactivate magnet
            activateMagnet(false);
            
            return true;
        }
        
        // Return servo to home
        moveServo(0);
        delay(200);
        
        // Deactivate magnet
        activateMagnet(false);
        
        Serial.print("Attempt ");
        Serial.print(attempt + 1);
        Serial.println(" failed");
        
        delay(300);
    }
    
    Serial.println("Failed to dispense pill after max attempts");
    return false;
}

void activateMagnet(bool state) {
    digitalWrite(MAGNET_PIN, state ? HIGH : LOW);
    Serial.print("Magnet: ");
    Serial.println(state ? "ON" : "OFF");
}

void moveServo(int angle) {
    dispenserServo.write(angle);
    Serial.print("Servo: ");
    Serial.println(angle);
}

bool detectPill() {
    unsigned long startTime = millis();
    
    while (millis() - startTime < PILL_DETECT_TIMEOUT) {
        if (digitalRead(IR_SENSOR) == LOW) {
            Serial.println("Pill detected!");
            return true;
        }
        delay(10);
    }
    
    Serial.println("No pill detected");
    return false;
}

void updateLCD() {
    lcd.clear();
    lcd.print("Slot: ");
    lcd.print(selectedCompartment);
    lcd.print(" Ready");
    lcd.setCursor(0, 1);
    if (deviceConnected) {
        lcd.print("BLE: Connected");
    } else {
        lcd.print("BLE: Waiting...");
    }
}

void handleButtons() {
    unsigned long currentTime = millis();
    
    if (currentTime - lastButtonPress < DEBOUNCE_DELAY) {
        return;
    }
    
    // Button 1-5: Direct compartment selection
    if (digitalRead(BTN_1) == LOW) {
        selectedCompartment = 1;
        lastButtonPress = currentTime;
        updateLCD();
    }
    else if (digitalRead(BTN_2) == LOW) {
        selectedCompartment = 2;
        lastButtonPress = currentTime;
        updateLCD();
    }
    else if (digitalRead(BTN_3) == LOW) {
        selectedCompartment = 3;
        lastButtonPress = currentTime;
        updateLCD();
    }
    else if (digitalRead(BTN_5) == LOW) {
        selectedCompartment = 5;
        lastButtonPress = currentTime;
        updateLCD();
    }
    
    // Back button: Decrement compartment
    else if (digitalRead(BTN_BACK) == LOW) {
        selectedCompartment--;
        if (selectedCompartment < 1) selectedCompartment = NUM_COMPARTMENTS;
        lastButtonPress = currentTime;
        updateLCD();
    }
    
    // Select button: Dispense from selected compartment
    else if (digitalRead(BTN_SELECT) == LOW) {
        lcd.clear();
        lcd.print("Dispensing...");
        lcd.setCursor(0, 1);
        lcd.print("Slot ");
        lcd.print(selectedCompartment);
        
        moveToCompartment(selectedCompartment);
        
        if (dispensePill()) {
            dispensedCount[selectedCompartment - 1]++;
            lcd.clear();
            lcd.print("Success!");
            delay(1500);
        } else {
            lcd.clear();
            lcd.print("Failed!");
            delay(1500);
        }
        
        lastButtonPress = currentTime;
        updateLCD();
    }
}

void IRAM_ATTR encoderISR() {
    int a = digitalRead(ENCODER_C1);
    int b = digitalRead(ENCODER_C2);
    
    if (a != lastEncoderA) {
        if (b != a) {
            encoderPosition++;
        } else {
            encoderPosition--;
        }
    }
    lastEncoderA = a;
}

void IRAM_ATTR homeSwitchISR() {
    // Home switch triggered
    Serial.println("Home switch triggered");
}