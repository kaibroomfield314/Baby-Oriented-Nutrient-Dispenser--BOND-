// ESP32 + A4988 + Servo – 5-Position Indexer with Post-Move Servo Cycle
// Always-CW indexing. Half-step (if used) is set in hardware on the driver.
// Stepper: EN=D18, STEP=D17, DIR=D16
// Home switch: D12 (active-LOW to GND, INPUT_PULLUP)
// Start homing button: D26 (active-LOW)
// Position buttons: 1=D13, 2=D14, 3=D27, 4=GPIO36(VP, ext pull-up), 5=D33
//
// After moving to the requested position:
// - Wait 2 s
// - Sweep servo MIN_SAFE -> MAX_SAFE (full arc), wait 1 s, then sweep back MAX_SAFE -> MIN_SAFE
//
// Servo (memorised):
// SERVO_PIN=4, BTN1=13, BTN2=14, LED=2
// MIN_US=150, MAX_US=2100, END_MARGIN_US adjustable (default 0)
// STEP_US=60 us per step, STEP_DELAY_MS=1 ms between steps

#include <ESP32Servo.h>

// ---------------- Pins ----------------
#define EN_PIN      18
#define STEP_PIN    17
#define DIR_PIN     16
#define HOME_PIN    12
#define START_HOME  26

#define BTN1_PIN    13
#define BTN2_PIN    14
#define BTN3_PIN    27
#define BTN4_PIN    36   // VP/GPIO36: input-only, needs external 10k pull-up
#define BTN5_PIN    33

// Servo pins (memorised)
const int SERVO_PIN = 4;
const int LED_PIN   = 2;   // onboard LED heartbeat

// ---------------- Stepper config ----------------
const int steps_per_rev = 200;    
const int step_us       = 15000;
const int POSITIONS       = 5;
const int steps_per_index = steps_per_rev / POSITIONS; 

// Homing offset (degrees)
const float HOME_OFFSET_DEG = 17.0f; //adjust this value to position the table correctly on to the magnet

// Debounce
const uint32_t DEBOUNCE_MS = 30;

// ---------------- Servo config (memorised) ----------------
int MIN_US = 150;       // adjust if needed for your servo
int MAX_US = 2100;      // adjust if needed for your servo
int END_MARGIN_US = 0;  // back off from hard stops to avoid stall

int MIN_SAFE = 0;       // computed in setup
int MAX_SAFE = 0;

const int SERVO_STEP_US   = 60;  // larger = faster motion across arc
const int STEP_DELAY_MS   = 1;   // smaller = faster

// ---------------- State ----------------
bool homed = false;
int  current_index = 1;

Servo servo;

// ---------------- Helpers ----------------
inline bool home_active() { return digitalRead(HOME_PIN) == LOW; } // active-LOW

inline void set_enabled(bool on) {
  digitalWrite(EN_PIN, on ? LOW : HIGH); // LOW=enable for A4988/DRV8825
}

inline void set_dir_cw()  { digitalWrite(DIR_PIN, HIGH); }
inline void set_dir_ccw() { digitalWrite(DIR_PIN, LOW);  }

inline void step_once() {
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(step_us);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(step_us);
}

void step_cw(long n) {
  if (n <= 0) return;
  set_dir_cw();
  for (long i = 0; i < n; i++) {
    step_once();
    if ((i & 0xFF) == 0) yield();
  }
}

void step_ccw(long n) {
  if (n <= 0) return;
  set_dir_ccw();
  for (long i = 0; i < n; i++) {
    step_once();
    if ((i & 0xFF) == 0) yield();
  }
}

inline long deg_to_steps(float deg) {
  // Positive = CW, Negative = CCW
  double steps = (deg / 360.0) * (double)steps_per_rev;
  if (steps >= 0) return (long)(steps + 0.5);
  else            return (long)(steps - 0.5);
}

bool pressed_now(uint8_t pin, uint32_t &last_ts) {
  const bool low = (digitalRead(pin) == LOW);
  uint32_t t = millis();
  if (low && (t - last_ts) > DEBOUNCE_MS) { last_ts = t; return true; }
  return false;
}

// ---------------- Homing ----------------
void apply_home_offset_steps(long off_steps) {
  if (off_steps > 0) {
    step_cw(off_steps);
  } else if (off_steps < 0) {
    step_ccw(-off_steps);
  }
}

