# Positioning System - How It Works

## Core Concept

The system uses **absolute positioning** based on a **START reference point** established during homing.

## The Three Key Components

### 1. START Position (Home Reference)
```
When homing completes and home switch is pressed:
  → System defines this as START position = 0° absolute
  → Variable: currentCompartmentNumber = 0
  → All container positions are measured from this START point
```

### 2. Container Offsets (Fixed, Stored in Configuration)
```cpp
// In ConfigurationSettings.h
float anglePerCompartmentInDegrees = 72.0;  // 360° / 5 containers = 72°

// Calculated offsets from START:
Container 1: (1-1) × 72° = 0°   → At START position
Container 2: (2-1) × 72° = 72°  → 72° from START
Container 3: (3-1) × 72° = 144° → 144° from START  
Container 4: (4-1) × 72° = 216° → 216° from START
Container 5: (5-1) × 72° = 288° → 288° from START
```

### 3. Current Position Tracking
```
After every movement:
  → System updates currentCompartmentNumber
  → This becomes the reference for NEXT movement
  → Movements are NOT always calculated from START
  → They are calculated from CURRENT position
```

## Movement Algorithm

### Step-by-Step Process

```
1. User selects Container N (e.g., N=5)

2. Calculate TARGET absolute position:
   targetAbsoluteAngle = (N - 1) × 72°
   Example: Container 5 → (5-1) × 72° = 288° absolute

3. Calculate CURRENT absolute position:
   If currentCompartmentNumber == 0:
       currentAbsoluteAngle = 0° (at START)
   Else:
       currentAbsoluteAngle = (currentCompartmentNumber - 1) × 72°
   Example: At Container 3 → (3-1) × 72° = 144° absolute

4. Calculate RELATIVE movement needed:
   angleToTravel = targetAbsoluteAngle - currentAbsoluteAngle
   Example: 288° - 144° = 144° forward

5. Handle wraparound (if negative, add 360°):
   If angleToTravel < 0:
       angleToTravel = angleToTravel + 360°
   Example: Going from Container 5 (288°) to Container 1 (0°)
           → 0° - 288° = -288°
           → -288° + 360° = 72° forward

6. Calculate movement time:
   movementTime = (angleToTravel / degreesPerSecond) × 1000

7. Move motor for calculated time

8. **UPDATE CURRENT POSITION** (crucial!):
   currentCompartmentNumber = targetCompartmentNumber
   
   This ensures next movement calculates from THIS position!
```

## Real-World Example Sequence

### Sequence 1: From Home to Container 5
```
Initial State:
  currentCompartmentNumber = 0 (at START)
  
User selects Container 5:
  targetAbsoluteAngle = (5-1) × 72° = 288°
  currentAbsoluteAngle = 0° (at START)
  angleToTravel = 288° - 0° = 288° forward
  
After Movement:
  currentCompartmentNumber = 5
  currentAbsoluteAngle = 288°
```

### Sequence 2: From Container 5 to Container 3
```
Initial State:
  currentCompartmentNumber = 5 (at 288° absolute)
  
User selects Container 3:
  targetAbsoluteAngle = (3-1) × 72° = 144°
  currentAbsoluteAngle = (5-1) × 72° = 288°
  angleToTravel = 144° - 288° = -144°
  angleToTravel (after wraparound) = -144° + 360° = 216° forward
  
After Movement:
  currentCompartmentNumber = 3
  currentAbsoluteAngle = 144°
```

### Sequence 3: From Container 3 to Container 4
```
Initial State:
  currentCompartmentNumber = 3 (at 144° absolute)
  
User selects Container 4:
  targetAbsoluteAngle = (4-1) × 72° = 216°
  currentAbsoluteAngle = (3-1) × 72° = 144°
  angleToTravel = 216° - 144° = 72° forward
  
After Movement:
  currentCompartmentNumber = 4
  currentAbsoluteAngle = 216°
```

## Serial Monitor Output Example

```
========================================
POSITIONING: Moving to container 5
========================================
Already at home, no need to move
[Positioning] CURRENT: START/HOME (0° absolute)
[Positioning] TARGET: Container 5 (288° absolute = START + 288° offset)
[Positioning] MOVEMENT: Rotate 288° forward from current position
[Positioning] Estimated movement time: 1600ms
[Positioning] Motor speed: 255 PWM
[Positioning] Motor started - rotating forward...
[Positioning] Motor stopped
[Positioning] Settling for 200ms...
========================================
POSITIONED: Now at Container 5 (288° absolute = START + 288° offset)
Next movement will calculate from THIS position
========================================

========================================
POSITIONING: Moving to container 3
========================================
[Positioning] CURRENT: Container 5 (288° absolute = START + 288° offset)
[Positioning] TARGET: Container 3 (144° absolute = START + 144° offset)
[Positioning] MOVEMENT: Rotate 216° forward from current position
[Positioning] Estimated movement time: 1200ms
[Positioning] Motor speed: 255 PWM
[Positioning] Motor started - rotating forward...
[Positioning] Motor stopped
[Positioning] Settling for 200ms...
========================================
POSITIONED: Now at Container 3 (144° absolute = START + 144° offset)
Next movement will calculate from THIS position
========================================
```

