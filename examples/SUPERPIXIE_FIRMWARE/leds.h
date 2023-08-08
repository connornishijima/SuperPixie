#define LEDS_X 7
#define LEDS_Y 15

CRGB leds[NUM_LEDS];
CRGB leds_temp[NUM_LEDS];
CRGB leds_past[NUM_LEDS];

float mask_output[LEDS_Y][LEDS_X];
float mask_output_fx[LEDS_Y][LEDS_X];

float mask[LEDS_Y][LEDS_X];
float temp_mask[LEDS_Y][LEDS_X];
bool searched[LEDS_Y][LEDS_X];

uint16_t lookup_table[15][7] = {
  { 30, 25, 20, 15, 10, 5, 0 },
  { 31, 26, 21, 16, 11, 6, 1 },
  { 32, 27, 22, 17, 12, 7, 2 },
  { 33, 28, 23, 18, 13, 8, 3 },
  { 34, 29, 24, 19, 14, 9, 4 },

  { 30 + 35, 25 + 35, 20 + 35, 15 + 35, 10 + 35, 5 + 35, 0 + 35 },
  { 31 + 35, 26 + 35, 21 + 35, 16 + 35, 11 + 35, 6 + 35, 1 + 35 },
  { 32 + 35, 27 + 35, 22 + 35, 17 + 35, 12 + 35, 7 + 35, 2 + 35 },
  { 33 + 35, 28 + 35, 23 + 35, 18 + 35, 13 + 35, 8 + 35, 3 + 35 },
  { 34 + 35, 29 + 35, 24 + 35, 19 + 35, 14 + 35, 9 + 35, 4 + 35 },

  { 30 + 70, 25 + 70, 20 + 70, 15 + 70, 10 + 70, 5 + 70, 0 + 70 },
  { 31 + 70, 26 + 70, 21 + 70, 16 + 70, 11 + 70, 6 + 70, 1 + 70 },
  { 32 + 70, 27 + 70, 22 + 70, 17 + 70, 12 + 70, 7 + 70, 2 + 70 },
  { 33 + 70, 28 + 70, 23 + 70, 18 + 70, 13 + 70, 8 + 70, 3 + 70 },
  { 34 + 70, 29 + 70, 24 + 70, 19 + 70, 14 + 70, 9 + 70, 4 + 70 }
};

// Init LED chain
void init_leds() {
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);  // GRB ordering is assumed
}

void blur_mask(float image[LEDS_Y][LEDS_X], uint8_t blur_size) {
  uint8_t half_blur_size = blur_size / 2;
  uint16_t blur_area = (uint16_t)blur_size * blur_size;

  for (uint8_t y = 0; y < LEDS_Y; y++) {
    for (uint8_t x = 0; x < LEDS_X; x++) {
      float sum = 0.0;
      for (int8_t i = -half_blur_size; i <= half_blur_size; i++) {
        for (int8_t j = -half_blur_size; j <= half_blur_size; j++) {
          int8_t current_x = x + j;
          int8_t current_y = y + i;
          current_x = (current_x < 0) ? 0 : ((current_x >= LEDS_X) ? LEDS_X - 1 : current_x);
          current_y = (current_y < 0) ? 0 : ((current_y >= LEDS_Y) ? LEDS_Y - 1 : current_y);
          sum += image[current_y][current_x];
        }
      }
      temp_mask[y][x] = sum / blur_area;
      if (temp_mask[y][x] > 1.0) {
        temp_mask[y][x] = 1.0;
      }
    }
  }

  memcpy(image, temp_mask, LEDS_X * LEDS_Y * sizeof(float));
}

void darken_mask(float image[LEDS_Y][LEDS_X], float new_max_brightness) {
  for (uint8_t y = 0; y < LEDS_Y; y++) {
    for (uint8_t x = 0; x < LEDS_X; x++) {
      image[y][x] *= new_max_brightness;
    }
  }
}

