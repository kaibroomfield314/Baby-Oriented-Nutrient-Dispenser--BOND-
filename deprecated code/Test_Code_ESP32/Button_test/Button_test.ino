// ESP32 Button + Home Switch + IR Sensor + LED State Test
// Open Serial Monitor at 115200 baud

// --- Pin assignments ---
#define HOME_SWITCH 12
#define BTN1        13
#define BTN2        14
#define BTN3        27
#define BTN4        36    // VP: input-only, no internal pull-ups
#define BTN5        33
#define BTN_BACK    25
#define BTN_SELECT  26
#define IR_PIN      35    // Input-only, no internal pull-ups
#define LED1_PIN     2    // LED output (on when any button pressed)

// --- Previous states (for change detection) ---
int prev_home     = HIGH;
int prev_btn1     = HIGH;
int prev_btn2     = HIGH;
int prev_btn3     = HIGH;
int prev_btn4     = HIGH;
int prev_btn5     = HIGH;
int prev_back     = HIGH;
int prev_select   = HIGH;
int prev_ir       = HIGH;

// --- Simple helper to print edge changes ---
void print_change(const char* name, int prev_state, int curr_state, bool active_low = true) {
  if (curr_state != prev_state) {
    if (active_low) {
      if (curr_state == LOW)  Serial.printf("%s pressed\n", name);
      else                    Serial.printf("%s released\n", name);
    } else {
      if (curr_state == HIGH) Serial.printf("%s ACTIVE (HIGH)\n", name);
      else                    Serial.printf("%s INACTIVE (LOW)\n", name);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("=== ESP32 Button + Home + IR + LED Test ===");

  // Buttons with internal pull-ups available
  pinMode(HOME_SWITCH, INPUT_PULLUP);
  pinMode(BTN1,        INPUT_PULLUP);
  pinMode(BTN2,        INPUT_PULLUP);
  pinMode(BTN3,        INPUT_PULLUP);
  pinMode(BTN5,        INPUT_PULLUP);
  pinMode(BTN_BACK,    INPUT_PULLUP);
  pinMode(BTN_SELECT,  INPUT_PULLUP);

  // GPIO36 & GPIO35 (VP/VN bank) are input-only and have NO internal pull resistors
  pinMode(BTN4, INPUT);   // requires external pull-up/down in hardware
  pinMode(IR_PIN, INPUT); // requires external pull-up/down in hardware

  // LED output
  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);

  // Seed previous states
  prev_home   = digitalRead(HOME_SWITCH);
  prev_btn1   = digitalRead(BTN1);
  prev_btn2   = digitalRead(BTN2);
  prev_btn3   = digitalRead(BTN3);
  prev_btn4   = digitalRead(BTN4);
  prev_btn5   = digitalRead(BTN5);
  prev_back   = digitalRead(BTN_BACK);
  prev_select = digitalRead(BTN_SELECT);
  prev_ir     = digitalRead(IR_PIN);

  Serial.println("Ready. LED on D2 lights when any button or sensor is active.");
  Serial.println("Note: BTN4 (GPIO36) & IR (GPIO35) need external pull resistors.");
}

void loop() {
  // Read all
  int curr_home   = digitalRead(HOME_SWITCH);
  int curr_btn1   = digitalRead(BTN1);
  int curr_btn2   = digitalRead(BTN2);
  int curr_btn3   = digitalRead(BTN3);
  int curr_btn4   = digitalRead(BTN4);
  int curr_btn5   = digitalRead(BTN5);
  int curr_back   = digitalRead(BTN_BACK);
  int curr_select = digitalRead(BTN_SELECT);
  int curr_ir     = digitalRead(IR_PIN);

  // Report changes (active LOW semantics for buttons/home/IR)
  print_change("Home Switch",   prev_home,   curr_home,   true);
  print_change("Button 1",      prev_btn1,   curr_btn1,   true);
  print_change("Button 2",      prev_btn2,   curr_btn2,   true);
  print_change("Button 3",      prev_btn3,   curr_btn3,   true);
  print_change("Button 4",      prev_btn4,   curr_btn4,   true);
  print_change("Button 5",      prev_btn5,   curr_btn5,   true);
  print_change("Button BACK",   prev_back,   curr_back,   true);
  print_change("Button SELECT", prev_select, curr_select, true);
  print_change("IR Sensor",     prev_ir,     curr_ir,     true);

  // --- LED control: ON if any input is active (LOW) ---
  bool any_pressed = (
    curr_home   == LOW ||
    curr_btn1   == LOW ||
    curr_btn2   == LOW ||
    curr_btn3   == LOW ||
    curr_btn4   == LOW ||
    curr_btn5   == LOW ||
    curr_back   == LOW ||
    curr_select == LOW ||
    curr_ir     == LOW
  );

  digitalWrite(LED1_PIN, any_pressed ? HIGH : LOW);

  // Update previous states
  prev_home   = curr_home;
  prev_btn1   = curr_btn1;
  prev_btn2   = curr_btn2;
  prev_btn3   = curr_btn3;
  prev_btn4   = curr_btn4;
  prev_btn5   = curr_btn5;
  prev_back   = curr_back;
  prev_select = curr_select;
  prev_ir     = curr_ir;

  delay(25); // light debounce/filtering
}
