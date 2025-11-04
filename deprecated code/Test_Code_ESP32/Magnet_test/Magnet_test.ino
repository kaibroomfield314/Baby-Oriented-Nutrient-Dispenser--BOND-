// ESP32 Test: Button 1 controls 12V Magnet via ULN2003
// Button 1: active LOW (connected to GND, using INPUT_PULLUP)
// Magnet: driven through ULN2003 on D12 (active HIGH)

#define BTN1  13      // Button 1 input pin
#define MAGNET 15     // Magnet control output (goes to ULN2003 IN1)

void setup() {
  pinMode(BTN1, INPUT_PULLUP);  // Button pulled up internally
  pinMode(MAGNET, OUTPUT);      // Magnet control output
  digitalWrite(MAGNET, LOW);    // Magnet off initially

  Serial.begin(115200);
  Serial.println("Button→Magnet Test Started");
}

void loop() {
  int button_state = digitalRead(BTN1);

  if (button_state == LOW) {
    digitalWrite(MAGNET, HIGH);   // Turn magnet ON
    Serial.println("Button 1 pressed → Magnet ON");
  } else {
    digitalWrite(MAGNET, LOW);    // Turn magnet OFF
  }

  delay(50); // small debounce delay
}