void do_homing() {
  Serial.print("Homing: CW until home switch... ");
  set_dir_cw();
  while (!home_active()) step_once();
  Serial.println("detected.");

  long off = deg_to_steps(HOME_OFFSET_DEG);
  apply_home_offset_steps(off);

  current_index = 1;
  homed = true;

  Serial.print("Homing complete. Index=1, home_offset_deg=");
  Serial.print(HOME_OFFSET_DEG, 3);
  Serial.print(" (");
  Serial.print(off);
  Serial.println(" steps)");
}

// ---------------- Servo motion ----------------
void servo_attach_if_needed() {
  if (!servo.attached()) {
    servo.attach(SERVO_PIN, MIN_SAFE, MAX_SAFE);
  }
}

void servo_move_to_us(int target_us) {
  servo_attach_if_needed();
  int cur = servo.readMicroseconds();
  if (cur < MIN_SAFE || cur > MAX_SAFE) {
    cur = MIN_SAFE; // clamp to a known good start
    servo.writeMicroseconds(cur);
    delay(5);
  }
  int step = (target_us >= cur) ? SERVO_STEP_US : -SERVO_STEP_US;
  for (int us = cur; (step > 0 ? us <= target_us : us >= target_us); us += step) {
    servo.writeMicroseconds(us);
    delay(STEP_DELAY_MS);
  }
}

void servo_full_arc_out_and_back() {
  // Ensure we start at MIN_SAFE, sweep to MAX_SAFE, wait, then sweep back
  servo_move_to_us(MIN_SAFE);
  servo_move_to_us(MAX_SAFE);
  delay(500); // 1 s at the end
  servo_move_to_us(MIN_SAFE);
}

// ---------------- Indexing + post-move cycle ----------------
void run_position_sequence_after_move() {
  delay(1000); // wait 2 seconds after reaching position
  servo_full_arc_out_and_back();
}

void move_to_index_cw(int target_index) {
  if (!homed) { Serial.println("Ignored: not homed."); return; }
  if (target_index < 1 || target_index > POSITIONS) return;

  int forward_slots = target_index - current_index;
  if (forward_slots < 0) forward_slots += POSITIONS;

  long steps = (long)forward_slots * (long)steps_per_index;

  Serial.print("CW move: "); Serial.print(current_index);
  Serial.print(" -> ");     Serial.print(target_index);
  Serial.print(" | steps=");Serial.println(steps);

  step_cw(steps);
  current_index = target_index;

  // After arriving, do the servo cycle
  run_position_sequence_after_move();

  Serial.print("Sequence complete at index "); Serial.println(current_index);
}

// ---------------- Setup / Loop ----------------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("=== 5-Pos Indexer + Servo Cycle ===");
  Serial.println("Press and release START (D26) to home. After homing, use 1..5 to index.");

  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(HOME_PIN, INPUT_PULLUP);
  pinMode(START_HOME, INPUT_PULLUP);

  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP);
  pinMode(BTN5_PIN, INPUT_PULLUP);
  pinMode(BTN4_PIN, INPUT); // VP with external pull-up

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  digitalWrite(STEP_PIN, LOW);
  set_enabled(true);

  // Compute safe endpoints from margins and attach servo
  MIN_SAFE = MIN_US + END_MARGIN_US;
  MAX_SAFE = MAX_US - END_MARGIN_US;
  servo.attach(SERVO_PIN, MIN_SAFE, MAX_SAFE);
  servo.writeMicroseconds(MIN_SAFE);

  Serial.print("steps_per_rev="); Serial.print(steps_per_rev);
  Serial.print(", steps_per_index="); Serial.println(steps_per_index);
  Serial.print("HOME_OFFSET_DEG="); Serial.println(HOME_OFFSET_DEG, 3);
  Serial.print("Servo MIN_SAFE="); Serial.print(MIN_SAFE);
  Serial.print(" us, MAX_SAFE="); Serial.print(MAX_SAFE);
  Serial.println(" us");
}

void loop() {
  static uint32_t ts_home=0, ts1=0, ts2=0, ts3=0, ts4=0, ts5=0;

  // Tiny heartbeat
  static uint32_t hb=0; 
  if (millis() - hb > 500) { hb = millis(); digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }

  if (pressed_now(START_HOME, ts_home)) do_homing();

  if (pressed_now(BTN1_PIN, ts1)) move_to_index_cw(1);
  if (pressed_now(BTN2_PIN, ts2)) move_to_index_cw(2);
  if (pressed_now(BTN3_PIN, ts3)) move_to_index_cw(3);
  if (pressed_now(BTN4_PIN, ts4)) move_to_index_cw(4);
  if (pressed_now(BTN5_PIN, ts5)) move_to_index_cw(5);
}
