// Used to signify start and end of packets
#define SYNC_PATTERN_1 0b11001100
#define SYNC_PATTERN_2 0b00110011
#define SYNC_PATTERN_3 0b10101010
#define SYNC_PATTERN_4 0b01010101

// Reserved addresses
#define ADDRESS_BROADCAST 255
#define ADDRESS_NULL      254
#define ADDRESS_COMMANDER 253

// UART objects
#define chain_left  Serial2
#define chain_right Serial

#define DEFAULT_BAUD_RATE 230400

#define ANOUNCEMENT_INTERVAL_MS 100

// LED Count
#define NUM_LEDS 70*2
#define LED_DATA_PIN 18

#define TOUCH_PIN 32

uint8_t NULL_DATA[1] = {0};

enum transitions {
  TRANSITION_INSTANT,
  TRANSITION_FADE,
  TRANSITION_FADE_OUT,
  TRANSITION_FLIP_HORIZONTAL,
  TRANSITION_FLIP_VERTICAL,
  TRANSITION_SPIN_LEFT,
  TRANSITION_SPIN_RIGHT,
  TRANSITION_SPIN_HALF_LEFT,
  TRANSITION_SPIN_HALF_RIGHT,
  TRANSITION_SHRINK,
  TRANSITION_PUSH_UP,
  TRANSITION_PUSH_DOWN,
  TRANSITION_PUSH_LEFT,
  TRANSITION_PUSH_RIGHT,

  NUM_TRANSITIONS
};

enum gradient_directions {
  GRADIENT_NONE,
  GRADIENT_HORIZONTAL,
  GRADIENT_VERTICAL,
  GRADIENT_BRIGHTNESS,
};

struct character {
  char     character;

  float    pos_x;
  float    pos_y;

  float    scale_x;
  float    scale_y;

  float    rotation;

  float    opacity;
};