extern system_state SYSTEM_STATE_INTERNAL[2];
extern system_state SYSTEM_STATE;
extern uint8_t current_system_state;
extern bool system_state_changed;
extern uint32_t time_us_now;
extern uint32_t time_ms_now;

extern TaskHandle_t cpu_task;
extern TaskHandle_t gpu_task;

// The array is only 15 LEDs tall, but column 4 has a 16th LED: the backlight!
// Because of how the LEDs are addressed, this means that every other column (1-3, 5-7)
// has a virtual 16th LED in memory that doesn't exist IRL
#define NUM_LEDS_PER_STRIP 16

// Array size, not counting the 16th row used for the backlight
#define LEDS_X 7
#define LEDS_Y 15

// The image is arranged in memory in the same shape it's arranged in real life
CRGBF leds[LEDS_X][NUM_LEDS_PER_STRIP];   // Floating point version  [7][16]
CRGB leds_8[LEDS_X][NUM_LEDS_PER_STRIP];  // Quantized 8-bit version [7][16]

CRGBF leds_blended[LEDS_X][NUM_LEDS_PER_STRIP];  // Output of frame blending
CRGBF leds_last[LEDS_X][NUM_LEDS_PER_STRIP];     // Stores the last frame, used for frame blending

CRGBF leds_debug[LEDS_X][NUM_LEDS_PER_STRIP];  // Debugging LED mask

CLEDController *controller[LEDS_X];  // One CLEDController object per lane

// These are the 7 GPIO pins of the LED highway
const uint8_t led_pins[LEDS_X] = { 12, 14, 27, 26, 25, 33, 32 };

float frame_blending_amount = 0.0;
float debug_led_opacity = 0.0;

// This is the monochrome drawing mask for the current and
// alternate vector characters, with both used during transitions
float character_mask[2][LEDS_X][LEDS_Y];

bool tx_flag_left = false;
bool tx_flag_right = false;

bool rx_flag_left = false;
bool rx_flag_right = false;

bool packet_execution_flag = false;
bool here_flag = false;

bool fade_in_complete = false;
float GLOBAL_LED_BRIGHTNESS = 0.0;

bool ripple_active = false;
uint8_t ripple_interpolation = LINEAR;

CRGBF ripple_color = { 1.0, 0.0, 0.0 };
float ripple_position = 0.0;
float ripple_width = 0.0;
float ripple_opacity = 0.0;

CRGBF ripple_color_start = { 1.0, 0.0, 0.0 };
float ripple_position_start = 0.0;
float ripple_width_start = 0.0;
float ripple_opacity_start = 0.0;

CRGBF ripple_color_destination = { 1.0, 0.0, 0.0 };
float ripple_position_destination = 0.0;
float ripple_width_destination = 0.0;
float ripple_opacity_destination = 0.0;

uint32_t ripple_start_time = 0;
uint32_t ripple_end_time = 0;

