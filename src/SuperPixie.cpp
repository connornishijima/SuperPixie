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

void SuperPixie::begin( uint8_t data_a_pin, uint8_t data_b_pin, uint32_t baud_rate ) {	
	if(debug_mode == true){
		Serial.begin(DEBUG_BAUD);
	}
	debugln("----- begin()");

	init_system(data_a_pin, data_b_pin, baud_rate);
	
	chain_initialized = true;
}


void SuperPixie::set_baud_rate( uint32_t new_baud ){
	uint8_t baud_data[4] = {
		get_byte_from_uint32(new_baud, 3),
		get_byte_from_uint32(new_baud, 2),
		get_byte_from_uint32(new_baud, 1),
		get_byte_from_uint32(new_baud, 0)
	};
	
	send_packet(ADDRESS_BROADCAST, COM_SET_BAUD, baud_data, 4, false);
	
	chain.flush();
	chain.end();
	chain.begin(new_baud, SWSERIAL_8N1, data_a_pin_, data_b_pin_);
}


// Bootup sequence
void SuperPixie::init_system( uint8_t data_a_pin, uint8_t data_b_pin, uint32_t baud_rate ){
	debug("----- init_system(");
	debug(data_a_pin);
	debug(",");
	debug(data_b_pin);
	debug(",");
	debug(baud_rate);
	debugln(")");
	
	pinMode(2, OUTPUT);
	digitalWrite(2, HIGH);
	
	// SEND RESET PULSE
	pinMode(data_a_pin, OUTPUT);
	digitalWrite(data_a_pin, LOW);
	delay(RESET_PULSE_DURATION_MS << 1);
	digitalWrite(data_a_pin, HIGH);
	pinMode(data_a_pin, INPUT);
	
	init_uart(data_b_pin, data_a_pin);
	delay(400);
		
	discover_nodes();
	
	init_serial_isr();
	
	debugln("INIT SYSTEM COMPLETE");
}

void SuperPixie::init_serial_isr(){
	serial_checker.attach(1.0 / serial_read_hz, [this]() { consume_serial(); });
}


// Set up UART communication
void SuperPixie::init_uart( uint8_t data_a_pin, uint8_t data_b_pin ){
	debugln("----- init_uart()");
	
	data_a_pin_ = data_a_pin;
    data_b_pin_ = data_b_pin;
	
    chain.begin(DEFAULT_CHAIN_BAUD, SWSERIAL_8N1, data_a_pin_, data_b_pin_);
	chain.listen();
	
	chain.flush();
	while(chain.available() > 0){
		uint8_t null = chain.read();
	}
}


int8_t SuperPixie::read_touch(uint8_t chain_index){
	send_packet(chain_index, COM_GET_TOUCH_STATE, NULL_DATA, 0, false);
	
	uint32_t t_timeout = millis()+ACK_TIMEOUT_MS;
	touch_response[chain_index] = false;
	touch_state[chain_index] = -1;
	
	while(touch_response[chain_index] == false){
		consume_serial();
		
		if(millis() >= t_timeout){
			touch_response[chain_index] = true;
			// timed out, leave touch_state value at -1
		}
	}
	
	return touch_state[chain_index];
}


