#include "SuperPixie.h"

SuperPixie pix;

void setup() {
  pix.begin(230400);
  pix.set_brightness(0.5);
  pix.set_transition_type(TRANSITION_PUSH_LEFT);
  pix.set_transition_duration_ms(250);
  pix.set_frame_blending(0.75);
  pix.set_color(CRGB(50, 255, 50), CRGB(0, 255, 0));
  pix.set_gradient_type(GRADIENT_BRIGHTNESS);
}

void loop() {
  static char char_set[4] = {0, 0, 0, 0};
  static uint32_t count = ' ';

  count += 1;
  if (count >= 127) {
    count = ' ';
  }

  for (uint8_t i = 0; i < 3; i++) {
    char_set[i] = char_set[i + 1];
  }
  char_set[2] = count;

  pix.set_string(char_set);
  pix.show();
  delay(500);
}
