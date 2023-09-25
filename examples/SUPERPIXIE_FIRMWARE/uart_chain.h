#include <driver/uart.h>
#include "soc/rtc_cntl_reg.h"
#include "esp_system.h"     // For esp_restart

#define DEFAULT_CHAIN_BAUD (9600)

#define SERIAL_0_RX_GPIO (3)
#define SERIAL_0_TX_GPIO (1)

#define SERIAL_2_RX_GPIO (18)
#define SERIAL_2_TX_GPIO (23)

#define PREAMBLE_PATTERN_1 (B10111000)
#define PREAMBLE_PATTERN_2 (B10000111)
#define PREAMBLE_PATTERN_3 (B10101010)
#define PREAMBLE_PATTERN_4 (B10010101)

// Reserved addresses
#define ADDRESS_CHAIN_HEAD (0)
#define ADDRESS_COMMANDER (253)
#define ADDRESS_NULL (254)
#define ADDRESS_BROADCAST (255)

#define DISCOVERY_PROBE_INTERVAL_MS 20

#define UPSTREAM (1)
#define DOWNSTREAM (0)

uint8_t NULL_DATA[1] = { 0 };

// character_state: Everything you can configure about the characters being
// drawn, like position, rotation, and scaling.
struct chain_config {
  uint8_t LOCAL_ADDRESS;
  uint8_t CHAIN_LENGTH;
  bool PROPAGATION_MODE;
  bool BUS_MODE;
};

chain_config CHAIN_CONFIG;
uint32_t last_probe_tx_time_ms = 0;
uint32_t probe_timeout_ms = 0;
bool probe_timeout_occurred = false;
bool probe_packet_received = false;
bool terminating_node = false;
bool propagation_queued = false;

// Holds incoming data
uint8_t sync_buffer[2][4];
uint8_t packet_buffer[2][128];
uint8_t packet_data[2][64];
uint8_t packet_buffer_index[2] = { 0, 0 };
bool packet_started[2] = { false, false };

bool assignment_complete = false;
bool discovery_complete = false;

uint8_t last_packet[128];

uint32_t rx_drop_start = 0;
int32_t rx_drop_duration = 0;
bool rx_low = false;

bool show_called_once = false;
uint32_t upstream_packets_receieved = 0;

inline uint8_t byte_to_padded_nibble(uint8_t b, uint8_t nibble_half) {
  uint8_t input = b;

  if (nibble_half == HIGH) {
    input = input >> 4;  // Wipe out bottom half
    return input;
  } else if (nibble_half == LOW) {
    input = input << 4;  // Wipe out top half
    input = input >> 4;
    return input;
  }

  // Normally never reached
  return 0;
}

inline uint8_t get_byte_from_16_bit(uint16_t input, uint8_t byte_half) {
  uint8_t input_high = uint16_t(input << 8) >> 8;
  uint8_t input_low = uint16_t(input >> 8);

  if (byte_half == HIGH) {
    return input_high;
  } else if (byte_half == LOW) {
    return input_low;
  }

  // Normally never reached
  return 0;
}

// Check to see if data has not arrived in an expected amount of time,
// showing a '?' if it hasn't (Commander not commanding this node properly)
void check_data() {
  if(time_ms_now >= 3000){
    if(show_called_once == false){
      CRGBF error_color = {0.0, 1.0, 0.0};

      if(CHAIN_CONFIG.LOCAL_ADDRESS == ADDRESS_NULL){
        error_color.r = 1.00;
        error_color.g = 0.40;
        error_color.b = 0.00;
      }
      else{ // Connected properly, just never being updated
        error_color.r = 1.00;
        error_color.g = 0.00;
        error_color.b = 0.50;
      }

      set_display_color( error_color );
      set_backlight_color( error_color );

      frame_blending_amount = 0.0;
      set_transition_type( TRANSITION_FADE );
      set_transition_time_ms( 250 );
      set_new_character( '?' );
      trigger_transition(); // show();

      show_called_once = true;
    }
  }
}

