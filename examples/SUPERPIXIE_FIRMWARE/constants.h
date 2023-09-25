// UART objects
#define chain_left Serial2
#define chain_right Serial

#define DEBUG_MODE 0

#define RESET_PULSE_DURATION_MS (50)

#define TOUCH_PIN 13

// A list of all possible transition types
enum transitions {
  TRANSITION_INSTANT,
  TRANSITION_FADE,
  TRANSITION_FADE_OUT,
  TRANSITION_FLIP_HORIZONTAL,
  TRANSITION_FLIP_VERTICAL,
  TRANSITION_SPIN_LEFT,
  TRANSITION_SPIN_RIGHT,
  TRANSITION_SPIN_LEFT_HALF,
  TRANSITION_SPIN_RIGHT_HALF,
  TRANSITION_SHRINK,
  TRANSITION_PUSH_UP,
  TRANSITION_PUSH_DOWN,
  TRANSITION_PUSH_LEFT,
  TRANSITION_PUSH_RIGHT,

  NUM_TRANSITIONS
};

// A list of all possible gradient types
enum gradient_directions {
  GRADIENT_NONE,
  GRADIENT_HORIZONTAL,
  GRADIENT_HORIZONTAL_MIRRORED,
  GRADIENT_VERTICAL,
  GRADIENT_VERTICAL_MIRRORED,
  GRADIENT_BRIGHTNESS
};

// A list of all possible interpolation types
enum interpolation_types {
  LINEAR,
  EASE_IN,
  EASE_OUT,
  EASE_IN_SOFT,
  EASE_OUT_SOFT,
  S_CURVE,
  S_CURVE_SOFT,
};

// A list of abstract "positions"
enum positions {
  TOP,
  BOTTOM,
  LEFT,
  RIGHT,
  CENTER,
};

// CRGBF: Floating point color channels based on the FastLED Syntax with a 0.0-1.0 range
struct CRGBF {
  float r;
  float g;
  float b;
};

// vec2D: A 2-dimensional variable to store things like position, scale,
struct vec2D {
  float x;
  float y;
};

// line: Stores a start and end coordinate of a line segment in 2D space
struct line {
  float x1;
  float y1;
  float x2;
  float y2;
};

CRGBF BOOT_COLOR = { 1.00, 0.00, 0.25 };

// system_state: Everything you can configure about a SuperPixie, in one place.
struct system_state {
  float BRIGHTNESS;
  uint8_t TRANSITION_TYPE;
  uint32_t TRANSITION_DURATION_MS;
  CRGBF BACKLIGHT_COLOR;
  float BACKLIGHT_BRIGHTNESS;
  CRGBF DISPLAY_COLOR_A;
  CRGBF DISPLAY_COLOR_B;
  CRGBF DISPLAY_BACKGROUND_COLOR_A;
  CRGBF DISPLAY_BACKGROUND_COLOR_B;
  uint8_t DISPLAY_GRADIENT_TYPE;
  uint8_t DISPLAY_BACKGROUND_GRADIENT_TYPE;
  bool TOUCH_ACTIVE;
  uint16_t TOUCH_VALUE;
  CRGBF TOUCH_COLOR;
  uint8_t TRANSITION_INTERPOLATION;
  uint8_t TOUCH_GLOW_POSITION;
};

// storage: Everything you can configure about a SuperPixie, that will persist between power cycles.
struct storage {
  float TOUCH_THRESHOLD;
  float TOUCH_HIGH_LEVEL;
  float TOUCH_LOW_LEVEL;
};

// character_state: Everything you can configure about the characters being
// drawn, like position, rotation, and scaling.
struct character_state {
  char ASCII_VALUE;
  vec2D POSITION;
  vec2D SCALE;
  float ROTATION;
  float OPACITY;
};

// Default values for system_state's on boot.
system_state SYSTEM_STATE_DEFAULTS = {
  0.5,                        // BRIGHTNESS
  TRANSITION_INSTANT,         // TRANSITION_TYPE
  250,                        // TRANSITION_DURATION_MS
  { 0.0, 0.0, 0.0 },          // BACKLIGHT_COLOR
  1.0,                        // BACKLIGHT_BRIGHTNESS
  BOOT_COLOR,                 // DISPLAY_COLOR_A
  BOOT_COLOR,                 // DISPLAY_COLOR_B
  { 0.0, 0.0, 0.0 },          // DISPLAY_BACKGROUND_COLOR_A
  { 0.0, 0.0, 0.0 },          // DISPLAY_BACKGROUND_COLOR_B
  GRADIENT_NONE,              // DISPLAY_GRADIENT_TYPE
  GRADIENT_NONE,              // DISPLAY_BACKGROUND_GRADIENT_TYPE
  false,                      // TOUCH_ACTIVE
  0,                          // TOUCH_VALUE
  { 1.0000, 0.4453, 0.1562 }, // TOUCH_COLOR
  LINEAR,                     // TRANSITION_INTERPOLATION
  TOP,                        // TOUCH_GLOW_POSITION
};

