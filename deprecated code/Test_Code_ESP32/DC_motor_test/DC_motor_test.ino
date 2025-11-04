// DC motor debug with encoder angle + button edge debug (ESP32, analogWrite path, FIXED PWM)
//
// Pins:
// D13 -> Button 1 (to GND, INPUT_PULLUP)
// D14 -> Button 2 (to GND, INPUT_PULLUP)
// VN  -> Encoder A / C1 (GPIO39, input-only)
// D34 -> Encoder B / C2 (GPIO34, input-only)
// D17 -> H-Bridge IN1
// D18 -> H-Bridge IN2
// D16 -> H-Bridge ENA (PWM via analogWrite)
//
// Behavior:
// - Hold Button 1: FORWARD at fixed PWM duty.
// - Hold Button 2: REVERSE at fixed PWM duty.
// - Both pressed: STOP.
// - None pressed: STOP.
// - Prints direction, duty, angle, and tick delta every 500 ms.
// - Prints button edge events when they happen.

#include <Arduino.h>

// ---- User settings ----
static const long TICKS_PER_REV = 8338L;    // <-- set to your encoder CPR
static const int  PWM_DUTY_FIXED = 230;     // 0..255 -> ~78% duty (change as desired)

// ---- Pins ----
static const uint8_t PIN_BTN1 = 13;
static const uint8_t PIN_BTN2 = 14;
static const uint8_t PIN_ENC_A = 39;  // VN (input-only)
static const uint8_t PIN_ENC_B = 34;  // D34 (input-only)
static const uint8_t PIN_IN1  = 17;
static const uint8_t PIN_IN2  = 18;
static const uint8_t PIN_PWM  = 16;

// ---- Motor control ----
enum Dir { STOP, FWD, REV };
volatile Dir dir = STOP;

// ---- Encoder state ----
volatile long enc_ticks = 0;   // signed tick count
volatile long last_isr_ticks = 0;

// ---- Button edge debug ----
bool last_b1 = false;
bool last_b2 = false;

// ---- Helpers ----
void motor_dir(Dir d) {
  dir = d;
  switch (d) {
    case FWD:  digitalWrite(PIN_IN1, HIGH); digitalWrite(PIN_IN2, LOW);  break;
    case REV:  digitalWrite(PIN_IN1, LOW);  digitalWrite(PIN_IN2, HIGH); break;
    default:   digitalWrite(PIN_IN1, LOW);  digitalWrite(PIN_IN2, LOW);  break; // coast
  }
}

void IRAM_ATTR enc_a_isr() {
  // Quadrature: on rising edge of A, read B to infer direction
  int b = digitalRead(PIN_ENC_B);
  if (b) enc_ticks++; else enc_ticks--;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Buttons (active-low)
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  last_b1 = (digitalRead(PIN_BTN1) == LOW);
  last_b2 = (digitalRead(PIN_BTN2) == LOW);

  // H-bridge
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_PWM, OUTPUT);
  motor_dir(STOP);
  analogWrite(PIN_PWM, 0);   // ESP32 core maps to LEDC under the hood

  // Encoder
  pinMode(PIN_ENC_A, INPUT);
  pinMode(PIN_ENC_B, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), enc_a_isr, RISING);

  Serial.println("DC motor + encoder debug (fixed PWM) ready.");
}

void loop() {
  bool b1 = (digitalRead(PIN_BTN1) == LOW);
  bool b2 = (digitalRead(PIN_BTN2) == LOW);

  // Edge debug
  if (b1 != last_b1) {
    Serial.printf("Button1 %s\n", b1 ? "PRESSED" : "RELEASED");
    last_b1 = b1;
  }
  if (b2 != last_b2) {
    Serial.printf("Button2 %s\n", b2 ? "PRESSED" : "RELEASED");
    last_b2 = b2;
  }

  // Motor control with fixed duty
  int duty_to_write = 0;
  if (b1 && b2) {
    motor_dir(STOP);
    duty_to_write = 0;
  } else if (b1) {
    motor_dir(FWD);
    duty_to_write = PWM_DUTY_FIXED;
  } else if (b2) {
    motor_dir(REV);
    duty_to_write = PWM_DUTY_FIXED;
  } else {
    motor_dir(STOP);
    duty_to_write = 0;
  }
  analogWrite(PIN_PWM, duty_to_write);

  // Periodic status (every 500 ms)
  static uint32_t t0 = 0;
  if (millis() - t0 > 500) {
    long ticks_now = enc_ticks;
    long delta = ticks_now - last_isr_ticks;
    last_isr_ticks = ticks_now;

    // Angle in degrees (wrap to [0, 360))
    double angle_deg = 0.0;
    if (TICKS_PER_REV != 0) {
      angle_deg = fmod((double)ticks_now * 360.0 / (double)TICKS_PER_REV, 360.0);
      if (angle_deg < 0) angle_deg += 360.0;
    }

    const char* dstr = (dir == FWD) ? "FWD" : (dir == REV) ? "REV" : "STOP";
    int duty_pct = (100 * duty_to_write) / 255;

    Serial.printf("Dir=%s  Duty=%d%%  Ticks=%ld  dTicks=+%ld  Angle=%.1f deg  B=%d\n",
                  dstr, duty_pct, ticks_now, delta, angle_deg, digitalRead(PIN_ENC_B));

    t0 = millis();
  }

  delay(50);  // faster scan, no ramping needed
}