CRGB get_color(uint8_t x, uint8_t y, float brightness){
  CRGB col_a = CONFIG.DISPLAY_COLOR_A;
  CRGB col_b = CONFIG.DISPLAY_COLOR_B;

  CRGB out_col;
  
  if(CONFIG.GRADIENT_TYPE == GRADIENT_NONE){
    out_col.r = col_a.r;
    out_col.g = col_a.g;
    out_col.b = col_a.b;
  }
  else if(CONFIG.GRADIENT_TYPE == GRADIENT_HORIZONTAL){
    float mix = x / float(LEDS_X-1);;
    out_col.r = col_a.r * (1.0-mix) + col_b.r * (mix);
    out_col.g = col_a.g * (1.0-mix) + col_b.g * (mix);
    out_col.b = col_a.b * (1.0-mix) + col_b.b * (mix);
  }
  else if(CONFIG.GRADIENT_TYPE == GRADIENT_VERTICAL){
    float mix = y / float(LEDS_Y-1);;
    out_col.r = col_a.r * (1.0-mix) + col_b.r * (mix);
    out_col.g = col_a.g * (1.0-mix) + col_b.g * (mix);
    out_col.b = col_a.b * (1.0-mix) + col_b.b * (mix);
  }
  else if(CONFIG.GRADIENT_TYPE == GRADIENT_BRIGHTNESS){
    float mix = 1.0-brightness;
    out_col.r = col_a.r * (1.0-mix) + col_b.r * (mix);
    out_col.g = col_a.g * (1.0-mix) + col_b.g * (mix);
    out_col.b = col_a.b * (1.0-mix) + col_b.b * (mix);
  }
  
  return out_col;
}

void render_mask() {
  for (uint8_t y = 0; y < LEDS_Y; y++) {
    for (uint8_t x = 0; x < LEDS_X; x++) {
      uint16_t index = lookup_table[y][x];
      //---------------------------------------------------------------
      // OUTPUT MASK
      
      float mask_val = mask_output[y][x];

      float brightness = mask_val * mask_val;
      CRGB col = get_color(x,y,brightness);
      col.r *= brightness;
      col.g *= brightness;
      col.b *= brightness;
      
      leds_temp[index] = col;

      //---------------------------------------------------------------
      // FX MASK ("ADD" blending mode)

      if(CONFIG.FX_OPACITY > 0.0){
        mask_val = mask_output_fx[y][x];
  
        brightness = mask_val * mask_val;
        CRGB fx_col;
        fx_col.r = CONFIG.FX_COLOR.r * brightness;
        fx_col.g = CONFIG.FX_COLOR.g * brightness;
        fx_col.b = CONFIG.FX_COLOR.b * brightness;
  
        leds_temp[index] += fx_col;
      }

      //---------------------------------------------------------------
      // OUTPUT
      
      if (CONFIG.FRAME_BLENDING == 0.0) {
        leds[index] = leds_temp[index];
      }
      else {
        leds[index].r = leds_past[index].r * (CONFIG.FRAME_BLENDING) + leds_temp[index].r * (1.0 - CONFIG.FRAME_BLENDING);
        leds[index].g = leds_past[index].g * (CONFIG.FRAME_BLENDING) + leds_temp[index].g * (1.0 - CONFIG.FRAME_BLENDING);
        leds[index].b = leds_past[index].b * (CONFIG.FRAME_BLENDING) + leds_temp[index].b * (1.0 - CONFIG.FRAME_BLENDING);

        leds_past[index] = leds[index];
      }
    }
  }
}

void update_leds() {
  render_mask();

  leds[NUM_LEDS - 1] = CRGB(0, 255*CONFIG.TOUCH_ACTIVE, 0);

  if (CONFIG.LOCAL_ADDRESS < 10) {
    for (uint8_t i = 0; i < CONFIG.LOCAL_ADDRESS + 1; i++) {
      //leds[(NUM_LEDS - 5) - (5 * i)] = CHSV(12, 255, 255);
    }
  }

  leds[NUM_LEDS - 31] = CRGB(255 * CONFIG.PROPAGATION, 0, 0);

  FastLED.show();
}

void clear_display() {
  memset(mask, 0, sizeof(float) * (LEDS_X * LEDS_Y));
}
