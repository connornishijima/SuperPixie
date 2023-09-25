#include "driver/uart.h"
#include "esp32-hal-uart.h"

void dump_last_packet_info_to_screen() {
  uint8_t byte_offset = 8;
  for (uint8_t y = 0; y < LEDS_Y; y++) {
    leds_debug[0][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 6)) * 0.7F + 0.25F };
    leds_debug[1][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 5)) * 0.7F + 0.25F };
    leds_debug[2][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 4)) * 0.7F + 0.25F };
    leds_debug[3][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 3)) * 0.7F + 0.25F };
    leds_debug[4][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 2)) * 0.7F + 0.25F };
    leds_debug[5][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 1)) * 0.7F + 0.25F };
    leds_debug[6][14 - y] = { 0.0, 0.0, float(bitRead(last_packet[y + byte_offset], 0)) * 0.7F + 0.25F };
  }
}

void draw_debug_address(uint8_t address) {
  if (address == 254) {
    for (uint8_t x = 0; x < LEDS_X; x++) {
      leds_debug[x][LEDS_Y - 1] = { 1.0, 0.0, 0.0 };
    }
  } else {
    for (uint8_t x = 0; x < LEDS_X; x++) {
      if (x < address + 1) {
        leds_debug[x][LEDS_Y - 1] = { 1.000, 0.500, 0.000 };
      } else {
        leds_debug[x][LEDS_Y - 1] = { 0.250, 0.125, 0.000 };
      }
    }
  }
}

void draw_chain_length(uint8_t chain_length) {
  for (uint8_t x = 0; x < LEDS_X; x++) {
    if (x < chain_length) {
      leds_debug[x][LEDS_Y - 3] = { 0.000, 0.000, 1.000 };
    } else {
      leds_debug[x][LEDS_Y - 3] = { 0.000, 0.000, 0.250 };
    }
  }
}

void draw_debug_leds() {
  if (debug_led_opacity > 0.0) {
    draw_debug_address(CHAIN_CONFIG.LOCAL_ADDRESS);
    draw_chain_length(CHAIN_CONFIG.CHAIN_LENGTH);

    if (tx_flag_left == true) {
      tx_flag_left = false;
      leds_debug[0][0] = { 1.0, 0.5, 0.0 };
    }
    if (rx_flag_left == true) {
      rx_flag_left = false;
      leds_debug[0][1] = { 0.0, 1.0, 0.0 };
    }

    if (tx_flag_right == true) {
      tx_flag_right = false;
      leds_debug[6][0] = { 1.0, 0.5, 0.0 };
    }
    if (rx_flag_right == true) {
      rx_flag_right = false;
      leds_debug[6][1] = { 0.0, 1.0, 0.0 };
    }

    if (packet_execution_flag == true) {
      packet_execution_flag = false;
      leds_debug[2][3] = { 0.0, 1.0, 1.0 };
      leds_debug[3][3] = { 0.0, 1.0, 1.0 };
      leds_debug[4][3] = { 0.0, 1.0, 1.0 };
    }

    if (here_flag == true) {
      here_flag = false;
      for (uint8_t x = 0; x < LEDS_X; x++) {
        leds_debug[x][5] = { 1.0, 0.5, 0.0 };
      }
    }

    leds_debug[3][10] = { float(terminating_node) * 0.5F, 0.0, float(terminating_node) * 0.5F };

    leds_debug[3][0] = { 0.0, float(CHAIN_CONFIG.PROPAGATION_MODE) * 0.5F, 0.0 };
    leds_debug[3][1] = { 0.0, float(CHAIN_CONFIG.BUS_MODE) * 0.5F, 0.0 };

    for (uint8_t x = 0; x < LEDS_X; x++) {
      for (uint8_t y = 0; y < LEDS_Y; y++) {
        leds[x][y].r = leds[x][y].r * (1.0 - debug_led_opacity) + leds_debug[x][y].r * debug_led_opacity;
        leds[x][y].g = leds[x][y].g * (1.0 - debug_led_opacity) + leds_debug[x][y].g * debug_led_opacity;
        leds[x][y].b = leds[x][y].b * (1.0 - debug_led_opacity) + leds_debug[x][y].b * debug_led_opacity;
      }
    }
  }
}


void print_dump(uint32_t interval_ms) {
  static uint32_t t_last = time_ms_now;

  if ((time_ms_now - t_last) >= interval_ms) {
    t_last = time_ms_now;

    Serial.print(CHAIN_CONFIG.LOCAL_ADDRESS);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");
    Serial.print(CHAIN_CONFIG.CHAIN_LENGTH);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");
    Serial.print(CHAIN_CONFIG.PROPAGATION_MODE);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");
    Serial.print(CHAIN_CONFIG.BUS_MODE);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(last_probe_tx_time_ms);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(probe_timeout_ms);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(probe_timeout_occurred);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(probe_packet_received);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(terminating_node);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(propagation_queued);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(assignment_complete);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(discovery_complete);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(rx_drop_start);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(rx_drop_duration);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(rx_low);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(show_called_once);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(upstream_packets_receieved);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(time_ms_now);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(time_us_now);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(esp_get_free_heap_size());
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(uxTaskGetStackHighWaterMark(cpu_task));
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(uxTaskGetStackHighWaterMark(gpu_task));
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(frame_blending_amount);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(debug_led_opacity);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(fade_in_complete);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(GLOBAL_LED_BRIGHTNESS);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(freeze_led_image);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(transition_complete_flag);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(system_state_changed);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(current_system_state);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(system_state_transition_progress);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(system_state_transition_progress_shaped);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(transition_start_ms);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(transition_end_ms);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(transition_running);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(last_gpu_check_in);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(system_ready);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(SYSTEM_STATE.BRIGHTNESS);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(SYSTEM_STATE.TRANSITION_TYPE);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(SYSTEM_STATE.TRANSITION_DURATION_MS);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(SYSTEM_STATE.TOUCH_ACTIVE);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(SYSTEM_STATE.TOUCH_VALUE);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(STORAGE.TOUCH_THRESHOLD);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(STORAGE.TOUCH_HIGH_LEVEL);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(STORAGE.TOUCH_LOW_LEVEL);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(character_state_changed);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(current_character_state);
    Serial.print("\t\t\t\t\t\t\t\t\t\t");

    Serial.print(dither_index);
    //Serial.print(" \t ");
    

    Serial.println();
  }
}

// Returns the number of bytes available in UART0's hardware FIFO queue
uint8_t uart0_fifo_available() {
  // Hardcoded address for UART0 status register
  volatile uint32_t* uart_status_reg = (uint32_t*) 0x3FF4001C;

  // Read the register
  uint32_t reg_val = *uart_status_reg;

  // Extract the Tx FIFO count from bits 16 to 23
  uint8_t fifo_count = (reg_val >> 16) & 0xFF;

  // Calculate and return the number of available bytes in FIFO
  return 128 - fifo_count;
}