// Default values for storage on boot.
storage STORAGE_DEFAULTS = {
  0.5,                        // TOUCH_THRESHOLD
  0.01,                       // TOUCH_HIGH_LEVEL
  0.01,                       // TOUCH_LOW_LEVEL  
};

// Default values for character_state's on boot.
character_state CHARACTER_STATE_DEFAULTS = {
  ' ',           // ASCII_VALUE
  { 0.0, 0.0 },  // POSITION
  { 1.0, 1.0 },  // SCALE
  0.0,           // ROTATION
  0.0,           // OPACITY
};

// This is used by "interpolate_hue()" in leds.h to simulate the FastLED HSV "Rainbow" color space
float hue_lookup[64][3] = {
  { 1.0000, 0.0000, 0.0000 },
  { 0.9608, 0.0392, 0.0000 },
  { 0.9176, 0.0824, 0.0000 },
  { 0.8745, 0.1255, 0.0000 },
  { 0.8314, 0.1686, 0.0000 },
  { 0.7922, 0.2078, 0.0000 },
  { 0.7490, 0.2510, 0.0000 },
  { 0.7059, 0.2941, 0.0000 },
  { 0.6706, 0.3333, 0.0000 },
  { 0.6706, 0.3725, 0.0000 },
  { 0.6706, 0.4157, 0.0000 },
  { 0.6706, 0.4588, 0.0000 },
  { 0.6706, 0.5020, 0.0000 },
  { 0.6706, 0.5412, 0.0000 },
  { 0.6706, 0.5843, 0.0000 },
  { 0.6706, 0.6275, 0.0000 },
  { 0.6706, 0.6667, 0.0000 },
  { 0.5882, 0.7059, 0.0000 },
  { 0.5059, 0.7490, 0.0000 },
  { 0.4196, 0.7922, 0.0000 },
  { 0.3373, 0.8353, 0.0000 },
  { 0.2549, 0.8745, 0.0000 },
  { 0.1686, 0.9176, 0.0000 },
  { 0.0863, 0.9608, 0.0000 },
  { 0.0000, 1.0000, 0.0000 },
  { 0.0000, 0.9608, 0.0392 },
  { 0.0000, 0.9176, 0.0824 },
  { 0.0000, 0.8745, 0.1255 },
  { 0.0000, 0.8314, 0.1686 },
  { 0.0000, 0.7922, 0.2078 },
  { 0.0000, 0.7490, 0.2510 },
  { 0.0000, 0.7059, 0.2941 },
  { 0.0000, 0.6706, 0.3333 },
  { 0.0000, 0.5882, 0.4157 },
  { 0.0000, 0.5059, 0.4980 },
  { 0.0000, 0.4196, 0.5843 },
  { 0.0000, 0.3373, 0.6667 },
  { 0.0000, 0.2549, 0.7490 },
  { 0.0000, 0.1686, 0.8353 },
  { 0.0000, 0.0863, 0.9176 },
  { 0.0000, 0.0000, 1.0000 },
  { 0.0392, 0.0000, 0.9608 },
  { 0.0824, 0.0000, 0.9176 },
  { 0.1255, 0.0000, 0.8745 },
  { 0.1686, 0.0000, 0.8314 },
  { 0.2078, 0.0000, 0.7922 },
  { 0.2510, 0.0000, 0.7490 },
  { 0.2941, 0.0000, 0.7059 },
  { 0.3333, 0.0000, 0.6706 },
  { 0.3725, 0.0000, 0.6314 },
  { 0.4157, 0.0000, 0.5882 },
  { 0.4588, 0.0000, 0.5451 },
  { 0.5020, 0.0000, 0.5020 },
  { 0.5412, 0.0000, 0.4627 },
  { 0.5843, 0.0000, 0.4196 },
  { 0.6275, 0.0000, 0.3765 },
  { 0.6667, 0.0000, 0.3333 },
  { 0.7059, 0.0000, 0.2941 },
  { 0.7490, 0.0000, 0.2510 },
  { 0.7922, 0.0000, 0.2078 },
  { 0.8353, 0.0000, 0.1647 },
  { 0.8745, 0.0000, 0.1255 },
  { 0.9176, 0.0000, 0.0824 },
  { 0.9608, 0.0000, 0.0392 }
};

#define PASS "PASS"
#define FAIL "FAIL ###################"

void debug(uint8_t val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(uint16_t val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(uint32_t val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(int8_t val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(int16_t val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(int32_t val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(float val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(char val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(char* val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debug(const char* val) {
  if (DEBUG_MODE == true) {
    chain_right.print(val);
  }
}

void debugln(uint8_t val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(uint16_t val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(uint32_t val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(int8_t val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(int16_t val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(int32_t val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(float val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(char val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(char* val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debugln(const char* val) {
  if (DEBUG_MODE == true) {
    chain_right.println(val);
  }
}

void debug_byte(uint8_t byte) {
  if (DEBUG_MODE == true) {
    for(uint8_t i = 0; i < 8; i++){
      debug( bitRead(byte, 7-i) );
    }
  }
}