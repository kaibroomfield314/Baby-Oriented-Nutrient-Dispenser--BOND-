# Positioning System - How It Works

## Core Concept

The system uses **absolute step-based positioning** with a **START reference point** established during homing. Position is tracked in **steps** (not time), providing precise and repeatable movement.

## The Three Key Components

### 1. START Position (Home Reference)
```
When homing completes and home switch is pressed:
  → System defines this as START position = 0 steps (0° absolute)
  → Variable: currentPositionSteps = 0
  → Variable: currentCompartmentNumber = 0
  → All container positions are measured from this START point
```

### 2. Container Positions (Fixed, Stored in Configuration)
```cpp
// In ConfigurationSettings.h
float containerPositionsInDegrees[5] = {
    0.0,      // Container 1: at home/START (0°)
    72.0,     // Container 2: 72° from START (evenly spaced)
    144.0,    // Container 3: 144° from START
    216.0,    // Container 4: 216° from START
    288.0     // Container 5: 288° from START
};

// These are converted to absolute step positions at initialization:
Container 1: 0 steps (0°)
Container 2: calculated steps for 72°
Container 3: calculated steps for 144°
Container 4: calculated steps for 216°
Container 5: calculated steps for 288°

// Step calculation:
// steps = (angle / 360.0) × stepsPerRevolution × microstepping × gearRatio
```

### 3. Current Position Tracking (Step-Based)
```
After every movement:
  → System updates currentPositionSteps (absolute position in steps)
  → System updates currentCompartmentNumber
  → Position is tracked in STEPS, not time
  → This becomes the reference for NEXT movement
  → Movements are NOT always calculated from START
  → They are calculated from CURRENT position in steps
```

## Movement Algorithm (Step-Based)

### Step-by-Step Process

```
1. User selects Container N (e.g., N=5)

2. Get TARGET absolute step position:
   targetStepPosition = compartmentStepPositions[N - 1]
   Example: Container 5 → pre-calculated step position for 288°

3. Get CURRENT absolute step position:
   currentStepPosition = currentPositionSteps
   Example: At Container 3 → currentPositionSteps (e.g., 800 steps)

4. Calculate RELATIVE movement needed:
   stepsToMove = targetStepPosition - currentStepPosition
   Example: 1600 steps - 800 steps = 800 steps forward

5. Handle wraparound (choose shortest path):
   If stepsToMove > (totalStepsPerRevolution / 2):
       stepsToMove -= totalStepsPerRevolution  // Go backward instead
   Example: If forward is 1400 steps, but backward is 200 steps,
           → choose backward (shorter path)

6. Move exact number of steps:
   - If stepsToMove > 0: moveForwardBySteps(stepsToMove)
   - If stepsToMove < 0: moveBackwardBySteps(abs(stepsToMove))

7. **UPDATE CURRENT POSITION** (crucial!):
   currentPositionSteps += stepsMoved
   currentCompartmentNumber = targetCompartmentNumber
   
   This ensures next movement calculates from THIS position!
```

## Real-World Example Sequence

### Sequence 1: From Home to Container 5
```
Initial State:
  currentPositionSteps = 0 (at START)
  currentCompartmentNumber = 0
  
User selects Container 5:
  targetStepPosition = compartmentStepPositions[4] = 1600 steps (288°)
  currentStepPosition = 0 steps
  stepsToMove = 1600 - 0 = 1600 steps forward
  
After Movement:
  currentPositionSteps = 1600 steps
  currentCompartmentNumber = 5
```

### Sequence 2: From Container 5 to Container 3
```
Initial State:
  currentPositionSteps = 1600 steps (at Container 5)
  currentCompartmentNumber = 5
  
User selects Container 3:
  targetStepPosition = compartmentStepPositions[2] = 800 steps (144°)
  currentStepPosition = 1600 steps
  stepsToMove = 800 - 1600 = -800 steps
  
  Since negative, move backward:
  moveBackwardBySteps(800)
  
After Movement:
  currentPositionSteps = 800 steps
  currentCompartmentNumber = 3
```

### Sequence 3: From Container 3 to Container 4
```
Initial State:
  currentPositionSteps = 800 steps (at Container 3)
  currentCompartmentNumber = 3
  
User selects Container 4:
  targetStepPosition = compartmentStepPositions[3] = 1200 steps (216°)
  currentStepPosition = 800 steps
  stepsToMove = 1200 - 800 = 400 steps forward
  
After Movement:
  currentPositionSteps = 1200 steps
  currentCompartmentNumber = 4
```

