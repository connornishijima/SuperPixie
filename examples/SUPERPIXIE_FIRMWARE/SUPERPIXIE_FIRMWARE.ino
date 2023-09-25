// #############################################################################################
//----------------------------------------------------------------------+
//   _____                    _____ _     _        _____       _        |
//  |   __|_ _ ___ ___ ___   |  _  |_|_ _|_|___   |   | |___ _| |___    |
//  |__   | | | . | -_|  _|  |   __| |_'_| | -_|  | | | | . | . | -_|   |
//  |_____|___|  _|___|_|    |__|  |_|_,_|_|___|  |_|___|___|___|___|   |
//            |_|                                                       |
//----------------------------------------------------------------------+
// #############################################################################################

// TODO: Document all function definition locations that aren't in the current file

// TODO: Learn how to use AUnit or write your own unit tests for node firmware

// TODO: Make firmware version available via COM_GET_VERSION packet
#define FIRMWARE_VERSION 10000

// Espressif dependencies
#include "freertos/FreeRTOS.h" // for uxTaskGetStackHighWaterMark()
#include "freertos/task.h"     // for xTaskGetCurrentTaskHandle()
#include "esp_task_wdt.h"
#include "esp_system.h" // for esp_get_free_heap_size()

// External dependencies
#include <FastLED.h>
#include <FS.h>
#include <LittleFS.h>

// Internal dependencies
#include "constants.h"
#include "math_utilities.h"
#include "ascii.h"
#include "dithering.h"
#include "node_fs.h"
#include "leds.h"
#include "system.h"
#include "transitions.h"
#include "vector_drawing.h"
#include "test_code.h"
#include "commands.h"
#include "uart_chain.h"
#include "debug.h"

// #############################################################################################
// Runs only once at startup
void setup() {
  //----------------------------
  // Initialize dual core system
  init_cores();
  //----------------------------
}
// #############################################################################################


// #############################################################################################
// Unused main Arduino loop()
void loop() {
  /*
  // Supervision code here
  if (millis() - last_gpu_check_in >= 100) {  // 100ms timeout
    //Serial.println("Task seems to be hung, restarting...");

    // Delete the old task if it exists
    if (gpu_task != NULL) {
      vTaskDelete(gpu_task);
      gpu_task = NULL;  // Reset task handle
    }

    //-------------------------------------
    // Initialize "GPU" core
    xTaskCreatePinnedToCore(
      loop_gpu,      // Task function
      "Loop (GPU)",  // Task name
      20000,         // Stack size
      NULL,          // Task parameter
      1,             // Priority
      &gpu_task,     // Task handle
      0              // Uses Core 0
    );
    //-------------------------------------

    // Reset the check-in time
    last_gpu_check_in = millis();
  }
  */

  yield();  // Keep system watchdog happy
}
// #############################################################################################


// #############################################################################################
// Core 0: "GPU CORE" handles vector rasterization, transitions, dithering, and LED updating
void loop_gpu(void *parameter) {
  // Register task to watchdog
  esp_task_wdt_add(NULL);

  //---------------------------
  // Initialize the LED Highway
  init_leds();
  //---------------------------

  CRGBF color = BOOT_COLOR;
  spawn_ripple(color, color, EASE_IN_SOFT, 250, -0.0, 5.0, 1.0, 15.0, 0.0, 0.0);

  //set_new_character(' ');
  //trigger_transition();

  while(system_ready == false){
    delay(10);
  }

  while (1) {
    // Watchdog check-in
    esp_task_wdt_reset();

    // -----------------------------------------------------------------
    // Note the current time in milliseconds and microcseconds this loop
    time_us_now = micros();
    time_ms_now = millis();
    // -----------------------------------------------------------------

    last_gpu_check_in = time_ms_now;

    // ----------------------------------------------
    // Run transitions and system state interpolation
    run_system_transition();
    // ----------------------------------------------

    if (freeze_led_image == false) {
      // ---------------------------
      // Reset output image to black
      clear_leds();
      // ---------------------------

      // -------------------------------------------------------------
      // Draw the background gradient to the display after clearing it
      draw_background_gradient();
      // -------------------------------------------------------------

      // ------------------------------------------------
      // Draw characters to the screen via the rasterizer
      draw_characters();
      // ------------------------------------------------

      // --------------------------
      // Update the backlight color
      draw_backlight();
      // --------------------------

      draw_debug_leds();

      //dump_last_packet_info_to_screen();

      draw_ripple();

      draw_touch();

      // ------------------------------------------------------------------------
      // Apply global brightness level
      apply_brightness();
      // ------------------------------------------------------------------------
    }

    // ------------------------------------------------------------------------
    // Apply frame blending algorithm to simulate motion blur or phosphor decay
    apply_frame_blending();
    // ------------------------------------------------------------------------

    // --------------------------------------------------------------
    // Send the updated LED image down all 7 lanes of the LED highway
    update_leds();
    // --------------------------------------------------------------

    // -----------------------
    // Measure the current FPS
    //watch_fps();
    // -----------------------

    //watch_heap();
    //watch_stack();

    yield();
  }

  // Unregister task from watchdog and delete it (this is unreachable, right?)
  esp_task_wdt_delete(NULL);
  vTaskDelete(NULL);
}


// #############################################################################################
// Core 1: "CPU CORE" handles UART chain communication, touch sensing and system configuration
void loop_cpu(void *parameter) {
  // Register task to watchdog
  //esp_task_wdt_add(NULL);

  // -------------------------------
  // Init LEDs, UARTs, memory, cores
  init_system();
  // -------------------------------

  system_ready = true;

  while (1) {
    check_reset();
    check_data();
    check_touch();
    check_transition_completion();

    run_chain_discovery();
    receive_chain_data();

    //debugln(uart0_fifo_available());
    //print_dump(250);

    yield(); // Keep watchdog timer happy 
  }
}
// #############################################################################################