// #############################################################################################
void init_leds() {
  // -------------------------------------------------------------------------------------------
  // Open all 7 seven lanes of the LED highway, allowing us to update each column of the
  // display in parallel. By having an exceptionally short chain of WS2812B-compatible
  // LEDs (16) split across seven GPIO at once, the frame update time becomes 6.625x faster
  // than if all 106 LEDs were updated using only a single GPIO. This boost in speed gets
  // your SuperPixie running at over 850 FPS, and the LEDs even get their own CPU core. This
  // absurdly high frame rate allows for temporal dithering, a technique that tricks your eye
  // into seeing extra levels of color resolution than is normally posssible with 8-bit LEDs
  // like these, helping to preserve color resolution when the display is dimmed.

  controller[0] = &FastLED.addLeds<WS2812B, 12, GRB>(leds_8[0], NUM_LEDS_PER_STRIP);
  controller[1] = &FastLED.addLeds<WS2812B, 14, GRB>(leds_8[1], NUM_LEDS_PER_STRIP);
  controller[2] = &FastLED.addLeds<WS2812B, 27, GRB>(leds_8[2], NUM_LEDS_PER_STRIP);
  controller[3] = &FastLED.addLeds<WS2812B, 26, GRB>(leds_8[3], NUM_LEDS_PER_STRIP);
  controller[4] = &FastLED.addLeds<WS2812B, 25, GRB>(leds_8[4], NUM_LEDS_PER_STRIP);
  controller[5] = &FastLED.addLeds<WS2812B, 33, GRB>(leds_8[5], NUM_LEDS_PER_STRIP);
  controller[6] = &FastLED.addLeds<WS2812B, 32, GRB>(leds_8[6], NUM_LEDS_PER_STRIP);

  // -------------------------------------------------------------------------------------------
  // Don't worry FastLED, we'll do our own dithering

  FastLED.setBrightness(255);
  FastLED.setDither(DISABLE_DITHER);
  // -------------------------------------------------------------------------------------------

  memset(leds_8[0], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);
  memset(leds_8[1], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);
  memset(leds_8[2], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);
  memset(leds_8[3], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);
  memset(leds_8[4], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);
  memset(leds_8[5], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);
  memset(leds_8[6], 0, sizeof(CRGB) * NUM_LEDS_PER_STRIP);

  // Send black image to LEDs on boot
  controller[0]->showLeds();
  controller[1]->showLeds();
  controller[2]->showLeds();
  controller[3]->showLeds();
  controller[4]->showLeds();
  controller[5]->showLeds();
  controller[6]->showLeds();
}
// #############################################################################################

// #############################################################################################
// ---------------------------------------------------------------------------------------------
// Desaturate an input CRGBF color by a floating point amount (0.0 - 1.0)
struct CRGBF desaturate(struct CRGBF input_color, float amount) {
  float luminance = 0.2126 * input_color.r + 0.7152 * input_color.g + 0.0722 * input_color.b;
  float amount_inv = 1.0 - amount;

  struct CRGBF output;
  output.r = input_color.r * amount_inv + luminance * amount;
  output.g = input_color.g * amount_inv + luminance * amount;
  output.b = input_color.b * amount_inv + luminance * amount;

  return output;
}
// #############################################################################################


// #############################################################################################
// ---------------------------------------------------------------------------------------------
// Return a CRGBF color that represents a fully bright and saturated hue from the color wheel,
// with an input range of 0.0-1.0 instead of 0 - 360deg
struct CRGBF interpolate_hue(float hue) {
  // Scale hue to [0, 63]
  float hue_scaled = hue * 63.0f;

  // Calculate index1, avoiding expensive floor() call by using typecast to int
  int index1 = (int)hue_scaled;

  // If index1 < 0, make it 0. If index1 > 63, make it 63
  index1 = (index1 < 0 ? 0 : (index1 > 63 ? 63 : index1));

  // Calculate index2, minimizing the range to [index1, index1+1]
  int index2 = index1 + 1;

  // If index2 > 63, make it 63
  index2 = (index2 > 63 ? 63 : index2);

  // Compute interpolation factor
  float t = hue_scaled - index1;

  // Linearly interpolate
  struct CRGBF output;
  output.r = (1.0f - t) * hue_lookup[index1][0] + t * hue_lookup[index2][0];
  output.g = (1.0f - t) * hue_lookup[index1][1] + t * hue_lookup[index2][1];
  output.b = (1.0f - t) * hue_lookup[index1][2] + t * hue_lookup[index2][2];

  return output;
}
// #############################################################################################


// #############################################################################################
// ---------------------------------------------------------------------------------------------
// Return a CRGBF color that represents an input HSV color in a float range of 0.0-1.0
struct CRGBF hsv(float h, float s, float v) {
  h = fmod(h, 1.0);

  struct CRGBF col = interpolate_hue(h);
  col = desaturate(col, 1.0 - s);
  col.r *= v;
  col.g *= v;
  col.b *= v;

  return col;
}
// #############################################################################################