// Check to see if the left-hand RX line has been pulled LOW
// for more than RESET_PULSE_DURATION_MS (constants.h) and
// resetting the node to its default state
void check_reset() {
  // Read the RX GPIO
  uint8_t state_now = digitalRead(SERIAL_2_RX_GPIO);

  // Voltage falling now, mark the time
  if (state_now == LOW && rx_low == false) {
    rx_drop_start = time_ms_now;
    rx_low = true;
  }

  // Voltage rising now
  else if (state_now == HIGH && rx_low == true) {
    rx_low = false;
  }

  // If RX is LOW:
  if (rx_low == true) {
    // Measure time since voltage fell
    rx_drop_duration = (int32_t)(time_ms_now - rx_drop_start);

    // if >= half second since boot
    if (time_ms_now >= 500) {
      if (rx_drop_duration >= RESET_PULSE_DURATION_MS-10) { // within 10ms
        //if(CHAIN_CONFIG.BUS_MODE == false){
          // Propagate reset pulse
          pinMode(SERIAL_0_TX_GPIO, OUTPUT);
          digitalWrite(SERIAL_0_TX_GPIO, LOW);
          fade_out_during_delay_ms(RESET_PULSE_DURATION_MS);
          digitalWrite(SERIAL_0_TX_GPIO, HIGH);
        //}

        //ESP.restart();
        // RESET AS FAST AS POSSIBLE VIA REGISTER WRITE, this also helps to not corrupt the LED image
        REG_WRITE(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_SYS_RST);
      }
    }
  }
}

void enable_bus_mode(){
  //chain_right.end();

  pinMode(SERIAL_2_RX_GPIO, INPUT);
  pinMode(SERIAL_0_TX_GPIO, OUTPUT);

  gpio_matrix_in(SERIAL_2_RX_GPIO, SIG_IN_FUNC224_IDX, false);          // chain_left RX pin
                                                                        // connected directly to...
  gpio_matrix_out(SERIAL_0_TX_GPIO, SIG_IN_FUNC224_IDX, false, false);  // chain_right TX pin
                                                                        // for instant propagation
                                                                        // of data down the chain
  CHAIN_CONFIG.BUS_MODE = true;
}

void disable_bus_mode() {
  gpio_matrix_in(SERIAL_2_RX_GPIO, 0x100, false);          // Restore GPIO 23 to its original function
  gpio_matrix_out(SERIAL_0_TX_GPIO, 0x100, false, false);  // Restore GPIO 1 to its original function

  //chain_right.begin(DEFAULT_CHAIN_BAUD, SERIAL_8N1, SERIAL_0_RX_GPIO, SERIAL_0_TX_GPIO);

  CHAIN_CONFIG.BUS_MODE = false;
}