## Serial Monitor Output Example

```
========================================
[Positioning] EXACT STEP-BASED POSITIONING
========================================
[Positioning] CURRENT: 0 steps (0°) - HOME
[Positioning] TARGET: 1600 steps (288°) - Compartment 5
[Positioning] MOVEMENT: 1600 steps FORWARD (288°)
[Positioning] Step delay: 10000 μs
[Positioning] Moving to exact step position...
[Stepper DEBUG] STARTING FORWARD MOVEMENT BY STEPS
[Stepper DEBUG] Movement: 1600 steps forward, delay 10000 μs
[Stepper DEBUG] Progress: 50% (800/1600 steps)
[Stepper DEBUG] Progress: 100% (1600/1600 steps)
[Stepper DEBUG] Completed 1600 steps forward in 16234 ms
[Positioning] Motor movement complete
[Positioning] Settling for 200ms...
POSITIONED: Now at Container 5 (1600 steps, 288°)
========================================

========================================
[Positioning] EXACT STEP-BASED POSITIONING
========================================
[Positioning] CURRENT: 1600 steps (288°) - Compartment 5
[Positioning] TARGET: 800 steps (144°) - Compartment 3
[Positioning] MOVEMENT: 800 steps BACKWARD (144°)
[Positioning] Step delay: 10000 μs
[Positioning] Moving to exact step position...
[Stepper DEBUG] STARTING BACKWARD MOVEMENT BY STEPS
[Stepper DEBUG] Movement: 800 steps backward, delay 10000 μs
[Stepper DEBUG] Completed 800 steps backward in 8112 ms
[Positioning] Motor movement complete
POSITIONED: Now at Container 3 (800 steps, 144°)
========================================
```

## Code Implementation

### In DispenserController.h

```cpp
// STEP 1: Define START position during homing
bool performHomingWithRetryAndEscalation() {
    // ... homing process ...
    // When switch activated:
    resetPositionToHome();  // Sets currentPositionSteps = 0
    currentCompartmentNumber = 0;
    isSystemHomedAndReady = true;
    return true;
}

// STEP 2: Calculate compartment step positions at initialization
void calculateCompartmentStepPositions() {
    for (int i = 0; i < numberOfCompartments; i++) {
        float angle = containerPositionsInDegrees[i];
        compartmentStepPositions[i] = 
            hardwareController->calculateStepsForAngle(angle);
    }
}

// STEP 3: Move to container using exact step positioning
bool moveRotaryDispenserToCompartmentNumber(int targetCompartmentNumber) {
    // Get absolute step positions
    long targetStepPosition = compartmentStepPositions[targetCompartmentNumber - 1];
    long currentStepPosition = currentPositionSteps;
    
    // Calculate relative movement
    long stepsToMove = targetStepPosition - currentStepPosition;
    
    // Handle wraparound (shortest path)
    float totalStepsPerRevolution = stepsPerRevolution * microstepping * gearRatio;
    if (abs(stepsToMove) > (totalStepsPerRevolution / 2)) {
        if (stepsToMove > 0) {
            stepsToMove -= totalStepsPerRevolution;  // Go backward
        } else {
            stepsToMove += totalStepsPerRevolution;  // Go forward
        }
    }
    
    // Move exact number of steps
    long stepsMoved = 0;
    if (stepsToMove > 0) {
        stepsMoved = hardwareController->moveStepperForwardBySteps(
            stepsToMove, stepDelay);
    } else {
        stepsMoved = hardwareController->moveStepperBackwardBySteps(
            abs(stepsToMove), stepDelay);
    }
    
    // STEP 4: Update current position (CRITICAL!)
    updatePositionAfterMovement(stepsMoved);
    currentCompartmentNumber = targetCompartmentNumber;
    
    return true;
}
```

## Why Step-Based Positioning?

### ✅ Advantages Over Time-Based

1. **Precise positioning**
   - Exact step count = exact position
   - No accumulation of timing errors
   - Repeatable movement

2. **Position memory**
   - System always knows exact position in steps
   - Position survives power cycles (if homed first)
   - No drift over time

