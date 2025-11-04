#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <LiquidCrystal.h>
#include "Config.h"
#include "ConfigurationSettings.h"

/**
 * Button action enumeration
 */
enum ButtonAction {
    NO_BUTTON_PRESSED,
    COMPARTMENT_1_SELECTED,
    COMPARTMENT_2_SELECTED,
    COMPARTMENT_3_SELECTED,
    COMPARTMENT_4_SELECTED,
    COMPARTMENT_5_SELECTED,
    NAVIGATION_BACK_PRESSED,
    NAVIGATION_SELECT_PRESSED
};

/**
 * UIManager Class
 * 
 * Responsible for all user interface operations including:
 * - LCD display updates and formatting
 * - Button input handling with debouncing
 * - Status message display
 * 
 * This class handles UI concerns without direct hardware control logic (low coupling).
 */
class UIManager {
private:
    SystemConfiguration* systemConfiguration;
    LiquidCrystal lcdDisplay;
    
    // Button state tracking
    unsigned long timeOfLastButtonPressMilliseconds;
    int currentlySelectedCompartmentNumber;
    
public:
    /**
     * Constructor
     * @param config Pointer to system configuration
     */
    UIManager(SystemConfiguration* config) 
        : systemConfiguration(config),
          lcdDisplay(PIN_FOR_LCD_REGISTER_SELECT, 
                    PIN_FOR_LCD_ENABLE_SIGNAL,
                    PIN_FOR_LCD_DATA_BIT_4,
                    PIN_FOR_LCD_DATA_BIT_5,
                    PIN_FOR_LCD_DATA_BIT_6,
                    PIN_FOR_LCD_DATA_BIT_7) {
        timeOfLastButtonPressMilliseconds = 0;
        currentlySelectedCompartmentNumber = 1;
    }
    
    /**
     * Initialize LCD display and button pins
     */
    void initializeLCDAndButtonPins() {
        // Initialize LCD
        lcdDisplay.begin(LCD_NUMBER_OF_COLUMNS, LCD_NUMBER_OF_ROWS);
        lcdDisplay.clear();
        
        // Configure button pins with internal pull-up resistors
        pinMode(PIN_FOR_COMPARTMENT_BUTTON_1, INPUT_PULLUP);
        pinMode(PIN_FOR_COMPARTMENT_BUTTON_2, INPUT_PULLUP);
        pinMode(PIN_FOR_COMPARTMENT_BUTTON_3, INPUT_PULLUP);
        // Note: GPIO36 (VP) is input-only and lacks internal pull-ups
        pinMode(PIN_FOR_COMPARTMENT_BUTTON_4, INPUT);
        pinMode(PIN_FOR_COMPARTMENT_BUTTON_5, INPUT_PULLUP);
        pinMode(PIN_FOR_NAVIGATION_BACK_BUTTON, INPUT_PULLUP);
        pinMode(PIN_FOR_NAVIGATION_SELECT_BUTTON, INPUT_PULLUP);
        
        Serial.println("LCD and button pins initialized");
    }
    
    // ========================================================================
    // LCD Display Methods
    // ========================================================================
    
    /**
     * Clear the LCD display completely
     */
    void clearLCDDisplay() {
        lcdDisplay.clear();
    }
    
    /**
     * Display initialization message
     */
    void displayInitializationMessage() {
        lcdDisplay.clear();
        lcdDisplay.setCursor(0, 0);
        lcdDisplay.print("Pill Dispenser");
        lcdDisplay.setCursor(0, 1);
        lcdDisplay.print("Initializing...");
    }
    
    /**
     * Display homing in progress message
     */
    void displayHomingInProgressMessage() {
        lcdDisplay.clear();
        lcdDisplay.setCursor(0, 0);
        lcdDisplay.print("Homing...");
    }
    
    /**
     * Display homing complete message
     */
    void displayHomingCompleteMessage() {
        lcdDisplay.setCursor(0, 1);
        lcdDisplay.print("Home: OK        ");
    }
    
    /**
     * Display ready status with compartment selection
     * @param selectedCompartment Currently selected compartment number
     * @param isBluetoothConnected Whether BLE device is connected
     */
    void displayReadyStatusWithCompartmentSelection(int selectedCompartment, bool isBluetoothConnected) {
        lcdDisplay.clear();
        lcdDisplay.setCursor(0, 0);
        lcdDisplay.print("Slot: ");
        lcdDisplay.print(selectedCompartment);
        lcdDisplay.print(" Ready  ");
        
        lcdDisplay.setCursor(0, 1);
        if (isBluetoothConnected) {
            lcdDisplay.print("BLE: Connected  ");
        } else {
            lcdDisplay.print("BLE: Waiting... ");
        }
    }
    
    /**
     * Display dispensing in progress message
     * @param compartmentNumber Compartment being dispensed from
     */
    void displayDispensingInProgressMessage(int compartmentNumber) {
        lcdDisplay.clear();
        lcdDisplay.setCursor(0, 0);
        lcdDisplay.print("Dispensing...   ");
        lcdDisplay.setCursor(0, 1);
        lcdDisplay.print("Slot ");
        lcdDisplay.print(compartmentNumber);
        lcdDisplay.print("          ");
    }
    
    /**
     * Display success message
     */
    void displaySuccessMessage() {
        lcdDisplay.clear();
        lcdDisplay.setCursor(0, 0);
        lcdDisplay.print("Success!        ");
    }
    