// Auto chain discovery
void SuperPixie::discover_nodes(){
	debugln("----- discover_nodes()");
	uint32_t del_time_ms = 10;
	
	chain_length = 0;
	
	//send_packet(ADDRESS_BROADCAST, COM_RESET_CHAIN, NULL_DATA, 0, false);
	//delay(50);

	uint8_t address_to_assign = 0;
	bool solved = false;

	debugln("BEGIN DISCOVERY");
	while(solved == false){
		uint8_t data[1] = { address_to_assign };
		if(send_packet_and_await_ack(ADDRESS_NULL, COM_ASSIGN_ADDRESS, data, 1) == true){
			debug("GOT ACK FROM NODE ");
			debugln(address_to_assign);
			//delay(10);
			//send_packet(address_to_assign, COM_ENABLE_PROPAGATION, NULL_DATA, 0, false);
			if(send_packet_and_await_ack(address_to_assign, COM_ENABLE_PROPAGATION, NULL_DATA, 0) == true){
				debug("PROPAGATION ENABLED ON NODE ");
				debugln(address_to_assign);
			}
			else{
				debug("NO RESPONSE FROM ");
				debugln(address_to_assign);
			}
			//delay(10);
			
			address_to_assign += 1;
			
			if(address_to_assign > chain_length){
				chain_length = address_to_assign;
			}

			//digitalWrite(2, LOW);
			//delay(10);
			//digitalWrite(2, HIGH);
			//delay(100);
		}
		else{
			debug("NO ACK FROM NODE ");
			debugln(address_to_assign);
			//digitalWrite(2, LOW);
			//delay(1000);
			//digitalWrite(2, HIGH);
			
			//leds[35] = CRGB(0,255,0);
			solved = true;
		}
	}
	
	
	//delay(del_time_ms);
	
	//measure_chain_length();
	
	debug("INITIAL CHAIN LENGTH: ");
	debugln(chain_length);
	
	announce_new_chain_length(chain_length);
	
	if(chain_length > 0){
		touch_response = new bool[ chain_length ]; 
		touch_state    = new int8_t[ chain_length ]; 
	}
	
	//delay(del_time_ms);
	
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% FUNCTIONS - WRITE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Send command packet to broadcast or a specific address, optionally requiring an ACK
uint16_t SuperPixie::send_packet(uint8_t dest_address, command_t packet_type, uint8_t* data, uint8_t data_length_in_bytes, bool get_ack) {
	
	
	//debug("---- send_packet(");
	//debug(dest_address);
	//debug(", ");
	//debug(uint32_t(packet_type));
	//debug(", [");
	for(uint8_t i = 0; i < data_length_in_bytes; i++){
		//debug(data[i]);
		if(i+1 != data_length_in_bytes){
			//debug(",");
		}
	}
	//debug("], ");
	//debug(data_length_in_bytes);
	//debug(", ");
	//debug(get_ack);
	//debugln(")");

	// Increment packet ID
	packet_id++;

	uint8_t packet_temp[256] = { 0 };

	// Packet header
	packet_temp[0] = SYNC_PATTERN_1;
	packet_temp[1] = SYNC_PATTERN_2;
	packet_temp[2] = SYNC_PATTERN_3;
	packet_temp[3] = SYNC_PATTERN_4;

	// 16-bit Packet command type as 2 bytes
	packet_temp[4] = get_byte_from_uint16(packet_id, 1);  // after: 8
	packet_temp[5] = get_byte_from_uint16(packet_id, 0);  // 7

	// Origin node
	packet_temp[6] = ADDRESS_COMMANDER;  // 6

	// Destination node, unless ADDRESS_BROADCAST
	packet_temp[7] = dest_address;  // 5

	// First Hop Address
	packet_temp[8] = ADDRESS_NULL;  // 4

	// Flags
	packet_temp[9] = 0;  // 3
	bitWrite(packet_temp[9], 7, get_ack);

	// 16-bit Packet command type as 2 bytes
	packet_temp[10] = get_byte_from_uint16(packet_type, 1);  // 2
	packet_temp[11] = get_byte_from_uint16(packet_type, 0);  // 1

	// How many bytes of extra data does this command have?
	packet_temp[12] = data_length_in_bytes;  // 0

	// The data content, if any
	for (uint16_t i = 0; i < data_length_in_bytes; i++) {
		packet_temp[13 + i] = data[i];
	}

	chain.write(packet_temp, 13 + data_length_in_bytes);
	chain.flush();
	
	//debug("PACKET SENT: ");
	for(uint16_t i = 0; i < 13 + data_length_in_bytes; i++){
		//debug(packet_temp[i]);
		//debug(",");
	}
	//debugln(" ");
	
	//debug("PACKET_ID SENT: ");
	//debugln(packet_id);

	// Return the ID the packet was assigned
	return packet_id;
}


bool SuperPixie::send_packet_and_await_ack(uint8_t dest_address, command_t packet_type, uint8_t* data, uint8_t data_length_in_bytes) {
	//debug("---- send_packet_and_await_ack(");
	//debug(dest_address);
	//debug(", ");
	//debug(uint32_t(packet_type));
	//debug(", [");
	for(uint8_t i = 0; i < data_length_in_bytes; i++){
		//debug(data[i]);
		if(i+1 != data_length_in_bytes){
			//debug(",");
		}
	}
	//debug("], ");
	//debug(data_length_in_bytes);
	//debugln(")");
	
	bool success = true;

	uint16_t packet_id = send_packet(dest_address, packet_type, data, data_length_in_bytes, true);
	packet_acks[packet_acks_index][0] = packet_id;
	packet_acks[packet_acks_index][1] = false;
	packet_acks_index++;
	if (packet_acks_index >= 8) {
		packet_acks_index = 0;
	}

	uint32_t t_start = millis();
	uint32_t ack_timeout = t_start + ACK_TIMEOUT_MS;

	bool got_ack = false;
	bool got_timeout = false;
	
	//debugln("BEGIN ACK WAIT");
	while (got_ack == false && got_timeout == false) {
		if (millis() >= ack_timeout) {
			got_timeout = true;
		}

		got_ack = await_ack(packet_id);
	}

	if (got_timeout == true) {
		success = false;
	}

	return success;
}


bool SuperPixie::await_ack(uint32_t packet_id) {
	consume_serial();

	for (uint8_t i = 0; i < 8; i++) {
		if (packet_acks[i][0] == packet_id && packet_acks[i][1] == true) {
			return true;
		}
	}

	return false;
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
	memset(packet_buffer, 0, sizeof(uint8_t) * 256);
	packet_buffer_index = 0;
	packet_started = true;
	packet_got_header = false;
	packet_read_counter = 9;
}


void SuperPixie::parse_packet() {
	uint8_t start_index = 0;
	// 16-bit ID of this packet for ACK use
	uint8_t id_a = packet_buffer[start_index + 0];
	uint8_t id_b = packet_buffer[start_index + 1];

	// Parse 2 bytes into 16-bit
	uint16_t packet_id = 0;
	packet_id = (packet_id << 8) + id_a;
	packet_id = (packet_id << 8) + id_b;

	// Address this packet came from
	uint8_t origin_address = packet_buffer[start_index + 2];

	// Address this packet is intended for
	uint8_t dest_address = packet_buffer[start_index + 3];

	// Address this packet is intended for
	uint8_t first_hop_address = packet_buffer[start_index + 4];

	// Boolean flags
	uint8_t packet_flags = packet_buffer[start_index + 5];

	// Command type of packet
	uint8_t packet_type_high = packet_buffer[start_index + 6];
	uint8_t packet_type_low = packet_buffer[start_index + 7];
	command_t packet_type = (command_t)((packet_type_high << 8) + packet_type_low);

	// Was an ACK requested?
	bool needs_ack = bitRead(packet_flags, 7);

	// How many bytes of extra data does this command have?
	uint8_t data_length_in_bytes = packet_buffer[start_index + 8];

	// This is where that extra data begins:
	uint8_t data_start_position = start_index + 9;

	/*
	debug("ID: ");
	debug(packet_id);
	debug("\t SIZE: ");
	debug(packet_size);
	debug("\t DEST ADDRESS: ");
	debug(dest_address);
	debug("\t PACKET TYPE: ");
	debug(packet_type);
	debug("\t DATA_LENGTH: ");
	debug(data_length_in_bytes);
	*/

	if (dest_address == ADDRESS_COMMANDER) {
		execute_packet(packet_id, origin_address, first_hop_address, packet_type, needs_ack, data_length_in_bytes, data_start_position);
	}
}


void SuperPixie::execute_packet(uint16_t packet_id, uint8_t origin_address, uint8_t first_hop_address, command_t packet_type, bool needs_ack, uint8_t data_length_in_bytes, uint8_t data_start_position) {
	if (packet_type == COM_ACK) {
		uint8_t ack_node_address = origin_address;

		for (uint8_t i = 0; i < 8; i++) {
			if (packet_acks[i][0] == packet_id) {
				packet_acks[i][1] = true;
				break;
			}
		}
	}
	else if (packet_type == COM_ANNOUNCE) {
		uint8_t last_good_node = first_hop_address;
		uint8_t address_to_assign = last_good_node + 1;
		if(last_good_node == ADDRESS_NULL){
			address_to_assign = 0;
		}
		uint8_t data[1] = { address_to_assign };

		if (send_packet_and_await_ack(ADDRESS_NULL, COM_ASSIGN_ADDRESS, data, 1) == true) {
			send_packet(address_to_assign, COM_ENABLE_PROPAGATION, NULL_DATA, 0, false);
			chain_length = address_to_assign+1;
			announce_new_chain_length(chain_length);
		}
	}
	else if (packet_type == COM_TOUCH_STATE) {
		touch_response[origin_address] = true;
		touch_state[origin_address] = packet_buffer[data_start_position + 0];
	}
	else if (packet_type == COM_TOUCH_EVENT) {
		pinMode(2, OUTPUT);
		
		// Non-polling touch event
		touch_state[origin_address] = packet_buffer[data_start_position + 0];
		Serial.print("TOUCH ");
		if(touch_state[origin_address] == true){
			Serial.print("START @ ");
			digitalWrite(2,!HIGH);
		}
		else{
			Serial.print("END   @ ");
			digitalWrite(2,!LOW);
		}
		Serial.println(origin_address);
	}
	else if (packet_type == COM_READY_STATUS) {
		bool ready_status = packet_buffer[data_start_position + 0];
		if(ready_status == true){
			waiting = false;
		}
	}
	else if (packet_type == COM_PANIC) {
		uint32_t t_start = millis();
		
		// Re-address orphaned node
		uint8_t address_to_assign = 0;
		if(first_hop_address != ADDRESS_NULL){ // Panic packet hopped at least once
			address_to_assign = first_hop_address+1;
		}
		else { // First in chain, panic packet never hopped
			address_to_assign = 0;
		}
		
		debug("NODE PANIC @ ");
		debugln(address_to_assign);
		
		//chain_length = 0;
		
		uint8_t data[1] = { address_to_assign };
		if(send_packet_and_await_ack(ADDRESS_NULL, COM_ASSIGN_ADDRESS, data, 1) == true){
			debug("GOT ACK FROM NODE ");
			debugln(address_to_assign);
			
			if(send_packet_and_await_ack(address_to_assign, COM_ENABLE_PROPAGATION, NULL_DATA, 0) == true){
				debug("PROPAGATION ENABLED ON NODE ");
				debugln(address_to_assign);
			}
			
			if(address_to_assign+1 > chain_length){
				//chain_length = address_to_assign+1;
			}
		}
		
		measure_chain_length();
		
		uint32_t t_end = millis();
		debug("TIME TO HEAL CHAIN: ");
		debugln(t_end-t_start);
		
		//announce_new_chain_length(chain_length);
		
		while (chain.available() > 0) { // Flush all incoming data and start fresh
			uint8_t byte = chain.read();
		}
	}
}


void SuperPixie::measure_chain_length(){
	bool solved = false;
	chain_length = 0;
	while(solved == false){
		uint8_t address = chain_length;
		if(send_packet_and_await_ack(address, COM_PROBE, NULL_DATA, 0) == true){
			chain_length++;
		}
		else{
			//chain_length--;
			solved = true;
		}
	}
	
	debug("NEW CHAIN LENGTH: ");
	debugln(chain_length);
	
	if(chain_length > 0){
		announce_new_chain_length(chain_length);
	}
}


void SuperPixie::announce_new_chain_length(uint8_t length){
	debug("ANNOUNCING NEW CHAIN LENGTH: ");
	debugln(length);
	uint8_t data[1] = { length };
	send_packet(ADDRESS_BROADCAST, COM_SET_CHAIN_LENGTH, data, 1, false);
	//send_packet(ADDRESS_BROADCAST, COM_SET_CHAIN_LENGTH, data, 1, false);
	//send_packet(ADDRESS_BROADCAST, COM_SET_CHAIN_LENGTH, data, 1, false);
}


void SuperPixie::consume_serial() {
	static uint8_t iter = 0;
	
	iter++;
	
	if(iter == 10){
		iter = 0;
		//send_packet(ADDRESS_BROADCAST, COM_NULL, NULL_DATA, 0, false);
	}
	//Serial.println(millis());
	
	// Data returning to chain commander
	while (chain.available() > 0) {
		uint8_t byte = chain.read();
		
		//debug("RX: ");
		//debugln(byte);
		//blink();

		feed_byte_into_sync_buffer(byte);

		// Packet started
		if (sync_buffer[0] == SYNC_PATTERN_1) {
			if (sync_buffer[1] == SYNC_PATTERN_2) {
				if (sync_buffer[2] == SYNC_PATTERN_3) {
					if (sync_buffer[3] == SYNC_PATTERN_4) {
						//blink();
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
				parse_packet();
				packet_started = false;
			}
		}
	}
}


void SuperPixie::set_string( char* string ){
	uint8_t length = strlen(string);
	send_packet(ADDRESS_BROADCAST, COM_SEND_STRING, reinterpret_cast<uint8_t*>(string), length, false);
}


void SuperPixie::scroll_string( char* string ){
	uint8_t length = strlen(string);
	send_packet(ADDRESS_BROADCAST, COM_SCROLL_STRING, reinterpret_cast<uint8_t*>(string), length, false);
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% FUNCTIONS - UPDATING THE MASK / LEDS %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void SuperPixie::set_scroll_speed( uint16_t scroll_time_ms, uint16_t hold_time_ms ){
	uint8_t scroll_time_ms_high = get_byte_from_uint16(scroll_time_ms, 1);
	uint8_t scroll_time_ms_low  = get_byte_from_uint16(scroll_time_ms, 0);
	
	uint8_t hold_time_ms_high = get_byte_from_uint16(hold_time_ms, 1);
	uint8_t hold_time_ms_low  = get_byte_from_uint16(hold_time_ms, 0);

	uint8_t scroll_speed_data[4] = { scroll_time_ms_high, scroll_time_ms_low, hold_time_ms_high, hold_time_ms_low };
	send_packet(ADDRESS_BROADCAST, COM_SET_SCROLL_SPEED, scroll_speed_data, 4, false);
}


void SuperPixie::set_brightness(float brightness){
	uint8_t data[1] = { brightness*255 };
	send_packet(ADDRESS_BROADCAST, COM_SET_BRIGHTNESS, data, 1, false);
}


void SuperPixie::set_transition_type(transition_type_t type){
	uint8_t transition_data[1] = { type };
	send_packet(ADDRESS_BROADCAST, COM_SET_TRANSITION_TYPE, transition_data, 1, false);
}


void SuperPixie::set_transition_duration_ms(uint16_t duration_ms){
	uint8_t duration_high = get_byte_from_uint16(duration_ms, 1);
	uint8_t duration_low  = get_byte_from_uint16(duration_ms, 0);

	uint8_t duration_data[2] = { duration_high, duration_low };
	send_packet(ADDRESS_BROADCAST, COM_SET_TRANSITION_DURATION_MS, duration_data, 2, false);
}


void SuperPixie::set_frame_blending(float blend_val){
	uint8_t blend_data[1] = { blend_val*255 };
	send_packet(ADDRESS_BROADCAST, COM_SET_FRAME_BLENDING, blend_data, 1, false);
}


void SuperPixie::set_color(CRGB color) {
	set_color(color, color);
}


void SuperPixie::set_color(CRGB color_a, CRGB color_b) {
	uint8_t color_data[6] = { color_a.r, color_a.g, color_a.b, color_b.r, color_b.g, color_b.b };
	send_packet(ADDRESS_BROADCAST, COM_SET_COLORS, color_data, 6, false);
}


void SuperPixie::set_gradient_type(gradient_type_t type){
	uint8_t gradient_data[1] = { type };
	send_packet(ADDRESS_BROADCAST, COM_SET_GRADIENT_TYPE, gradient_data, 1, false);
}


void SuperPixie::clear(){
	
}


void SuperPixie::hold(){
	send_packet(ADDRESS_BROADCAST, COM_HOLD_IMAGE, NULL_DATA, 0, false);
}


void SuperPixie::show(){
	send_packet(ADDRESS_BROADCAST, COM_SHOW, NULL_DATA, 0, false);
}


void SuperPixie::wait(){
	static uint32_t t_next_check = micros();
	waiting = true;
	while(waiting == true){
		uint32_t t_now = micros();
		
		if(t_now >= t_next_check){
			t_next_check += 20000;
			
			send_packet(0, COM_GET_READY_STATUS, NULL_DATA, 0, false);
		}
		else{
			consume_serial();
		}
	}
}