void send_packet(uint8_t direction, uint8_t command_type, uint8_t destination_address, uint8_t data_length_in_bytes, uint8_t* command_data) {
  uint8_t packet_temp[128] = { 0 };
  uint8_t origin_address = CHAIN_CONFIG.LOCAL_ADDRESS;
  uint16_t packet_id = random(0, 65535);

  if(direction == UPSTREAM){
    tx_flag_left = true;
  }
  else if(direction == DOWNSTREAM){
    tx_flag_right = true;
  }

  // Packet header
  packet_temp[0] = PREAMBLE_PATTERN_1;
  packet_temp[1] = PREAMBLE_PATTERN_2;
  packet_temp[2] = PREAMBLE_PATTERN_3;
  packet_temp[3] = PREAMBLE_PATTERN_4;

  packet_temp[4] = byte_to_padded_nibble(destination_address, HIGH);
  packet_temp[5] = byte_to_padded_nibble(destination_address, LOW);

  packet_temp[6] = byte_to_padded_nibble(origin_address, HIGH);
  packet_temp[7] = byte_to_padded_nibble(origin_address, LOW);

  packet_temp[8] = byte_to_padded_nibble(get_byte_from_16_bit(packet_id, HIGH), HIGH);
  packet_temp[9] = byte_to_padded_nibble(get_byte_from_16_bit(packet_id, HIGH), LOW);
  packet_temp[10] = byte_to_padded_nibble(get_byte_from_16_bit(packet_id, LOW), HIGH);
  packet_temp[11] = byte_to_padded_nibble(get_byte_from_16_bit(packet_id, LOW), LOW);

  packet_temp[12] = byte_to_padded_nibble(command_type, HIGH);
  packet_temp[13] = byte_to_padded_nibble(command_type, LOW);

  packet_temp[14] = byte_to_padded_nibble(data_length_in_bytes, HIGH);
  packet_temp[15] = byte_to_padded_nibble(data_length_in_bytes, LOW);

  uint8_t total_packet_bytes = 16;  // So far

  for (uint8_t i = 0; i < data_length_in_bytes; i++) {
    packet_temp[total_packet_bytes + 0] = byte_to_padded_nibble(command_data[i], HIGH);
    packet_temp[total_packet_bytes + 1] = byte_to_padded_nibble(command_data[i], LOW);
    total_packet_bytes += 2;  // Two extra packet bytes for every data byte
  }

  packet_temp[total_packet_bytes + 0] = PREAMBLE_PATTERN_4;
  packet_temp[total_packet_bytes + 1] = PREAMBLE_PATTERN_3;
  packet_temp[total_packet_bytes + 2] = PREAMBLE_PATTERN_2;
  packet_temp[total_packet_bytes + 3] = PREAMBLE_PATTERN_1;
  total_packet_bytes += 4;  // Include outro bytes

  if (direction == UPSTREAM) {
    chain_left.write(packet_temp, total_packet_bytes);
    chain_left.flush();
  } else if (direction == DOWNSTREAM) {
    chain_right.write(packet_temp, total_packet_bytes);
    chain_right.flush();
  }

  /*
  chain_right.print("SENT PACKET( ");
  chain_right.print("DEST: ");
  chain_right.print(destination_address);
  chain_right.print(" ORIG: ");
  chain_right.print(origin_address);
  chain_right.print(" ID: ");
  chain_right.print(packet_id);
  chain_right.print(" TYPE: ");
  chain_right.print(command_type);
  chain_right.print(" DATA_LENGTH: ");
  chain_right.print(data_length_in_bytes);
  chain_right.print(" DATA: ");

  chain_right.print(" { ");
  for (uint8_t i = 0; i < data_length_in_bytes; i++) {
    uint8_t data_byte_high = packet_temp[16 + i * 2];
    uint8_t data_byte_low = packet_temp[17 + i * 2];
    uint8_t data_byte = (data_byte_high << 4) + data_byte_low;

    chain_right.print(data_byte);
    chain_right.print(", ");
  }
  chain_right.print(" } ");

  chain_right.println(")");
  */
}

void send_probe_response(uint8_t origin_address) {
  bool is_commander = false;
  bool seen_commander = false;
  if (assignment_complete == true) {
    seen_commander = true;
  }

  uint8_t flags = 0;
  bitWrite(flags, 1, is_commander);
  bitWrite(flags, 0, seen_commander);

  uint8_t probe_data[1] = { flags };

  send_packet(DOWNSTREAM, COM_PROBE_RESPONSE, origin_address, 1, probe_data);
}

void send_probe_request() {
  send_packet(UPSTREAM, COM_PROBE, ADDRESS_BROADCAST, 0, NULL_DATA);
}

void run_chain_discovery() {
  if (assignment_complete == false) {
    if (time_ms_now - last_probe_tx_time_ms >= DISCOVERY_PROBE_INTERVAL_MS) {
      //chain_right.println("SENDING PROBE REQUEST...");
      send_probe_request();

      last_probe_tx_time_ms = time_ms_now;
    }
  } else {
    if (probe_packet_received == false && probe_timeout_occurred == false) {
      if (time_ms_now >= probe_timeout_ms) {
        probe_timeout_occurred = true;

        // This is the terminating node, so enable propagation upstream now that discovery is complete
        terminating_node = true;
        propagation_queued = true;

        discovery_complete = true;
      }
    } else {
      if (probe_packet_received == true) {
        // Not the terminating node
        terminating_node = false;

        discovery_complete = true;
      }
    }
  }
}