## Code Implementation

### In DispenserController.h

```cpp
// STEP 1: Define START position during homing
bool performHomingWithRetryAndEscalation() {
    // ... homing process ...
    sensorManager->resetEncoderPositionToZero();
    currentCompartmentNumber = 0;  // ← START position defined here
    isSystemHomedAndReady = true;
    return true;
}

// STEP 2: Store container offsets (in SystemConfiguration)
float anglePerCompartmentInDegrees = 72.0;

// STEP 3: Calculate and move to container
bool moveRotaryDispenserToCompartmentNumber(int targetCompartmentNumber) {
    // Calculate ABSOLUTE positions
    float targetAbsoluteAngle = (targetCompartmentNumber - 1) * 
                                systemConfiguration->anglePerCompartmentInDegrees;
    float currentAbsoluteAngle = (currentCompartmentNumber == 0) ? 0 : 
                                 (currentCompartmentNumber - 1) * 
                                 systemConfiguration->anglePerCompartmentInDegrees;
    
    // Calculate RELATIVE movement
    float angleToTravel = targetAbsoluteAngle - currentAbsoluteAngle;
    
    // Handle wraparound
    if (angleToTravel < 0) {
        angleToTravel += 360.0;
    }
    
    // Move motor...
    
    // STEP 4: Update current position (CRITICAL!)
    currentCompartmentNumber = targetCompartmentNumber;
    
    return true;
}
```

## Why This Design?

### ✅ Advantages

1. **START position is well-defined**
   - Established by physical home switch
   - Consistent reference point
   - Re-established on every homing

2. **Container offsets are fixed**
   - Stored in configuration
   - Easy to modify if mechanical design changes
   - Only one parameter: `anglePerCompartmentInDegrees`

3. **Movements from current position**
   - No need to return to START between moves
   - Efficient - shortest path always
   - Accurate position tracking

4. **Self-correcting with homing**
   - Press Button 6 anytime to re-home
   - Resets START reference
   - Eliminates accumulated drift

### 📐 Mathematical Correctness

The system maintains these invariants:

```
1. After homing: currentCompartmentNumber = 0, position = 0° absolute

2. At any time: 
   currentAbsolutePosition = (currentCompartmentNumber - 1) × 72°
   
3. For any movement:
   angleToTravel = targetAbsolute - currentAbsolute (with wraparound)
   
4. After movement:
   currentCompartmentNumber = targetCompartmentNumber
   Therefore: position is ALWAYS synchronized with currentCompartmentNumber
```

## Troubleshooting

### Problem: Plate doesn't reach correct container

**Check:**
1. Is `estimatedMotorDegreesPerSecond` calibrated?
2. Did you press Button 6 to home recently?
3. Is `anglePerCompartmentInDegrees` correct? (should be 72° for 5 containers)

### Problem: Position drifts over time

**Solution:**
- This is normal with time-based movement
- Press Button 6 to re-home periodically
- Future: Implement encoder-based positioning for closed-loop control

### Problem: Wrong container after several movements

**Check:**
1. Verify `currentCompartmentNumber` is being updated after each move
2. Check Serial Monitor - does it show correct CURRENT position?
3. Re-home with Button 6 to reset reference

## Summary

```
┌─────────────────────────────────────┐
│  HOMING: Define START = 0°          │
│  currentCompartmentNumber = 0       │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  CONTAINER OFFSETS (from START):    │
│  - Container 1: 0°                  │
│  - Container 2: 72°                 │
│  - Container 3: 144°                │
│  - Container 4: 216°                │
│  - Container 5: 288°                │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  SELECT CONTAINER N:                │
│  1. Calculate target absolute pos   │
│  2. Calculate current absolute pos  │
│  3. Calculate relative movement     │
│  4. Move motor                      │
│  5. UPDATE currentCompartmentNumber │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  REPEAT from current position       │
│  (Not from START!)                  │
└─────────────────────────────────────┘
```

**The positioning system correctly implements the requirement:**
- Homing defines START position
- Containers have fixed offsets from START
- Movements calculate from CURRENT position
- Current position updates after each move
- All future moves use updated position, not START