// #############################################################################################
// ---------------------------------------------------------------------------------------------
// Allows for blending of frames to simulate motion blur or phosphor decay
inline void apply_frame_blending() {
  float blend_val = frame_blending_amount;

  if (blend_val > 0.0) {
    // -------------------------------------------------------------------------------------------
    // Iterate over entire matrix
    for (uint8_t y = 0; y < LEDS_Y + 1; y++) {  // Extra row for backlight
      for (uint8_t x = 0; x < LEDS_X; x++) {
        float decayed_r = leds_last[x][y].r * blend_val;
        float decayed_g = leds_last[x][y].g * blend_val;
        float decayed_b = leds_last[x][y].b * blend_val;

        leds_blended[x][y].r = leds[x][y].r;
        leds_blended[x][y].g = leds[x][y].g;
        leds_blended[x][y].b = leds[x][y].b;

        if (decayed_r > leds[x][y].r) { leds_blended[x][y].r = decayed_r; }
        if (decayed_g > leds[x][y].g) { leds_blended[x][y].g = decayed_g; }
        if (decayed_b > leds[x][y].b) { leds_blended[x][y].b = decayed_b; }
      }
    }
    // -------------------------------------------------------------------------------------------
  } else {
    memcpy(leds_blended, leds, sizeof(CRGBF) * LEDS_X * NUM_LEDS_PER_STRIP);
  }

  memcpy(leds_last, leds_blended, sizeof(CRGBF) * LEDS_X * NUM_LEDS_PER_STRIP);
}
// #############################################################################################


// #############################################################################################
// Dims the input image by the global brightness level
inline void apply_brightness() {
  // -------------------------------------------------------------------------------------------
  // Iterate over entire matrix
  for (uint8_t y = 0; y < LEDS_Y; y++) {
    for (uint8_t x = 0; x < LEDS_X; x++) {
      leds[x][y].r *= SYSTEM_STATE.BRIGHTNESS * GLOBAL_LED_BRIGHTNESS;
      leds[x][y].g *= SYSTEM_STATE.BRIGHTNESS * GLOBAL_LED_BRIGHTNESS;
      leds[x][y].b *= SYSTEM_STATE.BRIGHTNESS * GLOBAL_LED_BRIGHTNESS;
    }
  }
}
// #############################################################################################


// #############################################################################################
// Run temporal dithering to quantize CRGBF values to 8-bit CRGBs, before sending the image to
// the LEDs via the LED highway pins
inline void update_leds() {
  // -------------------------------------------------------------------------------------------
  // Set up timekeeping
  static uint32_t iter = 0;
  iter++;

  // -------------------------------------------------------------------------------------------
  // Increment the dither_index, which decides how the pixels are strategically flickered
  // There are two fields in a checkerboard pattern, with two different indexes 180deg apart
  dither_index += 1;
  if (dither_index >= 8) {
    dither_index = 0;
  }
  uint8_t dither_index_b = dither_index + 4;
  if (dither_index_b >= 8) {
    dither_index_b -= 8;
  }
  // -------------------------------------------------------------------------------------------

  // -------------------------------------------------------------------------------------------
  // Iterate over entire matrix
  for (uint8_t y = 0; y < LEDS_Y + 1; y++) {  // Extra row for backlight
    for (uint8_t x = 0; x < LEDS_X; x++) {
      float gamma_r = leds_blended[x][y].r * leds_blended[x][y].r;  // Cheap gamma correction
      float gamma_g = leds_blended[x][y].g * leds_blended[x][y].g;
      float gamma_b = leds_blended[x][y].b * leds_blended[x][y].b;

      uint16_t color_r_16 = gamma_r * 65535;  // Scale from floating point to 16-bit
      uint16_t color_g_16 = gamma_g * 65535;
      uint16_t color_b_16 = gamma_b * 65535;

      uint8_t color_r_8 = color_r_16 >> 8;  // Upper 8 bits of color
      uint8_t color_g_8 = color_g_16 >> 8;
      uint8_t color_b_8 = color_b_16 >> 8;

      uint8_t color_r_dither = (color_r_16 << 8) >> 8;  // Lower 8 bits of color
      uint8_t color_g_dither = (color_g_16 << 8) >> 8;
      uint8_t color_b_dither = (color_b_16 << 8) >> 8;

      if (color_r_8 > 254) { color_r_8 = 254; }  // Leave room for added dither bit without overflow
      if (color_g_8 > 254) { color_g_8 = 254; }
      if (color_b_8 > 254) { color_b_8 = 254; }

      uint8_t dither_bit = bitRead(dither_pattern[iter % 2][y], x);  // Get the checkerboard pattern
      uint8_t dither_step_now = dither_index;
      if (dither_bit == 1) {  // Decide which of the two dither indices to use based on the pattern
        dither_step_now = dither_index_b;
      }

      // Set the dither bit according to the index vs. the lower 8 bits of the color
      uint8_t dither_bit_r = 0;
      uint8_t dither_bit_g = 0;
      uint8_t dither_bit_b = 0;
      if (color_r_dither > dither_steps[dither_step_now]) { dither_bit_r = 1; }
      if (color_g_dither > dither_steps[dither_step_now]) { dither_bit_g = 1; }
      if (color_b_dither > dither_steps[dither_step_now]) { dither_bit_b = 1; }

      // Assign quantized 8-bit data to the LED
      leds_8[x][y] = CRGB(color_r_8 + dither_bit_r, color_g_8 + dither_bit_g, color_b_8 + dither_bit_b);
    }
  }
  // -------------------------------------------------------------------------------------------

  // Send final dithered 8-bit image to LEDs
  controller[0]->showLeds();
  controller[1]->showLeds();
  controller[2]->showLeds();
  controller[3]->showLeds();
  controller[4]->showLeds();
  controller[5]->showLeds();
  controller[6]->showLeds();
}
// #############################################################################################


