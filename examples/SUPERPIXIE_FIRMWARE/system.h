//#include "esp32-hal-uart.h"
extern void init_leds();

// Set pin modes
void init_gpio() {
  pinMode(2, OUTPUT);
}

void set_new_baud_rate(uint32_t new_baud) {
  chain_left.flush();
  chain_right.flush();

  chain_left.begin(new_baud);
  chain_right.begin(new_baud);

  CONFIG.BAUD = new_baud;
}

// Set up UART communication
void init_uart() {
  chain_left.begin(DEFAULT_BAUD_RATE, SERIAL_8N1, 23, 22);
  chain_right.begin(DEFAULT_BAUD_RATE);
}

// Reset config struct to default values
void set_config_defaults() {
  character empty_char;
  empty_char.character = ' ';
  empty_char.pos_x = 0.0;
  empty_char.pos_y = 0.0;
  empty_char.scale_x = 1.0;
  empty_char.scale_y = 1.0;
  empty_char.rotation = 0.0;
  empty_char.opacity = 0.0;

  CONFIG.LOCAL_ADDRESS = ADDRESS_NULL;
  CONFIG.PROPAGATION = false;
  CONFIG.BAUD = 9600;
  CONFIG.DEBUG_LED = 0;

  CONFIG.DISPLAY_COLOR_A = CRGB(255, 0, 0);
  CONFIG.DISPLAY_COLOR_B = CRGB(255, 0, 0);

  CONFIG.GRADIENT_TYPE = GRADIENT_NONE;

  CONFIG.FX_COLOR = CRGB(0, 0, 0);
  CONFIG.FX_OPACITY = 0.0;
  CONFIG.FX_BLUR = 0.0;

  CONFIG.CHARACTER_SCALE = 1.0;

  CONFIG.TRANSITION_TYPE = TRANSITION_INSTANT;
  CONFIG.TRANSITION_DURATION_MS = 0;

  CONFIG.CURRENT_CHARACTER = empty_char;
  CONFIG.NEW_CHARACTER = empty_char;

  CONFIG.MASTER_OPACITY = 1.0;

  CONFIG.FRAME_BLENDING = 0.0;

  CONFIG.TOUCH_VAL = 255;
  CONFIG.TOUCH_THRESHOLD = 50;
  CONFIG.TOUCH_ACTIVE = false;

  CONFIG.FORCE_TRANSITION = true;

  CONFIG.CHAIN_LENGTH = 0;

  CONFIG.SCROLL_TIME_MS = 150;
  CONFIG.SCROLL_HOLD_TIME_MS = 250;

  CONFIG.READY = true;
}

// Blink debug LED
void blink() {
  digitalWrite(2, HIGH);
  delay(10);
  digitalWrite(2, LOW);
}

