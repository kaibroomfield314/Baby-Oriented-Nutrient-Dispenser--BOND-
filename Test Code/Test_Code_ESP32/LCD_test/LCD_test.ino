#include <LiquidCrystal.h>

// Pin mapping (ESP32 → LCD)
const int LCD_RS = 19;   // RS
const int LCD_E  = 5;    // Enable
const int LCD_D4 = 21;   // D4
const int LCD_D5 = 22;   // D5
const int LCD_D6 = 23;   // D6
const int LCD_D7 = 32;   // D32

// Create LCD object (4-bit mode)
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup() {
  lcd.begin(16, 2);  // 16x2 LCD
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("LCD Debug Test");
  lcd.setCursor(0, 1);
  lcd.print("ESP32 connected!");

  delay(3000);
}

void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Counting...");
  for (int i = 0; i <= 9; i++) {
    lcd.setCursor(0, 1);
    lcd.print("Value: ");
    lcd.print(i);
    delay(1000);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Test complete!");
  lcd.setCursor(0, 1);
  lcd.print("Restarting...");
  delay(2000);
}