void init_chain_config_defaults() {
  CHAIN_CONFIG.LOCAL_ADDRESS = ADDRESS_NULL;
  CHAIN_CONFIG.CHAIN_LENGTH = 0;
  CHAIN_CONFIG.PROPAGATION_MODE = false;
  CHAIN_CONFIG.BUS_MODE = false;
  assignment_complete = false;
  discovery_complete = false;
}

void init_chain_uart() {
  init_chain_config_defaults();

  chain_left.begin(DEFAULT_CHAIN_BAUD, SERIAL_8N1, SERIAL_2_RX_GPIO, SERIAL_2_TX_GPIO);
  chain_right.begin(DEFAULT_CHAIN_BAUD, SERIAL_8N1, SERIAL_0_RX_GPIO, SERIAL_0_TX_GPIO);
}

// Quickly shifts sync_buffer left using memmove,
// Adds newest byte at end
void feed_byte_into_sync_buffer(uint8_t from_direction, uint8_t incoming_byte) {
  memmove(sync_buffer[from_direction], sync_buffer[from_direction] + 1, (4 - 1) * sizeof(uint8_t));
  sync_buffer[from_direction][3] = incoming_byte;
}

void feed_byte_into_packet_buffer(uint8_t from_direction, uint8_t incoming_byte) {
  packet_buffer[from_direction][packet_buffer_index[from_direction]] = incoming_byte;
  packet_buffer_index[from_direction]++;
}

void init_packet(uint8_t from_direction) {
  memset(packet_buffer[from_direction], 0, sizeof(uint8_t) * 128);
  packet_buffer_index[from_direction] = 0;
  packet_started[from_direction] = true;
}