    /**
     * Display failure/error message
     */
    void displayFailureMessage() {
        lcdDisplay.clear();
        lcdDisplay.setCursor(0, 0);
        lcdDisplay.print("Failed!         ");
    }
    
    /**
     * Display BLE connected status on second row
     */
    void displayBluetoothConnectedStatus() {
        lcdDisplay.setCursor(0, 1);
        lcdDisplay.print("BLE: Connected  ");
    }
    
    /**
     * Display BLE waiting status on second row
     */
    void displayBluetoothWaitingStatus() {
        lcdDisplay.setCursor(0, 1);
        lcdDisplay.print("BLE: Waiting... ");
    }
    
    /**
     * Display custom message on LCD
     * @param row Row number (0 or 1)
     * @param message Message to display (will be padded/truncated to 16 chars)
     */
    void displayCustomMessageOnRow(int row, String message) {
        lcdDisplay.setCursor(0, row);
        // Pad or truncate message to LCD width
        while (message.length() < LCD_NUMBER_OF_COLUMNS) {
            message += " ";
        }
        if (message.length() > LCD_NUMBER_OF_COLUMNS) {
            message = message.substring(0, LCD_NUMBER_OF_COLUMNS);
        }
        lcdDisplay.print(message);
    }
    
    // ========================================================================
    // Button Input Methods
    // ========================================================================
    
    /**
     * Check if any button is pressed and return the action (with debouncing)
     * @return ButtonAction enum indicating which button was pressed
     */
    ButtonAction checkIfAnyButtonPressedWithDebounce() {
        unsigned long currentTimeMilliseconds = millis();
        
        // Check if debounce period has elapsed
        if (currentTimeMilliseconds - timeOfLastButtonPressMilliseconds < 
            systemConfiguration->buttonDebounceDelayMilliseconds) {
            return NO_BUTTON_PRESSED;
        }
        
        // Check each button (LOW = pressed with pull-up resistor)
        if (digitalRead(PIN_FOR_COMPARTMENT_BUTTON_1) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return COMPARTMENT_1_SELECTED;
        }
        else if (digitalRead(PIN_FOR_COMPARTMENT_BUTTON_2) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return COMPARTMENT_2_SELECTED;
        }
        else if (digitalRead(PIN_FOR_COMPARTMENT_BUTTON_3) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return COMPARTMENT_3_SELECTED;
        }
        else if (digitalRead(PIN_FOR_COMPARTMENT_BUTTON_4) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return COMPARTMENT_4_SELECTED;
        }
        else if (digitalRead(PIN_FOR_COMPARTMENT_BUTTON_5) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return COMPARTMENT_5_SELECTED;
        }
        else if (digitalRead(PIN_FOR_NAVIGATION_BACK_BUTTON) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return NAVIGATION_BACK_PRESSED;
        }
        else if (digitalRead(PIN_FOR_NAVIGATION_SELECT_BUTTON) == LOW) {
            timeOfLastButtonPressMilliseconds = currentTimeMilliseconds;
            return NAVIGATION_SELECT_PRESSED;
        }
        
        return NO_BUTTON_PRESSED;
    }
    
    /**
     * Get currently selected compartment number
     * @return Selected compartment (1-based)
     */
    int getCurrentlySelectedCompartmentNumber() {
        return currentlySelectedCompartmentNumber;
    }
    
    /**
     * Set currently selected compartment number
     * @param compartmentNumber New compartment selection (1-based)
     */
    void setCurrentlySelectedCompartmentNumber(int compartmentNumber) {
        currentlySelectedCompartmentNumber = compartmentNumber;
    }
    
    /**
     * Increment selected compartment (with wraparound)
     * @param maxCompartments Maximum number of compartments
     */
    void incrementSelectedCompartmentWithWraparound(int maxCompartments) {
        currentlySelectedCompartmentNumber++;
        if (currentlySelectedCompartmentNumber > maxCompartments) {
            currentlySelectedCompartmentNumber = 1;
        }
    }
    
    /**
     * Decrement selected compartment (with wraparound)
     * @param maxCompartments Maximum number of compartments
     */
    void decrementSelectedCompartmentWithWraparound(int maxCompartments) {
        currentlySelectedCompartmentNumber--;
        if (currentlySelectedCompartmentNumber < 1) {
            currentlySelectedCompartmentNumber = maxCompartments;
        }
    }
    
    /**
     * Handle button action and update selection
     * @param action Button action that occurred
     * @param maxCompartments Maximum number of compartments
     */
    void handleButtonActionAndUpdateSelection(ButtonAction action, int maxCompartments) {
        switch (action) {
            case COMPARTMENT_1_SELECTED:
                currentlySelectedCompartmentNumber = 1;
                break;
                
            case COMPARTMENT_2_SELECTED:
                currentlySelectedCompartmentNumber = 2;
                break;
                
            case COMPARTMENT_3_SELECTED:
                currentlySelectedCompartmentNumber = 3;
                break;
                
            case COMPARTMENT_4_SELECTED:
                currentlySelectedCompartmentNumber = 4;
                break;
                
            case COMPARTMENT_5_SELECTED:
                currentlySelectedCompartmentNumber = 5;
                break;
                
            case NAVIGATION_BACK_PRESSED:
                decrementSelectedCompartmentWithWraparound(maxCompartments);
                break;
                
            case NO_BUTTON_PRESSED:
            case NAVIGATION_SELECT_PRESSED:
            default:
                // No change to selection
                break;
        }
    }
};

#endif // UI_MANAGER_H