3. **Speed-independent accuracy**
   - Position accuracy doesn't depend on motor speed
   - Can change speed without affecting position
   - Only step count matters for position

4. **Shortest path calculation**
   - Automatically chooses forward or backward
   - Wraparound handling ensures efficient movement
   - Always moves shortest distance

5. **Homing-based reference**
   - Physical switch provides absolute reference
   - Re-home anytime to reset position
   - Eliminates accumulated position errors

### 📐 Mathematical Correctness

The system maintains these invariants:

```
1. After homing: 
   currentPositionSteps = 0, 
   currentCompartmentNumber = 0

2. At any time: 
   currentPositionSteps = absolute steps from home
   currentDegrees = (currentPositionSteps / totalStepsPerRevolution) × 360°

3. For any movement:
   stepsToMove = targetStepPosition - currentStepPosition
   (with shortest path wraparound)

4. After movement:
   currentPositionSteps += stepsMoved
   currentCompartmentNumber = targetCompartmentNumber
   Therefore: position is ALWAYS synchronized
```

## Position Tracking Details

### Position Variables

```cpp
long currentPositionSteps;              // Absolute position in steps (0 = home)
long compartmentStepPositions[5];       // Pre-calculated step positions for each compartment
```

### Position Conversion

```cpp
// Steps to degrees:
float degrees = (currentPositionSteps / totalStepsPerRevolution) × 360.0

// Degrees to steps:
long steps = (degrees / 360.0) × totalStepsPerRevolution
```

### Position Updates

```cpp
// After forward movement:
currentPositionSteps += stepsMoved;  // Positive steps

// After backward movement:
currentPositionSteps += stepsMoved;  // Negative steps (stepsMoved is negative)

// After homing:
currentPositionSteps = 0;  // Reset to home
```

## Troubleshooting

### Problem: Plate doesn't reach correct container

**Check:**
1. Is `stepperStepsPerRevolution` correct? (200 for 1.8° motor)
2. Is `stepperMicrostepping` correct? (check driver settings)
3. Is `stepperGearRatio` correct? (1.0 if no gear reduction)
4. Did you press Button 6 to home recently?
5. Are `containerPositionsInDegrees[]` values correct?

### Problem: Position drifts over time

**Solution:**
- Step-based positioning eliminates drift!
- If position seems wrong, re-home with Button 6
- Position is tracked in steps, not time - no accumulation of errors

### Problem: Wrong container after several movements

**Check:**
1. Verify `currentPositionSteps` is being updated after each move
2. Check Serial Monitor - does it show correct CURRENT position in steps?
3. Run calibration (Button 6 long press) to verify timing
4. Re-home with Button 6 to reset position reference

### Problem: Motor misses steps

**Solution:**
- Increase `stepperMinStepDelayMicroseconds` (motor going too fast)
- Check motor/driver connections
- Reduce mechanical load
- Verify step delay is within safe limits

## Calibration Function

The system includes a full rotation timing calibration:

1. **Press and hold Button 6 for 3+ seconds**
2. System will:
   - Home to switch
   - Rotate 360° forward
   - Measure time until switch activates again
3. Results displayed in Serial Monitor:
   - Full rotation time
   - Time per degree
   - Calculated time to each compartment

This helps verify that step-based positioning matches actual mechanical timing.

## Summary

```
┌─────────────────────────────────────┐
│  HOMING: Define START = 0 steps     │
│  currentPositionSteps = 0           │
│  currentCompartmentNumber = 0       │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  CALCULATE COMPARTMENT POSITIONS:   │
│  Convert degrees to steps            │
│  Store in compartmentStepPositions[] │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  SELECT CONTAINER N:                │
│  1. Get target step position        │
│  2. Get current step position       │
│  3. Calculate steps to move         │
│  4. Choose shortest path            │
│  5. Move exact number of steps      │
│  6. UPDATE currentPositionSteps     │
└──────────┬──────────────────────────┘
           │
           ▼
┌─────────────────────────────────────┐
│  REPEAT from current step position  │
│  (Not from START!)                   │
│  Position tracked in STEPS          │
└─────────────────────────────────────┘
```

**The positioning system correctly implements step-based positioning:**
- Homing defines START position (0 steps)
- Containers have fixed step positions (calculated from degrees)
- Movements use exact step counts (not time-based)
- Position tracked in steps (precise, no drift)
- Current position updates after each move
- All future moves use updated position, not START