void execute_packet(uint8_t from_direction, uint8_t origin_address, uint16_t packet_id, uint8_t command_type, uint8_t data_length) {
  if(from_direction == UPSTREAM){
    upstream_packets_receieved++;
  }

  if (command_type == COM_PROBE) {
    packet_execution_flag = true;
    send_probe_response(origin_address);
    if (assignment_complete == true) {
      probe_packet_received = true;
    }
  }
  
  else if (command_type == COM_PROBE_RESPONSE) {
    packet_execution_flag = true;
    //chain_right.println("GOT RESPONSE");

    //uint8_t flags_byte_high = packet_buffer[from_direction][12];
    //uint8_t flags_byte_low = packet_buffer[from_direction][13];
    //uint8_t flags_byte = (flags_byte_high << 4) + flags_byte_low;

    //bool is_commander   = bitRead(flags_byte, 1);
    //bool seen_commander = bitRead(flags_byte, 0);

    if (origin_address == ADDRESS_NULL) {
      // DO NOTHING
    } else if (origin_address == ADDRESS_COMMANDER) {
      CHAIN_CONFIG.LOCAL_ADDRESS = 0;
      assignment_complete = true;
      probe_timeout_ms = time_ms_now + DISCOVERY_PROBE_INTERVAL_MS * 10;
      probe_timeout_occurred = false;
    } else if (origin_address == ADDRESS_NULL) {
      // This node isn't ready to help with assignment yet
    } else {  // node with assigned address
      CHAIN_CONFIG.LOCAL_ADDRESS = origin_address + 1;
      assignment_complete = true;
      probe_timeout_ms = time_ms_now + DISCOVERY_PROBE_INTERVAL_MS * 10;
      probe_timeout_occurred = false;
    }
  }
  
  else if (command_type == COM_ENABLE_PROPAGATION) {
    packet_execution_flag = true;
    CHAIN_CONFIG.PROPAGATION_MODE = true;

    if (from_direction == UPSTREAM) {
      send_packet(DOWNSTREAM, COM_ENABLE_PROPAGATION, ADDRESS_BROADCAST, 0, NULL_DATA);
    } else if (from_direction == DOWNSTREAM) {
      send_packet(UPSTREAM, COM_ENABLE_PROPAGATION, ADDRESS_BROADCAST, 0, NULL_DATA);
    }
  }
  
  else if (command_type == COM_LENGTH_INQUIRY) {
    if(terminating_node == true){
      packet_execution_flag = true;
      uint8_t chain_length_data[1] = { uint8_t(CHAIN_CONFIG.LOCAL_ADDRESS+1) };
      send_packet(UPSTREAM, COM_LENGTH_RESPONSE, ADDRESS_COMMANDER, 1, chain_length_data);
    }
    else{
      // Not for this node
    }
  }
  
  else if (command_type == COM_INFORM_CHAIN_LENGTH) {
    packet_execution_flag = true;
    uint8_t new_length = packet_data[from_direction][0];
    CHAIN_CONFIG.CHAIN_LENGTH = new_length;
  }
  
  else if (command_type == COM_SET_BACKLIGHT_COLOR) {
    packet_execution_flag = true;
    float new_r = packet_data[from_direction][0] / 255.0;
    float new_g = packet_data[from_direction][1] / 255.0;
    float new_b = packet_data[from_direction][2] / 255.0;
    CRGBF new_color = { new_r, new_g, new_b };
    
    set_backlight_color(new_color);
  }

  else if (command_type == COM_SET_FRAME_BLENDING) {
    packet_execution_flag = true;
    frame_blending_amount = packet_data[from_direction][0] / 255.0;
  }

  else if (command_type == COM_SET_BRIGHTNESS) {
    packet_execution_flag = true;
    float new_brightness = packet_data[from_direction][0] / 255.0;
    set_brightness(new_brightness);
  }

  else if (command_type == COM_SHOW) {
    packet_execution_flag = true;
    show_called_once = true;
    trigger_transition();
  }

  else if (command_type == COM_SET_TRANSITION_TYPE) {
    packet_execution_flag = true;
    uint8_t new_transition_type = packet_data[from_direction][0];
    set_transition_type( new_transition_type );
    //debug("NEW TRANSITION TYPE: ");
    //debugln(new_transition_type);
  }

  else if (command_type == COM_SET_TRANSITION_DURATION_MS) {
    packet_execution_flag = true;
    uint16_t new_duration_ms = ( packet_data[from_direction][0] << 8 ) + packet_data[from_direction][1];
    set_transition_time_ms( new_duration_ms );
    
    //debug("TRANSITION PACKET DATA: ");
    //debug("{ ");
    //debug_byte(packet_data[from_direction][0]);
    //debug(", ");
    //debug_byte(packet_data[from_direction][1]);
    //debugln(" }");

    //debug("NEW TRANSITION DURATION MS: ");
    //debugln(new_duration_ms);
  }

  else if (command_type == COM_SET_CHARACTER) {
    freeze_led_image = true;

    packet_execution_flag = true;
    char new_character = packet_data[from_direction][0];
    set_new_character( new_character );
    //debug("NEW CHARACTER: ");
    //debugln(new_character);

    freeze_led_image = false;
  }

  else if (command_type == COM_START_BUS_MODE) {
    packet_execution_flag = true;
    enable_bus_mode();

    if (from_direction == UPSTREAM) {
      send_packet(DOWNSTREAM, COM_START_BUS_MODE, ADDRESS_BROADCAST, 0, nullptr);
    }
    else if (from_direction == DOWNSTREAM) {
      send_packet(UPSTREAM, COM_START_BUS_MODE, ADDRESS_BROADCAST, 0, nullptr);
    }
  }

  else if (command_type == COM_END_BUS_MODE) {
    packet_execution_flag = true;
    disable_bus_mode();
  }

  else if(command_type == COM_SET_DEBUG_OVERLAY_OPACITY){
    packet_execution_flag = true;
    debug_led_opacity = packet_data[from_direction][0] / 255.0;
  }

  else if(command_type == COM_SET_DISPLAY_COLORS){
    packet_execution_flag = true;
    float new_r_a = packet_data[from_direction][0] / 255.0;
    float new_g_a = packet_data[from_direction][1] / 255.0;
    float new_b_a = packet_data[from_direction][2] / 255.0;

    float new_r_b = packet_data[from_direction][3] / 255.0;
    float new_g_b = packet_data[from_direction][4] / 255.0;
    float new_b_b = packet_data[from_direction][5] / 255.0;

    CRGBF new_color_a = { new_r_a, new_g_a, new_b_a };
    CRGBF new_color_b = { new_r_b, new_g_b, new_b_b };

    set_display_color( new_color_a, new_color_b );
  }

  else if(command_type == COM_SET_GRADIENT_TYPE){
    packet_execution_flag = true;
    uint8_t new_type = packet_data[from_direction][0];
    set_gradient_type( new_type ); 
  }

  else if(command_type == COM_SET_STRING){
    packet_execution_flag = true;
    char new_character = packet_data[from_direction][CHAIN_CONFIG.LOCAL_ADDRESS];
    
    set_new_character( new_character );
    //debug("NEW CHARACTER: ");
    //debugln(new_character);
  }

  else if(command_type == COM_SET_TRANSITION_INTERPOLATION){
    packet_execution_flag = true;
    uint8_t interpolation_type = packet_data[from_direction][0];
    SYSTEM_STATE.TRANSITION_INTERPOLATION = interpolation_type;

    //debug("NEW TRANSITION INTERPOLATION: ");
    //debugln(interpolation_type);
  }

  else if(command_type == COM_SET_TOUCH_GLOW_POSITION){
    packet_execution_flag = true;
    uint8_t position = packet_data[from_direction][0];
    SYSTEM_STATE.TOUCH_GLOW_POSITION = position;

    //debug("NEW TOUCH GLOW POSITION: ");
    //debugln(position);
  }

  else if(command_type == COM_SET_TOUCH_GLOW_COLOR){
    packet_execution_flag = true;
    SYSTEM_STATE.TOUCH_COLOR = { packet_data[from_direction][0] / 255.0F, packet_data[from_direction][1] / 255.0F, packet_data[from_direction][2] / 255.0F };
  }

  else if(command_type == COM_READ_TOUCH){
    packet_execution_flag = true;

    uint8_t touch_data[2] = { 
      get_byte_from_16_bit(SYSTEM_STATE.TOUCH_VALUE, HIGH),
      get_byte_from_16_bit(SYSTEM_STATE.TOUCH_VALUE, LOW),
    };

    send_packet(UPSTREAM, COM_READ_TOUCH_RESPONSE, ADDRESS_COMMANDER, 2, touch_data);
  }

  else if(command_type == COM_CALIBRATE_TOUCH){
    packet_execution_flag = true;

    uint8_t touch_type = packet_data[from_direction][0];

    if(touch_type == HIGH){
      STORAGE.TOUCH_HIGH_LEVEL = SYSTEM_STATE.TOUCH_VALUE-5;
    }
    else if(touch_type == LOW){
      STORAGE.TOUCH_LOW_LEVEL = SYSTEM_STATE.TOUCH_VALUE+5;
    }

    //save_storage();
  }

  else if(command_type == COM_SET_TOUCH_THRESHOLD){
    packet_execution_flag = true;

    STORAGE.TOUCH_THRESHOLD = packet_data[from_direction][0] / 255.0;
    //save_storage();
  }

  else if(command_type == COM_SAVE_STORAGE){
    packet_execution_flag = true;

    save_storage();
  }
}

