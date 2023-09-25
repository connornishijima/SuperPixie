/*!
 * @file SuperPixie.cpp
 *
 * Designed specifically to work with SuperPixie:
 * ----> www.lixielabs.com/superpixie
 *
 * Last Updated by Connor Nishijima on 8/7/23
 */

#include "SuperPixie.h"
#include "utilities.h"

// ---------------------------------------------------------------------------------------------------------|
// -- PUBLIC CLASS FUNCTIONS -------------------------------------------------------------------------------|
// ---------------------------------------------------------------------------------------------------------|

SuperPixie::SuperPixie(){}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% FUNCTIONS - SETUP %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void SuperPixie::begin( uint8_t data_a_pin_, uint8_t data_b_pin_ ) {	
	if(debug_mode == true){
		Serial.begin(DEBUG_BAUD);
		debug("\n");
	}
	debug("----- begin(");
	debug(data_a_pin_);
	debug(", ");
	debug(data_b_pin_);
	debugln(")");
	
	// Store data GPIO pins for later
	data_a_pin = data_a_pin_;
	data_b_pin = data_b_pin_;
	
	init_system();
}


// Bootup sequence
void SuperPixie::init_system(){
	debugln("----- init_system()");
	
	pinMode(2, OUTPUT);
	digitalWrite(2, HIGH);
	
	chain_initialized = false;
	
	init_uart();
	init_serial_isr();
	
	//delay(100);
	
	reset_chain();
	
	//delay(100);
	
	debugln("INIT SYSTEM COMPLETE");
}

void SuperPixie::reset_chain(){
	chain_initialized = false;
	
	// SEND RESET PULSE
	pinMode( data_a_pin, OUTPUT );
	
	digitalWrite( data_a_pin, LOW );
	delay( RESET_PULSE_DURATION_MS );
	digitalWrite( data_a_pin, HIGH );
	
	pinMode( data_a_pin, INPUT );
	
	chain.begin(DEFAULT_CHAIN_BAUD, SWSERIAL_8N1, data_b_pin, data_a_pin);
	chain.listen();
	
	while(chain_initialized == false){
		//debugln("INIT WAIT");
		delay(10);
	}
}

void SuperPixie::init_serial_isr(){
	serial_checker.attach(1.0 / SERIAL_READ_HZ, [this]() { receive_chain_data(); });
}


