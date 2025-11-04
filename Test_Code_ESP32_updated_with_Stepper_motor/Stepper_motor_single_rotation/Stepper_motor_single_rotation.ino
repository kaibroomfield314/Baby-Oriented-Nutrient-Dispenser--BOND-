// ESP32 + A4988 – Simple 5-Position Indexer (Always CW) with Adjustable Home Offset
// Half-step is set in hardware (MS1=HIGH, MS2=LOW, MS3=LOW) if you want 400 steps/rev.
// EN=D18, STEP=D17, DIR=D16
// HOME switch=D12 (active-LOW to GND, INPUT_PULLUP)
// START HOMING button=D26 (active-LOW to GND, INPUT_PULLUP)
// Position buttons: 1=D13, 2=D14, 3=D27, 4=GPIO36(VP, ext pull-up), 5=D33
//
// Serial: type a signed number in degrees (e.g., "2.0" or "-1.5") + Enter to set HOME OFFSET.
// The offset is applied after the home switch triggers to fine-align position 1 at the exit.

#define EN_PIN      18
#define STEP_PIN    17
#define DIR_PIN     16
#define HOME_PIN    12
#define START_HOME  26

#define BTN1_PIN    13
#define BTN2_PIN    14
#define BTN3_PIN    27
#define BTN4_PIN    36   // VP/GPIO36: input-only, needs external 10k to 3.3V, button to GND
#define BTN5_PIN    33

// ---- Motion/config (your requested starting constants) ----
const int steps_per_rev = 200;     // If using half-step in hardware, set this to 400
const int step_us       = 15000;   // Pulse HIGH and LOW time (µs). Same speed for everything.

const int POSITIONS       = 5;
const int steps_per_index = steps_per_rev / POSITIONS; // 40 if 200/rev; 80 if 400/rev

// ---- Debounce ----
const uint32_t DEBOUNCE_MS = 30;

// ---- State ----
bool  homed = false;
int   current_index = 1;
float home_offset_deg = 0.0f;      // adjustable via Serial
bool  apply_home_offset_now = false; // optional: apply immediately if already homed

// ---- Serial input buffer ----
char inbuf[24];
int  inlen = 0;

// ---- Helpers ----
inline bool home_active() { return digitalRead(HOME_PIN) == LOW; } // active-LOW

inline void set_enabled(bool on) {
  digitalWrite(EN_PIN, on ? LOW : HIGH); // LOW = enabled on A4988/DRV8825
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
  // Positive deg = CW steps, negative deg = CCW steps
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

void apply_home_offset_steps(long off_steps) {
  if (off_steps > 0) {
    Serial.print("Apply CW home offset steps: "); Serial.println(off_steps);
    step_cw(off_steps);
  } else if (off_steps < 0) {
    long ccw = -off_steps;
    Serial.print("Apply CCW home offset steps: "); Serial.println(ccw);
    step_ccw(ccw);
  } else {
    Serial.println("Home offset: 0 steps");
  }
}

void do_homing() {
  Serial.print("Homing: CW until home switch... ");
  set_dir_cw();
  while (!home_active()) step_once();
  Serial.println("detected.");

  long off = deg_to_steps(home_offset_deg);
  apply_home_offset_steps(off);

  current_index = 1; // define this aligned point as logical index 1
  homed = true;

  Serial.print("Homing complete. Index = 1, home_offset_deg = ");
  Serial.print(home_offset_deg, 3);
  Serial.print(" (");
  Serial.print(off);
  Serial.println(" steps)");
  Serial.println("Press 1..5 to index compartments (always CW).");
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

  if (apply_home_offset_now && target_index == 1 && home_offset_deg != 0.0f) {
    // Optional: if you tweak offset after homing, you can apply it the next time you revisit index 1
    long off = deg_to_steps(home_offset_deg);
    apply_home_offset_steps(off);
    Serial.println("Applied updated home offset at index 1.");
  }
}

// --- Serial parsing (offset in degrees) ---
void try_parse_home_offset_deg() {
  inbuf[inlen] = '\0';
  char *endp = nullptr;
  double v = strtod(inbuf, &endp);

  if (endp != inbuf) {
    // Accept a reasonable tuning window, e.g., ±30°
    if (v >= -30.0 && v <= 30.0) {
      home_offset_deg = (float)v;
      Serial.print("home_offset_deg set to "); Serial.print(home_offset_deg, 3);
      Serial.print(" deg ("); Serial.print(deg_to_steps(home_offset_deg));
      Serial.println(" steps).");
      if (homed) {
        Serial.println("Note: offset takes effect on next homing. "
                       "Set apply_home_offset_now=true in code to apply when returning to index 1.");
      }
    } else {
      Serial.println("Invalid offset. Enter a value in degrees between -30.0 and +30.0.");
    }
  } else {
    Serial.println("Parse error. Enter a number like 2.0 or -1.5");
  }
  inlen = 0;
}

void poll_serial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { try_parse_home_offset_deg(); }
    else if (inlen < (int)sizeof(inbuf)-1) { inbuf[inlen++] = c; }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("=== Simple 5-Position Indexer (Always CW) – Adjustable Home Offset ===");
  Serial.println("Type a signed number (degrees) and press Enter to set home offset (e.g., 2.0 or -1.5).");
  Serial.println("Hold START (D26) to home; press 1..5 to index after homing.");

  pinMode(EN_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(HOME_PIN, INPUT_PULLUP);
  pinMode(START_HOME, INPUT_PULLUP);

  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP);
  pinMode(BTN5_PIN, INPUT_PULLUP);
  pinMode(BTN4_PIN, INPUT); // VP requires external pull-up

  digitalWrite(STEP_PIN, LOW);
  set_enabled(true);

  Serial.print("steps_per_rev="); Serial.print(steps_per_rev);
  Serial.print(", steps_per_index="); Serial.println(steps_per_index);
  Serial.print("Initial home_offset_deg="); Serial.println(home_offset_deg, 3);
}

void loop() {
  static uint32_t ts_home=0, ts1=0, ts2=0, ts3=0, ts4=0, ts5=0;

  poll_serial();

  if (pressed_now(START_HOME, ts_home)) do_homing();

  if (pressed_now(BTN1_PIN, ts1)) move_to_index_cw(1);
  if (pressed_now(BTN2_PIN, ts2)) move_to_index_cw(2);
  if (pressed_now(BTN3_PIN, ts3)) move_to_index_cw(3);
  if (pressed_now(BTN4_PIN, ts4)) move_to_index_cw(4);
  if (pressed_now(BTN5_PIN, ts5)) move_to_index_cw(5);
}