// #############################################################################################
// memset() the entire CRGBF "leds" matrix to 0
void clear_leds() {
  memset(leds, 0, sizeof(CRGBF) * LEDS_X * NUM_LEDS_PER_STRIP);
  memset(leds_debug, 0, sizeof(CRGBF) * LEDS_X * NUM_LEDS_PER_STRIP);
  // --------------------------------------------------------
}
// #############################################################################################


// #############################################################################################
// Mark the CPU cycles elapsed between now and the last
// call to this function, to calculate the current FPS
void watch_fps() {
  static uint8_t iter = 0;
  static uint32_t cycles_last = 0;
  uint32_t cycles_now = ESP.getCycleCount();

  uint32_t cycles_elapsed = cycles_now - cycles_last;

  iter++;
  if (iter % 10 == 0) {
    float FPS = float(F_CPU) / cycles_elapsed;
    Serial.println(FPS, 3);
  }

  cycles_last = cycles_now;
  // -------------------------------------------------------------------------------------------
}
// #############################################################################################


// #############################################################################################
// Get the free heap and occasionally print it
void watch_heap() {
  static uint8_t iter = 0;
  iter++;

  if (iter % 100 == 0) {
    uint32_t free_heap_size = esp_get_free_heap_size();
    debug("FREE HEAP: ");
    debugln(free_heap_size);
  }
}
// #############################################################################################


// #############################################################################################
// Get the free stack and occasionally print it
void watch_stack() {
  static uint8_t iter = 0;
  iter++;

  if (iter % 100 == 0) {
    //uint32_t free_heap_size = esp_get_free_heap_size();
    uint32_t free_stack_size = uxTaskGetStackHighWaterMark(gpu_task);
    debug("FREE STACK: ");
    debugln(free_stack_size);
  }
}
// #############################################################################################


