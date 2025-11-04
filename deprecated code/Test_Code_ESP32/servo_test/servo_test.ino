#include <ESP32Servo.h>

Servo servo;

const int SERVO_PIN = 4;
const int BTN1_PIN  = 13;
const int BTN2_PIN  = 14;

int angle_deg = 90;
int last_sent_deg = -1;

unsigned long last_input_ms = 0;
const unsigned long idle_detach_ms = 1500;  // relax after 1.5 s idle
bool attached_now = false;

void safe_attach() {
  if (!attached_now) {
    servo.setPeriodHertz(50);
    // Start conservative; expand only if needed to reach your endpoints
    servo.attach(SERVO_PIN, 1000, 2000);
    attached_now = true;
  }
}

void safe_detach() {
  if (attached_now) {
    servo.detach();
    attached_now = false;
  }
}

void setup() {
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  safe_attach();
  servo.write(angle_deg);
  last_sent_deg = angle_deg;
}

void loop() {
  bool b1 = digitalRead(BTN1_PIN) == LOW;
  bool b2 = digitalRead(BTN2_PIN) == LOW;

  // adjust only on active input
  if (b1 ^ b2) {                // exactly one pressed
    last_input_ms = millis();
    safe_attach();              // ensure attached while moving
    if (b1) angle_deg += 2;
    if (b2) angle_deg -= 2;
    if (angle_deg < 0)   angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;
  }

  // send command only if changed
  if (attached_now && angle_deg != last_sent_deg) {
    servo.write(angle_deg);
    last_sent_deg = angle_deg;
  }

  // relax after idle to stop holding hum (optional)
  if (attached_now && (millis() - last_input_ms > idle_detach_ms)) {
    safe_detach();
  }

  delay(20); // ~50 Hz scan; OK since we only write on change
}