void parse_packet(uint8_t from_direction) {
  memcpy(last_packet, packet_buffer[from_direction], 128);

  uint8_t destination_address_high = packet_buffer[from_direction][0];
  uint8_t destination_address_low = packet_buffer[from_direction][1];
  uint8_t destination_address = (destination_address_high << 4) + destination_address_low;

  uint8_t origin_address_high = packet_buffer[from_direction][2];
  uint8_t origin_address_low = packet_buffer[from_direction][3];
  uint8_t origin_address = (origin_address_high << 4) + origin_address_low;

  uint8_t packet_id_a = packet_buffer[from_direction][4];
  uint8_t packet_id_b = packet_buffer[from_direction][5];
  uint8_t packet_id_c = packet_buffer[from_direction][6];
  uint8_t packet_id_d = packet_buffer[from_direction][7];

  uint16_t packet_id = 0;
  packet_id = (packet_id << 4) + packet_id_a;
  packet_id = (packet_id << 4) + packet_id_b;
  packet_id = (packet_id << 4) + packet_id_c;
  packet_id = (packet_id << 4) + packet_id_d;

  uint8_t command_type_high = packet_buffer[from_direction][8];
  uint8_t command_type_low = packet_buffer[from_direction][9];
  uint8_t command_type = (command_type_high << 4) + command_type_low;

  uint8_t data_length_high = packet_buffer[from_direction][10];
  uint8_t data_length_low = packet_buffer[from_direction][11];
  uint8_t data_length = (data_length_high << 4) + data_length_low;

  memset(packet_data[from_direction], 0, sizeof(uint8_t) * 64);
  for (uint8_t i = 0; i < data_length; i++) {
		uint8_t data_byte_high = packet_buffer[from_direction][12 + i * 2];
		uint8_t data_byte_low = packet_buffer[from_direction][13 + i * 2];
		uint8_t data_byte = (data_byte_high << 4) + data_byte_low;

		packet_data[from_direction][i] = data_byte;
	}

  //chain_right.print("GOT PACKET( ");
  //chain_right.print("DEST: ");
  //chain_right.print(destination_address);
  //chain_right.print(" ORIG: ");
  //chain_right.print(origin_address);
  //chain_right.print(" ID: ");
  //chain_right.print(packet_id);
  //chain_right.print(" TYPE: ");
  //chain_right.print(command_type);
  //chain_right.print(" DATA_LENGTH: ");
  //chain_right.print(data_length);
  //chain_right.print(" DATA: ");

  //chain_right.print(" { ");
  for (uint8_t i = 0; i < data_length; i++) {
    //uint8_t data_byte_high = packet_buffer[from_direction][12 + i * 2];
    //uint8_t data_byte_low = packet_buffer[from_direction][13 + i * 2];
    //uint8_t data_byte = (data_byte_high << 4) + data_byte_low;

    //chain_right.print(data_byte);
    //chain_right.print(", ");
  }
  //chain_right.print(" } ");

  //chain_right.println(")");

  if (destination_address == ADDRESS_BROADCAST || destination_address == CHAIN_CONFIG.LOCAL_ADDRESS || command_type == COM_PROBE) {
    execute_packet(from_direction, origin_address, packet_id, command_type, data_length);
  }
}

