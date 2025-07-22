#include <distingnt/api.h>
#include <new>
#include <cstring>

// SysEx messages starting with F0, for NT_sendMidiSysEx(..., true) which appends F7
static const uint8_t enterRemoteSysEx_data[] = {
    0xF0, 0x00, 0x21, 0x5B, 0x00, 0x01, 0x06, 0x55};
static const uint8_t exitRemoteSysEx_data[] = {
    0xF0, 0x00, 0x21, 0x5B, 0x00, 0x01, 0x00};

// SysEx for Transport STOP (F0 + OXI Header + ID 0x02 + Param 0x00 for STOP)
// NT_sendMidiSysEx will append F7
static const uint8_t transportStop_data[] = {
    0xF0, 0x00, 0x21, 0x5B, 0x00, 0x01, 0x02, 0x00};

// Full SysEx message for ACK comparison (includes F0 and F7)
static const uint8_t ackRemoteSysEx[] = {
    0xF0, 0x00, 0x21, 0x5B, 0x00, 0x01, 0x06, 0x53, 0xF7};

// Grid constants
const int GRID_COLS = 16;
const int GRID_ROWS = 8;

// Approx number of step() calls for a desired delay between animation frames.
// Assuming 48kHz sample rate. If step() is called ~12000 times/sec for small blocks,
// 600 blocks aims for ~50ms delay per animation step.
const int ANIMATION_STEP_DELAY_BLOCKS = 100; // Adjusted for faster animation (~50ms per step)

struct LedColor
{
    uint8_t r, g, b; // Changed to uint8_t for 0-255 range
};
const LedColor RAINBOW_COLORS[] = {
    {255, 0, 0},   // Red
    {255, 128, 0}, // Orange
    {255, 255, 0}, // Yellow
    {0, 255, 0},   // Green
    {0, 255, 255}, // Cyan
    {0, 0, 255},   // Blue
    {128, 0, 255}, // Purple
    {255, 0, 128}  // Pink (adjusted from a darker pink)
}; // NUM_RAINBOW_COLORS will still be calculated correctly
const int NUM_RAINBOW_COLORS = sizeof(RAINBOW_COLORS) / sizeof(LedColor);

// --- Screen & Font Configuration ---
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int SCREEN_COL_BYTES = SCREEN_HEIGHT / 8; // Should be 8 (64 pixels / 8 bits per byte)

const int FONT_CHAR_WIDTH = 5;
const int FONT_CHAR_HEIGHT = 7;
const int FONT_INTER_CHAR_SPACING = 1; // Pixels between characters

// --- Font Data (5x7 Font for ASCII 32-96) ---
// Characters from ' ' (Space, ASCII 32) to '`' (Backtick, ASCII 96)
// Each char is 5 bytes (columns). LSB of byte = top pixel. 7 pixels high.
const char FONT_FIRST_CHAR_ASCII = 32; // ' '
const char FONT_LAST_CHAR_ASCII = 96;  // '`'
const int NUM_FONT_CHARS = FONT_LAST_CHAR_ASCII - FONT_FIRST_CHAR_ASCII + 1;

