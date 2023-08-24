#include "SuperPixie.h"

SuperPixie pix;

//CRGB incandescent_lookup = CRGB(255*1.0000, 255*0.4453, 255*0.1562);
CRGB incandescent_lookup = CRGB(255 * (1.0000 * 1.0000), 255 * (0.4453 * 0.4453), 255 * (0.1562 * 0.1562));
CRGB incandescent_lookup_cold = CRGB(255 * 1.0000, 128 * 0.4453, 128 * 0.1562);

CRGB tint(CRGB light_color, CRGB tint_color) {
  CRGB output;

  output.r = uint16_t(light_color.r * tint_color.r) >> 8;
  output.g = uint16_t(light_color.g * tint_color.g) >> 8;
  output.b = uint16_t(light_color.b * tint_color.b) >> 8;

  output.r = light_color.r * 0.1 + output.r * 0.9;
  output.g = light_color.g * 0.1 + output.g * 0.9;
  output.b = light_color.b * 0.1 + output.b * 0.9;

  return output;
}

void scroll_chars() {
  static char char_set[4] = { 0, 0, 0, 0 };
  static uint32_t count = ' ';

  count += 1;
  if (count >= 127) {
    count = ' ';
  }

  for (uint8_t i = 0; i < 3; i++) {
    char_set[i] = char_set[i + 1];
  }
  char_set[2] = count;

  /*
  static uint8_t hue = 0;
  pix.set_color(CHSV(hue, 210, 255), CHSV(hue, 255, 255));
  pix.set_color(CHSV(hue, 210, 255), CHSV(hue, 255, 255));
  pix.set_color(CHSV(hue, 210, 255), CHSV(hue, 255, 255));
  hue+=4;
  */
  pix.hold();
  pix.set_string(char_set);
  pix.show();
  
  //Serial.println(char_set);
  delay(500);
}

void cycle_numbers() {
  uint16_t del_time_ms = 500;

  pix.set_string("123");
  pix.show();
  delay(del_time_ms);
  pix.set_string("234");
  pix.show();
  delay(del_time_ms);
  pix.set_string("345");
  pix.show();
  delay(del_time_ms);
  pix.set_string("456");
  pix.show();
  delay(del_time_ms);
  pix.set_string("567");
  pix.show();
  delay(del_time_ms);
  pix.set_string("678");
  pix.show();
  delay(del_time_ms);
  pix.set_string("789");
  pix.show();
  delay(del_time_ms);
  pix.set_string("890");
  pix.show();
  delay(del_time_ms);
  pix.set_string("901");
  pix.show();
  delay(del_time_ms);
  pix.set_string("012");
  pix.show();
  delay(del_time_ms);
}

void setup() {
  //Serial.begin(230400);
  pix.begin(4, 5, 9600);
  pix.set_brightness(1.0);

  pix.set_transition_type(TRANSITION_SPIN_HALF_RIGHT);
  pix.set_transition_duration_ms(200);
  pix.set_frame_blending(0.00);
  
  pix.set_color(incandescent_lookup, incandescent_lookup_cold);
  //pix.set_color(incandescent_lookup);
  //pix.set_color(CRGB(50, 255, 50), CRGB(0, 255, 0));
  //pix.set_color(CHSV(10, 255, 255));
  //pix.set_color(CRGB(255, 0, 0));

  pix.set_gradient_type(GRADIENT_BRIGHTNESS);

  pix.set_scroll_speed( 250, 50 );
}

void loop() {
  //scroll_chars();
  cycle_numbers();
  cycle_numbers();
  cycle_numbers();
  cycle_numbers();
  //Serial.println(pix.read_touch(2));
  //delay(10);
  pix.scroll_string("Hello!");
  delay(4000);
}