// Parse/propagate chain data
void run_node(uint32_t t_now) {
  static uint32_t next_announcement = 0;
  if (CONFIG.LOCAL_ADDRESS == ADDRESS_NULL) {
    if (t_now >= next_announcement) {
      next_announcement = t_now + (ANOUNCEMENT_INTERVAL_MS * 1000);
      announce_to_chain();
    }
  }

  while (chain_left.available() > 0 || chain_right.available() > 0) {
    // Data coming from chain commander
    if (chain_left.available() > 0) {
      uint8_t byte = chain_left.read();

      if (CONFIG.PROPAGATION == true && debug_mode == false) {
        chain_right.write(byte);
      }

      //debug("RX: ");
      //debugln(byte);

      feed_byte_into_sync_buffer(byte);

      // Packet started
      if (sync_buffer[0] == SYNC_PATTERN_1) {
        if (sync_buffer[1] == SYNC_PATTERN_2) {
          if (sync_buffer[2] == SYNC_PATTERN_3) {
            if (sync_buffer[3] == SYNC_PATTERN_4) {
              digitalWrite(2, HIGH);
              //debugln("GOT SYNC");
              init_packet();
            }
          }
        }
      } else if (packet_started) {
        bool got_data = false;

        feed_byte_into_packet_buffer(byte);
        packet_read_counter--;
        //debug("packet_read_counter = ");
        //debugln(packet_read_counter);

        if (packet_read_counter == 4) {  // Alter first Hop Address byte
          if (byte == ADDRESS_NULL) {
            byte = CONFIG.LOCAL_ADDRESS;
          }
        }

        if (packet_read_counter == 0) {
          if (packet_got_header == false) {
            packet_got_header = true;
            //debugln("GOT HEADER");
            packet_read_counter = byte;  // Currently reading DATA_LENGTH_IN_BYTES byte
            if (packet_read_counter == 0) {
              got_data = true;
            }
          } else if (packet_got_header == true) {
            got_data = true;
          }
        }

        if (got_data == true) {
          //debugln("GOT DATA, PARSING PACKET");
          parse_packet(t_now);
          packet_started = false;
          digitalWrite(2, LOW);
        }
      }
    }

    // -----------------------------------------------------------------
    // Data returning to chain commander
    if (chain_right.available() > 0) {
      uint8_t byte = chain_right.read();
      chain_left.write(byte);

      feed_byte_into_return_sync_buffer(byte);

      // Packet started
      if (return_sync_buffer[0] == SYNC_PATTERN_1) {
        if (return_sync_buffer[1] == SYNC_PATTERN_2) {
          if (return_sync_buffer[2] == SYNC_PATTERN_3) {
            if (return_sync_buffer[3] == SYNC_PATTERN_4) {
              //digitalWrite(2, HIGH);
              debugln("GOT RETURNING SYNC");
              init_return_packet();
            }
          }
        }
      } else if (return_packet_started) {
        bool return_got_data = false;

        feed_byte_into_return_packet_buffer(byte);
        return_packet_read_counter--;
        debug("return_packet_read_counter = ");
        debugln(return_packet_read_counter);

        if (return_packet_read_counter == 4) {  // Alter first Hop Address byte
          if (byte == ADDRESS_NULL) {
            byte = CONFIG.LOCAL_ADDRESS;
          }
        }

        if (return_packet_read_counter == 0) {
          if (return_packet_got_header == false) {
            return_packet_got_header = true;
            debugln("GOT HEADER");
            return_packet_read_counter = byte;  // Currently reading DATA_LENGTH_IN_BYTES byte
            if (return_packet_read_counter == 0) {
              return_got_data = true;
            }
          } else if (return_packet_got_header == true) {
            return_got_data = true;
          }
        }

        if (return_got_data == true) {
          debugln("GOT DATA, PARSING RETURN PACKET");
          parse_return_packet(t_now);
          return_packet_started = false;
          //digitalWrite(2, LOW);
        }
      }
    }
  }
}

void run_touch() {
  CONFIG.TOUCH_VAL = touchRead(TOUCH_PIN);

  bool touching = false;
  if (CONFIG.TOUCH_VAL <= CONFIG.TOUCH_THRESHOLD) {
    touching = true;
  }

  if (touching != CONFIG.TOUCH_ACTIVE) {
    /*
    if(touching == true){
      debugln("---------- TOUCH START");
    }
    else{
      debugln("------------ TOUCH END");
    }
    */

    CONFIG.TOUCH_ACTIVE = touching;
    send_touch_event();
  }

  //debug("TOUCH: ");
  //debugln(CONFIG.TOUCH_VAL);
}

// Bootup sequence
void init_system() {
  init_gpio();
  init_uart();
  init_leds();

  // Disable Wi-Fi
  WiFi.mode(WIFI_OFF);

  set_config_defaults();
}

void run_fps(uint32_t t_now) {
  static uint32_t last_frame_us = 0;
  current_frame_duration_us = t_now - last_frame_us;

  last_frame_us = t_now;
}