CRGBF get_gradient_color(uint8_t x, uint8_t y, float draw_mask[LEDS_X][LEDS_Y]) {
  uint8_t gradient_type = SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_GRADIENT_TYPE;
  CRGBF out_color = { 0.0, 0.0, 0.0 };

  if (gradient_type == GRADIENT_NONE) {
    out_color = SYSTEM_STATE.DISPLAY_COLOR_A;
  } else if (gradient_type == GRADIENT_VERTICAL) {
    float mix = y / float(LEDS_Y - 1);
    out_color.r = SYSTEM_STATE.DISPLAY_COLOR_A.r * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.r * (1.0 - mix);
    out_color.g = SYSTEM_STATE.DISPLAY_COLOR_A.g * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.g * (1.0 - mix);
    out_color.b = SYSTEM_STATE.DISPLAY_COLOR_A.b * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.b * (1.0 - mix);
  } else if (gradient_type == GRADIENT_VERTICAL_MIRRORED) {
    float mix = saw_to_tri(y / float(LEDS_Y - 1));
    out_color.r = SYSTEM_STATE.DISPLAY_COLOR_A.r * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.r * (1.0 - mix);
    out_color.g = SYSTEM_STATE.DISPLAY_COLOR_A.g * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.g * (1.0 - mix);
    out_color.b = SYSTEM_STATE.DISPLAY_COLOR_A.b * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.b * (1.0 - mix);
  } else if (gradient_type == GRADIENT_HORIZONTAL) {
    float mix = x / float(LEDS_X - 1);
    out_color.r = SYSTEM_STATE.DISPLAY_COLOR_A.r * (1.0 - mix) + SYSTEM_STATE.DISPLAY_COLOR_B.r * (mix);
    out_color.g = SYSTEM_STATE.DISPLAY_COLOR_A.g * (1.0 - mix) + SYSTEM_STATE.DISPLAY_COLOR_B.g * (mix);
    out_color.b = SYSTEM_STATE.DISPLAY_COLOR_A.b * (1.0 - mix) + SYSTEM_STATE.DISPLAY_COLOR_B.b * (mix);
  } else if (gradient_type == GRADIENT_HORIZONTAL_MIRRORED) {
    float mix = saw_to_tri(x / float(LEDS_X - 1));
    out_color.r = SYSTEM_STATE.DISPLAY_COLOR_A.r * (1.0 - mix) + SYSTEM_STATE.DISPLAY_COLOR_B.r * (mix);
    out_color.g = SYSTEM_STATE.DISPLAY_COLOR_A.g * (1.0 - mix) + SYSTEM_STATE.DISPLAY_COLOR_B.g * (mix);
    out_color.b = SYSTEM_STATE.DISPLAY_COLOR_A.b * (1.0 - mix) + SYSTEM_STATE.DISPLAY_COLOR_B.b * (mix);
  } else if (gradient_type == GRADIENT_BRIGHTNESS) {
    float mix = draw_mask[x][y];
    out_color.r = SYSTEM_STATE.DISPLAY_COLOR_A.r * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.r * (1.0 - mix);
    out_color.g = SYSTEM_STATE.DISPLAY_COLOR_A.g * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.g * (1.0 - mix);
    out_color.b = SYSTEM_STATE.DISPLAY_COLOR_A.b * (mix) + SYSTEM_STATE.DISPLAY_COLOR_B.b * (1.0 - mix);
  }

  return out_color;
}


// #############################################################################################
// Converts a monochromatic character mask to an RGB image using solid colors or gradients
void draw_mask_to_leds(float draw_mask[LEDS_X][LEDS_Y]) {
  for (uint8_t x = 0; x < LEDS_X; x++) {
    for (uint8_t y = 0; y < LEDS_Y; y++) {
      CRGBF col_here = get_gradient_color(x, y, draw_mask);
      leds[x][y].r = add_clipped_float(leds[x][y].r, draw_mask[x][y] * col_here.r);
      leds[x][y].g = add_clipped_float(leds[x][y].g, draw_mask[x][y] * col_here.g);
      leds[x][y].b = add_clipped_float(leds[x][y].b, draw_mask[x][y] * col_here.b);
    }
  }
}
// #############################################################################################


// #############################################################################################
// Set the color of text/icons on the screen
void set_display_color(CRGBF color_a, CRGBF color_b) {
  SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_COLOR_A = color_a;
  SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_COLOR_B = color_b;
  system_state_changed = true;
}

void set_display_color(CRGBF color) {
  set_display_color(color, color);
}
// #############################################################################################


// #############################################################################################
// Set the type of gradient used for drawing characters
void set_gradient_type(uint8_t type) {
  SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_GRADIENT_TYPE = type;
  system_state_changed = true;
}


// #############################################################################################
// Set the color of the background
void set_display_background_color(CRGBF color_a, CRGBF color_b) {
  SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_BACKGROUND_COLOR_A = color_a;
  SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_BACKGROUND_COLOR_B = color_b;
  system_state_changed = true;
}

void set_display_background_color(CRGBF color) {
  set_display_background_color(color, color);
}
// #############################################################################################