void finalize_packet(uint8_t from_direction) {
  if (from_direction == UPSTREAM) {
    rx_flag_left = true;
  } else if (from_direction == DOWNSTREAM) {
    rx_flag_right = true;
  }

  packet_buffer_index[from_direction] -= 4;

  //chain_right.print("GOT VALID PACKET FROM ");

  if (from_direction == UPSTREAM) {
    //chain_right.print("UPSTREAM");
  } else if (from_direction == DOWNSTREAM) {
    //chain_right.print("DOWNSTREAM");
  }

  //chain_right.print("! {");

  for (uint8_t i = 0; i < packet_buffer_index[from_direction]; i++) {
    //chain_right.print(uint8_t(packet_buffer[from_direction][i]));
    //chain_right.print(", ");
  }
  //chain_right.println("}");

  // -------------------------------------------
  parse_packet(from_direction);
  // -------------------------------------------

  packet_buffer_index[from_direction] = 0;
  packet_started[from_direction] = false;
}

void parse_incoming_data_from(uint8_t from_direction) {
  uint8_t byte = 0;

  if (from_direction == UPSTREAM) {
    byte = chain_left.read();

    if(CHAIN_CONFIG.PROPAGATION_MODE == true && DEBUG_MODE == false){
      tx_flag_right = true;
      if(CHAIN_CONFIG.BUS_MODE == false){
        chain_right.write(byte);
      }
    }
  } else if (from_direction == DOWNSTREAM) {
    byte = chain_right.read();

    if(CHAIN_CONFIG.PROPAGATION_MODE == true){
      tx_flag_left = true;
      chain_left.write(byte);
    }
  }

  if (packet_started[from_direction] == true) {
    if (from_direction == UPSTREAM) {
      //chain_right.print("UPSTREAM");
    } else if (from_direction == DOWNSTREAM) {
      //chain_right.print("DOWNSTREAM");
    }

    //chain_right.print(" PACKET BYTE: ");
    //chain_right.println(byte);
    feed_byte_into_packet_buffer(from_direction, byte);
  } else {
    //chain_right.print(" FAIL BYTE: ");
    //chain_right.println(byte);
  }

  feed_byte_into_sync_buffer(from_direction, byte);

  // Packet started
  if (sync_buffer[from_direction][0] == PREAMBLE_PATTERN_1) {
    if (sync_buffer[from_direction][1] == PREAMBLE_PATTERN_2) {
      if (sync_buffer[from_direction][2] == PREAMBLE_PATTERN_3) {
        if (sync_buffer[from_direction][3] == PREAMBLE_PATTERN_4) {
          //chain_right.println("PACKET START PATTERN DETECTED");
          init_packet(from_direction);
        }
      }
    }
  }

  // Packet ended (reversed preamble)
  else if (sync_buffer[from_direction][0] == PREAMBLE_PATTERN_4) {
    if (sync_buffer[from_direction][1] == PREAMBLE_PATTERN_3) {
      if (sync_buffer[from_direction][2] == PREAMBLE_PATTERN_2) {
        if (sync_buffer[from_direction][3] == PREAMBLE_PATTERN_1) {
          //chain_right.println("PACKET END PATTERN DETECTED");
          finalize_packet(from_direction);
        }
      }
    }
  }
}