static const uint8_t FONT_DATA[NUM_FONT_CHARS][FONT_CHAR_WIDTH] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32: Space
    {0x00, 0x00, 0x5f, 0x00, 0x00}, // 33: !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34: "
    {0x14, 0x7f, 0x14, 0x7f, 0x14}, // 35: #
    {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // 36: $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37: %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38: &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39: '
    {0x00, 0x1c, 0x22, 0x41, 0x00}, // 40: (
    {0x00, 0x41, 0x22, 0x1c, 0x00}, // 41: )
    {0x14, 0x08, 0x3e, 0x08, 0x14}, // 42: *
    {0x08, 0x08, 0x3e, 0x08, 0x08}, // 43: +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44: ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45: -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46: .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47: /

    {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 48: 0
    {0x00, 0x42, 0x7f, 0x40, 0x00}, // 49: 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50: 2
    {0x21, 0x41, 0x45, 0x4b, 0x31}, // 51: 3
    {0x18, 0x14, 0x12, 0x7f, 0x10}, // 52: 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53: 5
    {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 54: 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55: 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56: 8
    {0x06, 0x49, 0x49, 0x29, 0x1e}, // 57: 9

    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58: :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59: ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // 60: <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61: =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // 62: >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63: ?
    {0x32, 0x49, 0x79, 0x41, 0x3e}, // 64: @

    {0x7e, 0x11, 0x11, 0x11, 0x7e}, // 65: A
    {0x7f, 0x49, 0x49, 0x49, 0x36}, // 66: B
    {0x3e, 0x41, 0x41, 0x41, 0x22}, // 67: C
    {0x7f, 0x41, 0x41, 0x22, 0x1c}, // 68: D
    {0x7f, 0x49, 0x49, 0x49, 0x41}, // 69: E
    {0x7f, 0x09, 0x09, 0x09, 0x01}, // 70: F
    {0x3e, 0x41, 0x49, 0x49, 0x7a}, // 71: G
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, // 72: H
    {0x00, 0x41, 0x7f, 0x41, 0x00}, // 73: I
    {0x20, 0x40, 0x41, 0x3f, 0x01}, // 74: J
    {0x7f, 0x08, 0x14, 0x22, 0x41}, // 75: K
    {0x7f, 0x40, 0x40, 0x40, 0x40}, // 76: L
    {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // 77: M
    {0x7f, 0x04, 0x08, 0x10, 0x7f}, // 78: N
    {0x3e, 0x41, 0x41, 0x41, 0x3e}, // 79: O
    {0x7f, 0x09, 0x09, 0x09, 0x06}, // 80: P
    {0x3e, 0x41, 0x51, 0x21, 0x5e}, // 81: Q
    {0x7f, 0x09, 0x19, 0x29, 0x46}, // 82: R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83: S
    {0x01, 0x01, 0x7f, 0x01, 0x01}, // 84: T
    {0x3f, 0x40, 0x40, 0x40, 0x3f}, // 85: U
    {0x1f, 0x20, 0x40, 0x20, 0x1f}, // 86: V
    {0x3f, 0x40, 0x38, 0x40, 0x3f}, // 87: W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88: X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 89: Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90: Z

    {0x00, 0x7f, 0x41, 0x41, 0x00}, // 91: [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92: \\ (backslash)
    {0x00, 0x41, 0x41, 0x7f, 0x00}, // 93: ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94: ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95: _
    {0x00, 0x01, 0x02, 0x04, 0x00}  // 96: \` (backtick)
};

// Placeholder for '?' if char not found (outside ASCII 32-96)
const uint8_t FONT_UNKNOWN_CHAR_DATA[FONT_CHAR_WIDTH] = {
    0x3E, 0x01, 0x11, 0x09, 0x07 // Visual representation of a '?'
};

// --- Screen Buffer ---
static uint8_t g_screen_buffer[SCREEN_WIDTH][SCREEN_COL_BYTES];

// --- SysEx Message Buffer ---
static const uint8_t OXI_SYSEX_PREAMBLE_AND_F0[] = {0xF0, 0x00, 0x21, 0x5B, 0x00, 0x01};
const uint8_t OXI_SCREEN_MSG_ID = 0x03;
const int OXI_SCREEN_SYSEX_PAYLOAD_SIZE = SCREEN_WIDTH * SCREEN_COL_BYTES * 2;
const int OXI_SCREEN_SYSEX_TOTAL_LEN = sizeof(OXI_SYSEX_PREAMBLE_AND_F0) + 1 + OXI_SCREEN_SYSEX_PAYLOAD_SIZE;
static uint8_t g_sysex_screen_message_buffer[OXI_SCREEN_SYSEX_TOTAL_LEN];

// Function to get font data for a character
const uint8_t *get_font_char_data(char c)
{
    if (c >= FONT_FIRST_CHAR_ASCII && c <= FONT_LAST_CHAR_ASCII)
    {
        return FONT_DATA[c - FONT_FIRST_CHAR_ASCII];
    }
    return FONT_UNKNOWN_CHAR_DATA;
}

// --- Drawing Primitives ---
void plotPixel(int x, int y, bool on)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
    {
        return;
    }
    int byte_column = x;
    int byte_y_index = y / 8;
    int bit_in_byte = y % 8;

    if (on)
    {
        g_screen_buffer[byte_column][byte_y_index] |= (1 << bit_in_byte);
    }
    else
    {
        g_screen_buffer[byte_column][byte_y_index] &= ~(1 << bit_in_byte);
    }
}

void clearScreenBuffer(bool white_background)
{
    uint8_t fill_value = white_background ? 0xFF : 0x00;
    std::memset(g_screen_buffer, fill_value, sizeof(g_screen_buffer));
}

void drawChar(int x_start, int y_start, char c, bool inverted)
{
    const uint8_t *char_font_data = get_font_char_data(c);

    for (int font_col = 0; font_col < FONT_CHAR_WIDTH; ++font_col)
    {
        uint8_t column_pixels = char_font_data[font_col];
        for (int font_row_pixel = 0; font_row_pixel < FONT_CHAR_HEIGHT; ++font_row_pixel)
        {
            bool is_pixel_set_in_font = (column_pixels >> font_row_pixel) & 1;
            bool pixel_on_screen = inverted ? !is_pixel_set_in_font : is_pixel_set_in_font;
            plotPixel(x_start + font_col, y_start + font_row_pixel, pixel_on_screen);
        }
    }
}

void drawString(int x_start, int y_start, const char *str, bool inverted)
{
    int current_x = x_start;
    while (*str)
    {
        drawChar(current_x, y_start, *str, inverted);
        current_x += FONT_CHAR_WIDTH + FONT_INTER_CHAR_SPACING;
        if (current_x + FONT_CHAR_WIDTH > SCREEN_WIDTH)
            break; // Stop if next char won't fit
        str++;
    }
}

void drawRect(int x, int y, int w, int h, bool on)
{
    for (int i = 0; i < w; ++i)
    {
        for (int j = 0; j < h; ++j)
        {
            plotPixel(x + i, y + j, on);
        }
    }
}

void drawVLine(int x, int y_start, int h, bool on)
{
    for (int j = 0; j < h; ++j)
    {
        plotPixel(x, y_start + j, on);
    }
}

// --- SysEx Generation and Sending ---
void sendOxiScreenData()
{
    std::memcpy(g_sysex_screen_message_buffer, OXI_SYSEX_PREAMBLE_AND_F0, sizeof(OXI_SYSEX_PREAMBLE_AND_F0));
    int current_idx = sizeof(OXI_SYSEX_PREAMBLE_AND_F0);
    g_sysex_screen_message_buffer[current_idx++] = OXI_SCREEN_MSG_ID;

    for (int page = 0; page < SCREEN_COL_BYTES; ++page)
    {
        for (int col_x = 0; col_x < SCREEN_WIDTH; ++col_x)
        {
            uint8_t column_data_for_page = g_screen_buffer[col_x][page];
            uint8_t high_nibble = (column_data_for_page >> 4) & 0x0F;
            uint8_t low_nibble = column_data_for_page & 0x0F;

            if (current_idx < OXI_SCREEN_SYSEX_TOTAL_LEN - 1)
            { // ensure space for 2 nibbles
                g_sysex_screen_message_buffer[current_idx++] = high_nibble;
                g_sysex_screen_message_buffer[current_idx++] = low_nibble;
            }
            else
            {
                // Should not happen if math is correct, but good to be aware
                // In a real scenario, handle this error (e.g., log, assert)
                break;
            }
        }
        if (current_idx >= OXI_SCREEN_SYSEX_TOTAL_LEN - 1 && page < SCREEN_COL_BYTES - 1)
            break; // Break outer if inner broke
    }
    NT_sendMidiSysEx(kNT_destinationUSB, g_sysex_screen_message_buffer, OXI_SCREEN_SYSEX_TOTAL_LEN, true);
}

// --- Main Display Function ---
void displayCustomOxiScreen(const char *line1_text, const char *line2_text,
                            const char *line3_text, const char *line4_text,
                            const char *menu1_text, const char *menu2_text,
                            const char *menu3_text, const char *menu4_text)
{
    clearScreenBuffer(false);

    int char_render_height = FONT_CHAR_HEIGHT;
    int line_spacing = 2; // Increased line spacing for better readability
    int effective_line_height = char_render_height + line_spacing;
    int text_y_offset = 1; // Small top margin for first line

    drawString(0, text_y_offset + 0 * effective_line_height, line1_text, false);
    drawString(0, text_y_offset + 1 * effective_line_height, line2_text, false);
    drawString(0, text_y_offset + 2 * effective_line_height, line3_text, false);
    drawString(0, text_y_offset + 3 * effective_line_height, line4_text, false);

    int line5_y_start = text_y_offset + 4 * effective_line_height;
    int menu_item_width = SCREEN_WIDTH / 4;
    int menu_text_height = FONT_CHAR_HEIGHT; // In case you want to use a different font/size later

    // Background for the 5th line
    drawRect(0, line5_y_start - 1, SCREEN_WIDTH, menu_text_height + 2, true); // -1 and +2 for small padding

    int text_padding_x = 2; // Padding from left of segment and from dividers

    // Calculate max chars per menu item to prevent overflow
    // (Width - 2*padding) / (char_width + inter_char_spacing)
    int max_menu_chars = (menu_item_width - (text_padding_x * 2) - (FONT_INTER_CHAR_SPACING > 0 ? FONT_INTER_CHAR_SPACING : 0)) / (FONT_CHAR_WIDTH + FONT_INTER_CHAR_SPACING);
    if (max_menu_chars < 1)
        max_menu_chars = 1; // At least 1 char

    // Helper to truncate menu text if it's too long
    char truncated_menu_text[32]; // Adjust size as needed, max menu item text length + 1
    auto truncate = [&](const char *text_in) -> const char *
    {
        if (std::strlen(text_in) > static_cast<size_t>(max_menu_chars))
        {
            std::strncpy(truncated_menu_text, text_in, max_menu_chars);
            truncated_menu_text[max_menu_chars] = '\\0';
            return truncated_menu_text;
        }
        return text_in;
    };

    drawString(0 * menu_item_width + text_padding_x, line5_y_start, truncate(menu1_text), true);
    drawString(1 * menu_item_width + text_padding_x, line5_y_start, truncate(menu2_text), true);
    drawString(2 * menu_item_width + text_padding_x, line5_y_start, truncate(menu3_text), true);
    drawString(3 * menu_item_width + text_padding_x, line5_y_start, truncate(menu4_text), true);

    if (menu_item_width > 0)
    {
        // Adjusted divider height to match the white rectangle
        drawVLine(1 * menu_item_width - 1, line5_y_start - 1, menu_text_height + 2, false);
        drawVLine(2 * menu_item_width - 1, line5_y_start - 1, menu_text_height + 2, false);
        drawVLine(3 * menu_item_width - 1, line5_y_start - 1, menu_text_height + 2, false);
    }

    sendOxiScreenData();
}

// Plugin state structure
struct _oxiRemote : public _NT_algorithm
{
    bool enableLast;
    bool ackReceived;
    bool active;
    bool inSysEx;
    int sysExIndex;
    uint8_t sysExBuffer[16]; // Increased size just in case, OXI ACK is short

    bool triggerAnimationSequence;
    float animationSequenceCountdown_s;

    bool animationActive;
    int currentAnimationColumn;
    int framesUntilNextAnimationStep;

    bool sendEnterRemoteOnInit;
    int screenUpdateCounter; // For periodic screen updates

    _oxiRemote() {}
    ~_oxiRemote() {}
};

enum
{
    kParamEnable
};

static const _NT_parameter parameters[] = {
    {.name = "Enable",
     .min = 0,
     .max = 1,
     .def = 0,
     .unit = kNT_unitNone, // no unit
     .scaling = 0,
     .enumStrings = nullptr}};

static const uint8_t page1[] = {kParamEnable};
static const _NT_parameterPage pages[] = {
    {.name = "Remote", .numParams = 1, .params = page1}};
static const _NT_parameterPages parameterPages = {
    .numPages = 1,
    .pages = pages};

void calculateRequirements(_NT_algorithmRequirements &req, const int32_t *specifications)
{
    req.numParameters = 1;
    req.sram = sizeof(_oxiRemote);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

_NT_algorithm *construct(const _NT_algorithmMemoryPtrs &ptrs,
                         const _NT_algorithmRequirements &req,
                         const int32_t *specifications)
{
    _oxiRemote *alg = new (ptrs.sram) _oxiRemote();
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    alg->enableLast = false;
    alg->ackReceived = false;
    alg->active = false;
    alg->inSysEx = false;
    alg->sysExIndex = 0;
    std::memset(alg->sysExBuffer, 0, sizeof(alg->sysExBuffer));

    alg->triggerAnimationSequence = false;
    alg->animationSequenceCountdown_s = 0.0f;

    alg->animationActive = false;
    alg->currentAnimationColumn = 0;
    alg->framesUntilNextAnimationStep = 0;
    alg->sendEnterRemoteOnInit = false;
    alg->screenUpdateCounter = 0;

    // Initial screen display if you want it to show something on load (even if disabled)
    // displayCustomOxiScreen("OXI REMOTE OFF", "", "", "", "---", "---", "---", "---");

    return alg;
}

void sendSysExPayload(const uint8_t *sysex_message_with_F0, int length)
{
    uint32_t destMask = kNT_destinationUSB;
    // Send the message (which starts with F0); API should append F7 as end is true
    NT_sendMidiSysEx(destMask, sysex_message_with_F0, length, true);
}

// Updated to send X,Y and 7-bit encoded R,G,B pairs
static void setGridLedColor(int col, int row, uint8_t r_0_255, uint8_t g_0_255, uint8_t b_0_255)
{
    if (col < 0 || col >= GRID_ROWS || row < 0 || row >= GRID_COLS)
        return;

    // SysEx message structure based on Kotlin example:
    // F0 + OXI Preamble (00 21 5B 00 01) + MsgID (01) + X + Y + R_h + R_l + G_h + G_l + B_h + B_l
    // Total 15 bytes before F7 is appended by NT_sendMidiSysEx
    uint8_t sysex_led_data_with_F0[] = {
        0xF0, 0x00, 0x21, 0x5B, 0x00, 0x01, // OXI SysEx Preamble including F0 (6 bytes)
        0x01,                               // Message ID: GRID LED (1 byte)
        (uint8_t)col,                       // X coordinate (physical column, 0-15)
        (uint8_t)row,                       // Y coordinate (physical row, 0-7)
        (uint8_t)(r_0_255 >> 7),            // Red high 7-bit (1 byte)
        (uint8_t)(r_0_255 & 0x7F),          // Red low 7-bit  (1 byte)
        (uint8_t)(g_0_255 >> 7),            // Green high 7-bit (1 byte)
        (uint8_t)(g_0_255 & 0x7F),          // Green low 7-bit (1 byte)
        (uint8_t)(b_0_255 >> 7),            // Blue high 7-bit (1 byte)
        (uint8_t)(b_0_255 & 0x7F)           // Blue low 7-bit (1 byte)
        // NO F7 here, NT_sendMidiSysEx with end=true should add it
    };
    sendSysExPayload(sysex_led_data_with_F0, sizeof(sysex_led_data_with_F0));
}

void parameterChanged(_NT_algorithm *self_base, int p)
{
    _oxiRemote *self = (_oxiRemote *)self_base;
    bool newEnable = (self->v[kParamEnable] != 0);

    if (newEnable && !self->enableLast)
    {
        // Defer sending SysEx to the step function
        self->sendEnterRemoteOnInit = true;
        self->ackReceived = false; // Keep other state changes
        self->active = true;
        self->enableLast = true;

        self->triggerAnimationSequence = true;
        self->animationSequenceCountdown_s = 1.0f;
        self->animationActive = false;
        self->currentAnimationColumn = 0;
    }
    else if (!newEnable && self->enableLast)
    {
        sendSysExPayload(exitRemoteSysEx_data, sizeof(exitRemoteSysEx_data));
        self->ackReceived = false;
        self->active = false;
        self->enableLast = false;
        self->triggerAnimationSequence = false;
        self->animationActive = false;
    }
}

void step(_NT_algorithm *self_base, float *busFrames, int numFramesBy4)
{
    _oxiRemote *self = (_oxiRemote *)self_base;

    // Part 0: Send initial Enter Remote SysEx if flagged
    if (self->sendEnterRemoteOnInit && self->enableLast) // Check enableLast to ensure it's still desired
    {
        sendSysExPayload(enterRemoteSysEx_data, sizeof(enterRemoteSysEx_data));
        displayCustomOxiScreen("OXI REMOTE ON", "WAITING ACK...", " ", " ", "MODE", "SEQ", "ARR", "MUTE");

        self->sendEnterRemoteOnInit = false; // Clear the flag
    }

    // Part 1: Trigger for Transport Stop + Animation Start (after 1s delay)
    if (self->triggerAnimationSequence && self->active)
    {
        float dt = (float)(numFramesBy4 * 4) / NT_globals.sampleRate;
        self->animationSequenceCountdown_s -= dt;

        if (self->animationSequenceCountdown_s <= 0.0f)
        {
            self->triggerAnimationSequence = false; // Stop this timer/trigger

            // Send Transport STOP SysEx first
            sendSysExPayload(transportStop_data, sizeof(transportStop_data));

            // Then, start the LED animation
            self->animationActive = true;
            self->currentAnimationColumn = 0;
            self->framesUntilNextAnimationStep = ANIMATION_STEP_DELAY_BLOCKS;
        }
    }

    // Part 2: Animation steps
    if (self->animationActive && self->active)
    {
        self->framesUntilNextAnimationStep--;
        if (self->framesUntilNextAnimationStep <= 0)
        {
            self->framesUntilNextAnimationStep = ANIMATION_STEP_DELAY_BLOCKS;
            int current_physical_col_to_light = self->currentAnimationColumn;        // This is X (0-15)
            int current_physical_col_to_turn_off = self->currentAnimationColumn - 1; // This is X (0-15)

            if (current_physical_col_to_turn_off >= 0 && current_physical_col_to_turn_off < GRID_COLS)
            {
                for (int y_physical_row = 0; y_physical_row < GRID_ROWS; ++y_physical_row) // y_physical_row is Y (0-7)
                {
                    // OXI expects X (0-15), Y (0-7)
                    setGridLedColor(y_physical_row, current_physical_col_to_turn_off, 0, 0, 0);
                }
            }
            if (current_physical_col_to_light >= 0 && current_physical_col_to_light < GRID_COLS)
            {
                LedColor currentColor = RAINBOW_COLORS[current_physical_col_to_light % NUM_RAINBOW_COLORS];
                for (int y_physical_row = 0; y_physical_row < GRID_ROWS; ++y_physical_row) // y_physical_row is Y (0-7)
                {
                    // OXI expects X (0-15), Y (0-7)
                    setGridLedColor(y_physical_row, current_physical_col_to_light, currentColor.r, currentColor.g, currentColor.b);
                }
            }
            self->currentAnimationColumn++;
            if (self->currentAnimationColumn > GRID_COLS)
            {
                self->animationActive = false;
                // Optionally, update screen after animation finishes
                if (self->enableLast)
                {
                    if (self->ackReceived)
                    {
                        displayCustomOxiScreen("OXI REMOTE ON", "ACK RECEIVED", "ANIMATION DONE", "", "MODE", "SEQ", "ARR", "MUTE");
                    }
                    else
                    {
                        displayCustomOxiScreen("OXI REMOTE ON", "NO ACK YET", "ANIMATION DONE", "", "MODE", "SEQ", "ARR", "MUTE");
                    }
                }
            }
        }
    }

    // Example: Update screen periodically or on state changes
    // This is a simple counter, you might want more sophisticated logic
    self->screenUpdateCounter++;
    if (self->screenUpdateCounter > 20000 / numFramesBy4)
    { // Roughly every 0.5 sec at 48kHz
        self->screenUpdateCounter = 0;
        if (self->enableLast && !self->animationActive)
        { // Only update if not animating to avoid conflicts
            // This is just an example, update with relevant status
            if (self->ackReceived)
            {
                displayCustomOxiScreen("OXI REMOTE ON", "ACK RECEIVED", "IDLE", "", "MODE", "SEQ", "ARR", "MUTE");
            }
            else
            {
                displayCustomOxiScreen("OXI REMOTE ON", "WAITING ACK...", "IDLE", "", "MODE", "SEQ", "ARR", "MUTE");
            }
        }
        else if (!self->enableLast)
        {
            // displayCustomOxiScreen("OXI REMOTE OFF", "", "", "", "----", "----", "----", "----");
        }
    }
}

// Called for every incoming MIDI byte (including SysEx)
void midiRealtime(_NT_algorithm *self_base, uint8_t byte)
{
    _oxiRemote *self = (_oxiRemote *)self_base;

    if (byte == 0xF0)
    {
        self->inSysEx = true;
        self->sysExIndex = 0;
        if (self->sysExIndex < (int)sizeof(self->sysExBuffer))
        { // Check before first write
            self->sysExBuffer[self->sysExIndex++] = byte;
        }
    }
    else if (self->inSysEx)
    {
        if (self->sysExIndex < (int)sizeof(self->sysExBuffer))
        {
            self->sysExBuffer[self->sysExIndex++] = byte;
        }
        if (byte == 0xF7)
        {
            // End of SysEx: check for ACK sequence
            if (self->sysExIndex == (int)sizeof(ackRemoteSysEx))
            {
                bool match = true;
                for (int i = 0; i < self->sysExIndex; ++i)
                {
                    if (self->sysExBuffer[i] != ackRemoteSysEx[i])
                    {
                        match = false;
                        break;
                    }
                }
                if (match)
                {
                    self->ackReceived = true;
                    self->active = true;
                }
            }
            self->inSysEx = false;
            self->sysExIndex = 0; // Reset for next message
        }
        // Safety break if buffer overflows before F7
        if (self->sysExIndex >= (int)sizeof(self->sysExBuffer) && byte != 0xF7)
        {
            self->inSysEx = false;
            self->sysExIndex = 0;
        }
    }
}

bool draw(_NT_algorithm *self_base)
{
    _oxiRemote *self = (_oxiRemote *)self_base;

    // The custom screen drawing is now handled by displayCustomOxiScreen
    // So, we don't need to draw text here using NT_drawText for the Oxi status.
    // However, we can still use this for standard plugin parameter display if needed.

    // For example, to show the "Enable" parameter value:
    // NT_drawText(0, 0, self->parameters[kParamEnable].name); // Draw param name at top
    // char valStr[16];
    // snprintf(valStr, sizeof(valStr), "%s", (self->v[kParamEnable] != 0) ? "ON" : "OFF");
    // NT_drawText(60, 0, valStr);

    // Return true if you've drawn everything and want to hide default param drawing
    // Return false if you want default parameter drawing to occur (e.g. the top line param name/value)
    return false; // Let Disting draw its usual parameter line at the top
}

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('O', 'X', 'R', 'M'),
    .name = "OxiOneRemote",
    .description = "Toggle Oxi One Remote Mode (enter/exit)",
    .numSpecifications = 0,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = midiRealtime,
    .midiMessage = nullptr,
    .tags = kNT_tagUtility,
    .hasCustomUi = nullptr,
    .customUi = nullptr,
    .setupUi = nullptr,
    .serialise = nullptr,
    .deserialise = nullptr};

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data)
{
    switch (selector)
    {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return (uintptr_t)((data == 0) ? &factory : nullptr);
    }
    return 0;
}