// #############################################################################################
// Set the color of the backlight
void set_backlight_color(CRGBF color) {
  SYSTEM_STATE_INTERNAL[!current_system_state].BACKLIGHT_COLOR = color;
  system_state_changed = true;
}
// #############################################################################################

// #############################################################################################
// Sets the backlight brightness independently of the display
void set_backlight_brightness(float brightness_value) {
  SYSTEM_STATE_INTERNAL[!current_system_state].BACKLIGHT_BRIGHTNESS = brightness_value;
  system_state_changed = true;
}
// #############################################################################################


// #############################################################################################
// Set the brightness of the screen
void set_brightness(float brightness_value) {
  SYSTEM_STATE_INTERNAL[!current_system_state].BRIGHTNESS = brightness_value;
  system_state_changed = true;
}
// #############################################################################################


// #############################################################################################
// Update the backlight color during frame updates
void draw_backlight() {
  leds[3][15].r = SYSTEM_STATE.BACKLIGHT_COLOR.r * SYSTEM_STATE.BACKLIGHT_BRIGHTNESS * GLOBAL_LED_BRIGHTNESS;
  leds[3][15].g = SYSTEM_STATE.BACKLIGHT_COLOR.g * SYSTEM_STATE.BACKLIGHT_BRIGHTNESS * GLOBAL_LED_BRIGHTNESS;
  leds[3][15].b = SYSTEM_STATE.BACKLIGHT_COLOR.b * SYSTEM_STATE.BACKLIGHT_BRIGHTNESS * GLOBAL_LED_BRIGHTNESS;
}
// #############################################################################################


// #############################################################################################
// Draws the background color/gradient to the LED image during frame updates
void draw_background_gradient() {
  for (uint8_t y = 0; y < LEDS_Y; y++) {

    float blend_val = (y / float(LEDS_Y - 1));

    CRGBF row_color = interpolate_CRGBF(SYSTEM_STATE.DISPLAY_BACKGROUND_COLOR_A, SYSTEM_STATE.DISPLAY_BACKGROUND_COLOR_B, blend_val);

    if (y == 0 || y == LEDS_Y - 1) {
      leds[0][y].r = row_color.r * 0.5;
      leds[0][y].g = row_color.g * 0.5;
      leds[0][y].b = row_color.b * 0.5;
    } else {
      leds[0][y] = row_color;
    }

    leds[1][y] = row_color;
    leds[2][y] = row_color;
    leds[3][y] = row_color;
    leds[4][y] = row_color;
    leds[5][y] = row_color;

    if (y == 0 || y == LEDS_Y - 1) {
      leds[6][y].r = row_color.r * 0.5;
      leds[6][y].g = row_color.g * 0.5;
      leds[6][y].b = row_color.b * 0.5;
    } else {
      leds[6][y] = row_color;
    }
  }
}
// #############################################################################################


void fade_out_during_delay_ms(uint16_t del_time_ms) {
  uint32_t t_start = millis();
  uint32_t t_end = t_start + del_time_ms;

  while (millis() < t_end) {
    float progress = (millis() - t_start) / float(del_time_ms - 10);
    if (progress > 1.0) { progress = 1.0; }

    GLOBAL_LED_BRIGHTNESS = 1.0 - progress;
  }
}


void run_ripple() {
  float progress = (time_us_now - ripple_start_time) / float(ripple_end_time - ripple_start_time);
  if (progress > 1.0) {
    progress = 1.0;
  }

  if (ripple_interpolation == LINEAR) {
    // Nothing
  } else if (ripple_interpolation == EASE_IN) {
    progress *= progress;
  } else if (ripple_interpolation == EASE_OUT) {
    progress = sqrt(progress);
  } else if (ripple_interpolation == EASE_IN_SOFT) {
    progress *= progress;
    progress *= progress;
  } else if (ripple_interpolation == EASE_OUT_SOFT) {
    progress = sqrt(progress);
    progress = sqrt(progress);
  }

  ripple_color = interpolate_CRGBF(ripple_color_start, ripple_color_destination, progress);
  ripple_position = interpolate_float(ripple_position_start, ripple_position_destination, progress);
  ripple_width = interpolate_float(ripple_width_start, ripple_width_destination, progress);
  ripple_opacity = interpolate_float(ripple_opacity_start, ripple_opacity_destination, progress);
}