void send_touch_event() {
  uint8_t touch_data[1] = { SYSTEM_STATE.TOUCH_ACTIVE };

  //debug("SENDING TOUCH EVENT: ");
  //debugln(touch_data[0]);

  send_packet(UPSTREAM, COM_TOUCH_EVENT, ADDRESS_COMMANDER, 1, touch_data);
}


void receive_chain_data() {
  if (propagation_queued == true) {
    propagation_queued = false;
    CHAIN_CONFIG.PROPAGATION_MODE = true;
    CHAIN_CONFIG.BUS_MODE = true;

    send_packet(UPSTREAM, COM_ENABLE_PROPAGATION, ADDRESS_BROADCAST, 0, nullptr);    
    send_packet(UPSTREAM, COM_START_BUS_MODE,     ADDRESS_BROADCAST, 0, nullptr);
  }

  while (chain_left.available() > 0 || chain_right.available() > 0) {
    // Data coming from the left
    if (chain_left.available() > 0) {
      parse_incoming_data_from(UPSTREAM);
    }

    // Data coming from the right
    if (chain_right.available() > 0) {
      parse_incoming_data_from(DOWNSTREAM);
    }
  }
}

void check_transition_completion() {
  if (transition_complete_flag == true) {
    transition_complete_flag = false;
    if(CHAIN_CONFIG.LOCAL_ADDRESS == ADDRESS_CHAIN_HEAD){
      send_packet(UPSTREAM, COM_TRANSITION_COMPLETE, ADDRESS_COMMANDER, 0, nullptr);
    }
  }
}