void glitter_test() {
  static uint32_t iter = 0;
  iter++;

  clear_leds();

  if (iter % 160 == 0) {
    uint8_t x = random(0, LEDS_X);
    uint8_t y = random(0, LEDS_Y);

    CRGB temp_col = CHSV(random(0, 256), 255, 255);

    leds[x][y] = { float(temp_col.r / 255.0), float(temp_col.g / 255.0), float(temp_col.b / 255.0) };
  }
}

void line_drawing_test() {
  static float x_iter = 0.0;
  static float y_iter = 0.0;
  static float hue = 0.0;
  hue += 0.001;

  x_iter += 0.04;
  y_iter += 0.01;
  
  float x_pos = (sin(x_iter)*0.5+0.5) * (LEDS_X-1);
  float y_pos = (cos(y_iter)*0.5+0.5) * (LEDS_Y-1);

  float line_segment_width = 1.0;
  float line_segment_width_squared = line_segment_width * line_segment_width;

  clear_leds();

  for(uint8_t x = 0; x < LEDS_X; x++){
    for(uint8_t y = 0; y < LEDS_Y; y++){
      float dist_squared = shortest_distance_to_segment(x, y, x_pos, y_pos, x_pos, y_pos);
      if(dist_squared <= line_segment_width_squared){
        float brightness = line_segment_width_squared-dist_squared;
        leds[x][y] = hsv(hue, 1.0, brightness);
      }
    }
  }
}