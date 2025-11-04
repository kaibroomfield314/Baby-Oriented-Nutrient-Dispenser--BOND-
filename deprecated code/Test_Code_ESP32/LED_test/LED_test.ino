// --- Pin setup ---
const int BUTTON1_PIN = 13;   // Button 1 input (to GND)
const int BUTTON2_PIN = 14;   // Button 2 input (to GND)
const int LED1_PIN    = 2;   // LED 1 output
const int LED2_PIN    = 15;  // LED 2 output

void setup() {
  pinMode(BUTTON1_PIN, INPUT_PULLUP);  // uses internal pull-up
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  Serial.begin(115200);
  Serial.println("LED Button Test Ready");
}

void loop() {
  bool btn1 = digitalRead(BUTTON1_PIN) == LOW;  // pressed if pulled low
  bool btn2 = digitalRead(BUTTON2_PIN) == LOW;

  if (btn1) digitalWrite(LED1_PIN, HIGH);
  else      digitalWrite(LED1_PIN, LOW);

  if (btn2) digitalWrite(LED2_PIN, HIGH);
  else      digitalWrite(LED2_PIN, LOW);

  delay(50);  // simple debounce
}