void draw_ripple() {
  if (ripple_active == true) {
    if (time_us_now <= ripple_end_time) {
      run_ripple();

      for (uint8_t y = 0; y < LEDS_Y; y++) {
        float y_dist = fabs(y - ripple_position);
        if (y_dist <= ripple_width) {
          float proximity = 1.0 - (y_dist / ripple_width);  // (0.0 is farthest, 1.0 is closest)

          CRGBF out_col = ripple_color;
          float brightness = (proximity * ripple_opacity);
          out_col.r *= brightness;
          out_col.g *= brightness;
          out_col.b *= brightness;

          for (uint8_t x = 0; x < LEDS_X; x++) {
            leds[x][y] = add_clipped_CRGBF(leds[x][y], out_col);
          }
        }
      }
    } else {
      ripple_active = false;  // Move is complete
    }
  }
}

void spawn_ripple(CRGBF r_color_start, CRGBF r_color_destination, uint8_t interpolation_type, uint16_t duration_ms, float r_position_start, float r_width_start, float r_opacity_start, float r_position_destination, float r_width_destination, float r_opacity_destination) {
  ripple_active = true;
  ripple_color_start = r_color_start;
  ripple_color_destination = r_color_destination;

  ripple_interpolation = interpolation_type;

  ripple_position_start = r_position_start;
  ripple_width_start = r_width_start;
  ripple_opacity_start = r_opacity_start;
  ripple_position_destination = r_position_destination;
  ripple_width_destination = r_width_destination;
  ripple_opacity_destination = r_opacity_destination;

  ripple_start_time = time_us_now;
  ripple_end_time = ripple_start_time + (duration_ms * 1000);
}

void draw_touch() {
  if (time_ms_now >= 500) {
    static float touch_strength_smooth = 0.0;
    static float touch_strength_smoother = 0.0;

    float touch_strength = clip_float(1.0 - clip_float((SYSTEM_STATE.TOUCH_VALUE - STORAGE.TOUCH_LOW_LEVEL) / (STORAGE.TOUCH_HIGH_LEVEL - STORAGE.TOUCH_LOW_LEVEL)));

    touch_strength_smooth = touch_strength * 0.05 + touch_strength_smooth * 0.95;
    touch_strength_smoother = touch_strength_smooth * 0.95 + touch_strength_smoother * 0.05;

    /*
    debug("TOUCH: ");
    debug(STORAGE.TOUCH_LOW_LEVEL);
    debug(" / ");
    debug(SYSTEM_STATE.TOUCH_VALUE);
    debug(" / ");
    debug(STORAGE.TOUCH_HIGH_LEVEL);
    debug(" \t ");
    debug(touch_strength);
    debug(" \t ");
    debug(touch_strength_smooth);
    debug(" \t ");
    debug(touch_strength_smoother);
    debug(" \t ");
    debugln(STORAGE.TOUCH_THRESHOLD);
    */

    if (touch_strength_smooth > 0.01) {
      float dimming = (0.35 + 0.65 * (1.0 - touch_strength_smoother));

      for (uint8_t y = 0; y < LEDS_Y; y++) {
        for (uint8_t x = 0; x < LEDS_X; x++) {
          leds[x][y] = desaturate(leds[x][y], touch_strength_smoother);
          leds[x][y].r *= dimming;
          leds[x][y].g *= dimming;
          leds[x][y].b *= dimming;
        }
      }

      for (uint8_t y = 0; y < LEDS_Y; y++) {
        float height = y / float(LEDS_Y - 1);
        if (SYSTEM_STATE.TOUCH_GLOW_POSITION == BOTTOM) {
          height = 1.0 - height;
        }

        height *= height;
        height *= height;
        height *= height;
        height *= height;

        for (uint8_t x = 0; x < LEDS_X; x++) {
          leds[x][y] = interpolate_CRGBF(leds[x][y], SYSTEM_STATE.TOUCH_COLOR, height * touch_strength_smoother);
        }
      }

      leds[3][15] = interpolate_CRGBF(leds[3][15], SYSTEM_STATE.TOUCH_COLOR, touch_strength_smoother);
    }
  }
}