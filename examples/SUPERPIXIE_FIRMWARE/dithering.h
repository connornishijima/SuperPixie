// Alternating binary checkboard pattern
const uint8_t dither_pattern[2][16] = {
  {
    B01010101, B10101010, B01010101, B10101010,
    B01010101, B10101010, B01010101, B10101010,
    B01010101, B10101010, B01010101, B10101010,
    B01010101, B10101010, B01010101, B10101010
  },
  {
    B10101010, B01010101, B10101010, B01010101,
    B10101010, B01010101, B10101010, B01010101,
    B10101010, B01010101, B10101010, B01010101,
    B10101010, B01010101, B10101010, B01010101
  }
};

// Used to PWM the pixels based on if their lower 8-bits of color data are greater than these thresholds
const uint8_t dither_steps[8] = {
  0,
  32,
  64,
  96,
  128,
  160,
  192,
  224,
};

// Decides what the current dither threshold from above is used
uint8_t dither_index = 0;