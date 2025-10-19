# Disting NT Hardware Controls Specification

**Date:** 2025-10-19
**Source:** NT API specification (api.h:306-334)

---

## Physical Controls

### Analog Controls (6 total)

**3 Potentiometers** (continuous 0.0-1.0):
- Pot L (Left)
- Pot C (Center)
- Pot R (Right)

**2 Rotary Encoders** (continuous, infinite rotation):
- Encoder L (Left) - provides delta values (-1, 0, +1)
- Encoder R (Right) - provides delta values (-1, 0, +1)

### Button Controls (9 total)

**4 Discrete Buttons:**
- Button 1
- Button 2
- Button 3
- Button 4

**3 Pot Press Buttons** (push the pot knobs):
- Pot Button L
- Pot Button C
- Pot Button R

**2 Encoder Press Buttons** (push the encoder knobs):
- Encoder Button L
- Encoder Button R

---

## NT API Representation

From `api.h:306-334`:

```c
enum _NT_controls {
    // 4 discrete buttons
    kNT_button1 = (1 << 0),
    kNT_button2 = (1 << 1),
    kNT_button3 = (1 << 2),
    kNT_button4 = (1 << 3),

    // 3 pot press buttons
    kNT_potButtonL = (1 << 4),
    kNT_potButtonC = (1 << 5),
    kNT_potButtonR = (1 << 6),

    // 2 encoder press buttons
    kNT_encoderButtonL = (1 << 7),
    kNT_encoderButtonR = (1 << 8),

    // 2 encoder rotation events
    kNT_encoderL = (1 << 9),
    kNT_encoderR = (1 << 10),

    // 3 pot value change events
    kNT_potL = (1 << 11),
    kNT_potC = (1 << 12),
    kNT_potR = (1 << 13),
};

struct _NT_uiData {
    float pots[3];        // Pot positions [0.0-1.0]
    uint16_t controls;    // Button/control states (bitmask)
    uint16_t lastButtons; // Previous button states
    int8_t encoders[2];   // Encoder deltas (-1, 0, +1)
};
```

---

## Summary Table

| Control Type | Count | Values | Notes |
|--------------|-------|--------|-------|
| **Potentiometers** | 3 | 0.0-1.0 | Continuous analog |
| **Encoders** | 2 | -1, 0, +1 per frame | Infinite rotation, delta values |
| **Discrete Buttons** | 4 | Press/Release | Independent buttons |
| **Pot Press Buttons** | 3 | Press/Release | Push on pot knobs |
| **Encoder Press Buttons** | 2 | Press/Release | Push on encoder knobs |
| **TOTAL BUTTONS** | **9** | - | 4 + 3 + 2 |
| **TOTAL CONTROLS** | **14** | - | 3 pots + 2 encoders + 9 buttons |

---

## Common Descriptions

**Simplified (main controls):**
- "3 pots, 2 encoders, 4 buttons" ✅ (refers to primary control types)

**Complete (all controls):**
- "3 pots (0-1.0), 2 encoders (continuous rotation), 9 buttons (4 discrete + 3 pot-press + 2 encoder-press)" ✅

**Technical (API perspective):**
- "3 analog inputs (pots), 2 continuous inputs (encoders), 9 digital inputs (buttons with various functions)" ✅

---

## VCV Rack Implementation

From `EmulatorConstants.hpp`:

```cpp
enum ParamIds {
    // 3 Pots
    POT_L_PARAM, POT_C_PARAM, POT_R_PARAM,

    // 4 Buttons
    BUTTON_1_PARAM, BUTTON_2_PARAM, BUTTON_3_PARAM, BUTTON_4_PARAM,

    // 2 Encoders (continuous)
    ENCODER_L_PARAM, ENCODER_R_PARAM,

    // 2 Encoder Press
    ENCODER_L_PRESS_PARAM, ENCODER_R_PRESS_PARAM,

    NUM_PARAMS
};
```

**Note:** VCV implementation currently implements:
- 3 pots ✅
- 4 discrete buttons ✅
- 2 encoders ✅
- 2 encoder presses ✅
- Pot presses: Not explicitly shown in ParamIds (may be combined with pot params or buttons)

---

## Documentation Consistency

When referring to controls, use context-appropriate description:

**For users:**
- "3 pots, 2 encoders, 4 buttons" (main controls)

**For developers:**
- "3 pots (with press), 2 encoders (with press), 4 discrete buttons" (complete)

**For API documentation:**
- Reference _NT_controls enum with all 14 control types

---

**Authority:** NT API Specification (api.h)
**Verified:** 2025-10-19
