//----------------------------------------------------------------------+
//   _____                    _____ _     _        _____       _        |
//  |   __|_ _ ___ ___ ___   |  _  |_|_ _|_|___   |   | |___ _| |___    |
//  |__   | | | . | -_|  _|  |   __| |_'_| | -_|  | | | | . | . | -_|   |
//  |_____|___|  _|___|_|    |__|  |_|_,_|_|___|  |_|___|___|___|___|   |
//            |_|                                                       |
//----------------------------------------------------------------------+

// External dependencies
#include <FastLED.h>
#include <FixedPoints.h>
#include <FixedPointsCommon.h>
#include <WiFi.h>

// Internal dependencies
#include "constants.h"
#include "globals.h"
#include "utilities.h"
#include "transitions.h"
#include "commands.h"
#include "packets.h"
#include "system.h"
#include "sine.h"
#include "leds.h"
#include "ascii.h"
#include "vector_drawing.h"

void setup() {
  init_system();

  xTaskCreatePinnedToCore(
    loop_node,      // Function representing the task
    "Loop (Node)",  // Task name (optional)
    10000,          // Stack size (bytes)
    NULL,           // Task parameter (can be NULL)
    1,              // Priority (higher value means higher priority)
    NULL,           // Task handle (optional)
    0               // Core to run the task on (0 or 1)
  );

  xTaskCreatePinnedToCore(
    loop_leds,      // Function representing the task
    "Loop (LEDs)",  // Task name (optional)
    10000,          // Stack size (bytes)
    NULL,           // Task parameter (can be NULL)
    1,              // Priority (higher value means higher priority)
    NULL,           // Task handle (optional)
    1               // Core to run the task on (0 or 1)
  );
}


void loop() {
  yield();
}

void run_leds(uint32_t t_now) {
  if (transition_complete == false) {
    run_transition(t_now);

    clear_display();

    //uint8_t asterisk_polygon[42] = {0,42,0,6,0,3,0,0,2,88,0,0,0,0,0,212,0,88,255,44,2,0,255,44,0,88,0,212,2,0,0,0,0,1,0,2,0,3,0,4,0,5};
    //parse_vector_from_bytes(ascii_font, 30+30);
    debug_polygon();

    memcpy(mask_output, mask, sizeof(float) * (LEDS_X * LEDS_Y));

    if (CONFIG.FX_OPACITY > 0.0) {
      memcpy(mask_output_fx, mask, sizeof(float) * (LEDS_X * LEDS_Y));

      blur_mask(mask_output_fx, CONFIG.FX_BLUR);
      darken_mask(mask_output_fx, CONFIG.FX_OPACITY);
    }
  }

  update_leds();
  yield();
}

void loop_node(void *parameter) {
  while (1) {
    static uint32_t iter = 0;

    uint32_t t_now = micros();
    run_node(t_now);
    run_touch();

    iter++;
    if(iter == 10000){
      iter = 0;
      CONFIG.FPS = 10000000 / float(current_frame_duration_us);
      //print_lookup_table();
      //debug("FPS: ");
      //debugln(CONFIG.FPS);
    }
    
    yield();
  }
}

void loop_leds(void *parameter) {
  while (1) {
    uint32_t t_now = micros();
    run_leds(t_now);
    run_fps(t_now);
  }
}

void debug_polygon() {
  draw_character(CONFIG.CURRENT_CHARACTER, 1.0);
  draw_character(CONFIG.NEW_CHARACTER, 1.0);
}