// Set up UART communication
void SuperPixie::init_uart(){
	debugln("----- init_uart()");
	
    chain.begin(DEFAULT_CHAIN_BAUD, SWSERIAL_8N1, data_b_pin, data_a_pin);
	chain.listen();
	
	chain.flush();
	while(chain.available() > 0){
		uint8_t null = chain.read();
	}
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% FUNCTIONS - WRITE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


// Send command packet to broadcast or a specific address, strictly forcing acknowledgement
uint16_t SuperPixie::send_packet(uint8_t command_type, uint8_t destination_address, uint8_t data_length_in_bytes, uint8_t* command_data) {
	debugln("TX: ");
	debug("  TYPE:\t");
	debugln(command_type);
	debug("  DEST:\t");
	debugln(destination_address);
	debug("  DATA LENGTH:\t");
	debugln(data_length_in_bytes);
	
	if(data_length_in_bytes > 0){
		debug("  DATA:\t");
		for(uint8_t i = 0; i < data_length_in_bytes; i++){
			debug(command_data[i]);
			debug("\t");
		}
		debugln(" ");
	}
	
	uint8_t packet_temp[128] = { 0 };
	uint8_t origin_address = ADDRESS_COMMANDER;
	uint16_t packet_id = random(0, 65535);

	// Packet header
	packet_temp[0] = PREAMBLE_PATTERN_1;
	packet_temp[1] = PREAMBLE_PATTERN_2;
	packet_temp[2] = PREAMBLE_PATTERN_3;
	packet_temp[3] = PREAMBLE_PATTERN_4;

	packet_temp[4] = byte_to_padded_nibble(destination_address, HIGH);
	packet_temp[5] = byte_to_padded_nibble(destination_address, LOW);

	packet_temp[6] = byte_to_padded_nibble(origin_address, HIGH);
	packet_temp[7] = byte_to_padded_nibble(origin_address, LOW);

	packet_temp[8] = get_byte_from_16_bit(byte_to_padded_nibble(packet_id, HIGH), HIGH);
	packet_temp[9] = get_byte_from_16_bit(byte_to_padded_nibble(packet_id, LOW), HIGH);
	packet_temp[10] = get_byte_from_16_bit(byte_to_padded_nibble(packet_id, HIGH), LOW);
	packet_temp[11] = get_byte_from_16_bit(byte_to_padded_nibble(packet_id, LOW), LOW);

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

	chain.write(packet_temp, total_packet_bytes);
	chain.flush();

	return packet_id;
}


// Quickly shifts sync buffer left using memmove,
// Adds newest byte at end
void SuperPixie::feed_byte_into_sync_buffer(uint8_t incoming_byte) {
	memmove(sync_buffer, sync_buffer + 1, (4 - 1) * sizeof(uint8_t));
	sync_buffer[3] = incoming_byte;
}


void SuperPixie::feed_byte_into_packet_buffer(uint8_t incoming_byte) {
	packet_buffer[packet_buffer_index] = incoming_byte;
	packet_buffer_index++;
}


void SuperPixie::init_packet() {
	memset(packet_buffer, 0, sizeof(uint8_t) * 128);
	packet_buffer_index = 0;
	packet_started = true;
}


void SuperPixie::parse_packet() {
	uint8_t destination_address_high = packet_buffer[0];
	uint8_t destination_address_low = packet_buffer[1];
	uint8_t destination_address = (destination_address_high << 4) + destination_address_low;

	uint8_t origin_address_high = packet_buffer[2];
	uint8_t origin_address_low = packet_buffer[3];
	uint8_t origin_address = (origin_address_high << 4) + origin_address_low;

	uint8_t packet_id_a = packet_buffer[4];
	uint8_t packet_id_b = packet_buffer[5];
	uint8_t packet_id_c = packet_buffer[6];
	uint8_t packet_id_d = packet_buffer[7];

	uint16_t packet_id = 0;
	packet_id = (packet_id << 4) + packet_id_a;
	packet_id = (packet_id << 4) + packet_id_b;
	packet_id = (packet_id << 4) + packet_id_c;
	packet_id = (packet_id << 4) + packet_id_d;

	uint8_t command_type_high = packet_buffer[8];
	uint8_t command_type_low = packet_buffer[9];
	uint8_t command_type = (command_type_high << 4) + command_type_low;

	uint8_t data_length_high = packet_buffer[10];
	uint8_t data_length_low = packet_buffer[11];
	uint8_t data_length = (data_length_high << 4) + data_length_low;

	memset(packet_data, 0, sizeof(uint8_t) * 64);
	for (uint8_t i = 0; i < data_length; i++) {
		uint8_t data_byte_high = packet_buffer[12 + i * 2];
		uint8_t data_byte_low = packet_buffer[13 + i * 2];
		uint8_t data_byte = (data_byte_high << 4) + data_byte_low;

		packet_data[i] = data_byte;
	}

	if (destination_address == ADDRESS_BROADCAST || destination_address == ADDRESS_COMMANDER) {
		execute_packet(origin_address, packet_id, command_type, data_length);
	}
	else{
		// Not for this node (Commander)
	}
}


void SuperPixie::execute_packet(uint8_t origin_address, uint16_t packet_id, uint8_t command_type, uint8_t data_length_in_bytes) {
	debugln("RX: ");
	debug("  ORIG:\t");
	debugln(origin_address);
	debug("  PACKET_ID:\t");
	debugln(packet_id);
	debug("  COMMAND_TYPE:\t");
	debugln(command_type);
	debug("  DATA LENGTH:\t");
	debugln(data_length_in_bytes);
	
	if(data_length_in_bytes > 0){
		debug("  DATA:\t");
		for(uint8_t i = 0; i < data_length_in_bytes; i++){
			debug(packet_data[i]);
			debug("\t");
		}
		debugln(" ");
	}
	
	if (command_type == COM_PROBE) {
		send_probe_response(origin_address);
	}
	else if (command_type == COM_START_BUS_MODE) {
		send_packet(COM_LENGTH_INQUIRY, ADDRESS_BROADCAST, 0, nullptr);
	}
	else if (command_type == COM_LENGTH_RESPONSE) {
		chain_length = packet_data[0];
		debug("FINAL CHAIN LENGTH: ");
		debugln(chain_length);

		// Inform nodes of discovered length
		uint8_t length_data[1] = { chain_length };
		send_packet(COM_INFORM_CHAIN_LENGTH, ADDRESS_BROADCAST, 1, length_data);

		chain_initialized = true;
		
		//delay(100);
		
		//bus_ready = false;
		//start_bus_mode();
		//while(bus_ready == false){
			//delay(10);
		//}
	}
	else if (command_type == COM_BUS_READY) {
		bus_ready = true;
	}
	else if (command_type == COM_TOUCH_EVENT) {
		bool touch = packet_data[0];
		
		debug("TOUCH EVENT: ");
		debug(uint8_t(touch));
		debug(" @ ");
		debugln(origin_address);
	}
	else if (command_type == COM_TRANSITION_COMPLETE) {
		show_complete = true;
	}
}


void SuperPixie::set_string( char* string ){
	uint8_t length = strlen(string);
	send_packet(COM_SET_STRING, ADDRESS_BROADCAST, length, reinterpret_cast<uint8_t*>(string));
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% FUNCTIONS - UPDATING THE MASK / LEDS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void SuperPixie::set_scroll_speed( uint16_t scroll_time_ms, uint16_t hold_time_ms, uint8_t destination_address ){
	uint8_t scroll_time_ms_high = get_byte_from_16_bit(scroll_time_ms, 1);
	uint8_t scroll_time_ms_low  = get_byte_from_16_bit(scroll_time_ms, 0);
	
	uint8_t hold_time_ms_high = get_byte_from_16_bit(hold_time_ms, 1);
	uint8_t hold_time_ms_low  = get_byte_from_16_bit(hold_time_ms, 0);

	uint8_t scroll_speed_data[4] = { scroll_time_ms_high, scroll_time_ms_low, hold_time_ms_high, hold_time_ms_low };
	//send_command(destination_address, COM_SET_SCROLL_SPEED, scroll_speed_data, 4);
}


void SuperPixie::set_brightness(float brightness, uint8_t destination_address){
	uint8_t brightness_data[1] = { brightness*255 };
	send_packet(COM_SET_BRIGHTNESS, destination_address, 1, brightness_data);
}


void SuperPixie::set_transition_type(transition_type_t type, uint8_t destination_address){
	uint8_t transition_data[1] = { type };
	send_packet(COM_SET_TRANSITION_TYPE, destination_address, 1, transition_data);
}


void SuperPixie::set_character( uint8_t new_character, uint8_t destination_address ){
	uint8_t character_data[1] = { new_character };
	send_packet(COM_SET_CHARACTER, destination_address, 1, character_data);
}


void SuperPixie::set_transition_duration_ms(uint16_t duration_ms, uint8_t destination_address){
	uint8_t duration_high = get_byte_from_16_bit(duration_ms, 1);
	uint8_t duration_low  = get_byte_from_16_bit(duration_ms, 0);

	uint8_t duration_data[2] = { duration_high, duration_low };
	send_packet(COM_SET_TRANSITION_DURATION_MS, destination_address, 2, duration_data);
}


void SuperPixie::set_frame_blending(float blend_val, uint8_t destination_address){
	uint8_t blend_data[1] = { blend_val*255 };
	send_packet(COM_SET_FRAME_BLENDING, destination_address, 1, blend_data);
}


void SuperPixie::set_color(CRGB color, uint8_t destination_address) {
	set_color(color, color, destination_address);
}


void SuperPixie::set_color(CRGB color_a, CRGB color_b, uint8_t destination_address) {
	uint8_t color_data[6] = { color_a.r, color_a.g, color_a.b, color_b.r, color_b.g, color_b.b };
	send_packet(COM_SET_DISPLAY_COLORS, destination_address, 6, color_data);
}


void SuperPixie::set_gradient_type(gradient_type_t type, uint8_t destination_address){
	uint8_t gradient_data[1] = { type };
	send_packet(COM_SET_GRADIENT_TYPE, destination_address, 1, gradient_data);
}


void SuperPixie::set_backlight_color( CRGB col, uint8_t destination_address ){
	uint8_t backlight_data[3] = { col.r, col.g, col.b };
	send_packet(COM_SET_BACKLIGHT_COLOR, destination_address, 3, backlight_data);
}

void SuperPixie::set_debug_overlay_opacity( float opacity, uint8_t destination_address ){
	uint8_t opacity_data[1] = { opacity*255 };
	send_packet(COM_SET_DEBUG_OVERLAY_OPACITY, destination_address, 1, opacity_data);
}


void SuperPixie::clear(){
	
}


void SuperPixie::show(){
	static uint32_t t_us_last = micros();
	uint32_t t_us_now = micros();
	
	if(t_us_now - t_us_last >= MINIMUM_UPDATE_MICROS){
		//show_called_once = true;
		show_complete = false;
		send_packet(COM_SHOW, ADDRESS_BROADCAST, 0, nullptr);
		t_us_last = t_us_now;
	}
}

void SuperPixie::wait(){
	const uint32_t wait_timeout_ms = 10000;
	uint32_t t_start = millis();
	while(millis() - t_start <= wait_timeout_ms && show_complete == false){
		yield();
	}
	
	if(show_complete == false){
		// THIS IS BAD, NO RESPONSE WAS RECIEVED IN TIME
	}
}







void SuperPixie::start_bus_mode(){
	send_packet(COM_START_BUS_MODE, ADDRESS_BROADCAST, 0, nullptr);
}


void SuperPixie::end_bus_mode(){
	send_packet(COM_END_BUS_MODE, ADDRESS_BROADCAST, 0, nullptr);
}




void SuperPixie::set_transition_interpolation( uint8_t interpolation_type ){
	uint8_t interpolation_data[1] = { interpolation_type };
	send_packet(COM_SET_TRANSITION_INTERPOLATION, ADDRESS_BROADCAST, 1, interpolation_data);
}




void SuperPixie::send_probe_response(uint8_t origin_address) {
  uint8_t flags = 0;
  bool is_commander = true;
  bool seen_commander = true;

  bitWrite(flags, 1, is_commander);
  bitWrite(flags, 0, seen_commander);

  uint8_t probe_data[1] = { flags };

  send_packet(COM_PROBE_RESPONSE, origin_address, 1, probe_data);
}


void SuperPixie::finalize_packet() {
  packet_buffer_index -= 4;

  // -------------------------------------------
  parse_packet();
  // -------------------------------------------

  packet_buffer_index = 0;
  packet_started = false;

  sync_buffer[0] = 0;
  sync_buffer[1] = 0;
  sync_buffer[2] = 0;
  sync_buffer[3] = 0;
}


void SuperPixie::parse_incoming_data() {
  uint8_t byte = chain.read();

  if (packet_started == true) {
    feed_byte_into_packet_buffer(byte);
  }

  feed_byte_into_sync_buffer(byte);

  // Packet started
  if (sync_buffer[0] == PREAMBLE_PATTERN_1) {
    if (sync_buffer[1] == PREAMBLE_PATTERN_2) {
      if (sync_buffer[2] == PREAMBLE_PATTERN_3) {
        if (sync_buffer[3] == PREAMBLE_PATTERN_4) {
          init_packet();
        }
      }
    }
  }

  // Packet ended (reversed preamble)
  else if (sync_buffer[0] == PREAMBLE_PATTERN_4) {
    if (sync_buffer[1] == PREAMBLE_PATTERN_3) {
      if (sync_buffer[2] == PREAMBLE_PATTERN_2) {
        if (sync_buffer[3] == PREAMBLE_PATTERN_1) {
          finalize_packet();
        }
      }
    }
  }
}


void SuperPixie::receive_chain_data() {
  while (chain.available() > 0) {
    // Data coming from the chain
    if (chain.available() > 0) {
      parse_incoming_data();
    }
  }
}
