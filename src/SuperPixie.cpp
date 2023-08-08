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

void SuperPixie::begin( uint32_t chain_baud ){
	init_system(chain_baud);
}

// Bootup sequence
void SuperPixie::init_system(uint32_t chain_baud){
	init_uart(chain_baud);
	discover_nodes();
}

// Set up UART communication
void SuperPixie::init_uart(uint32_t chain_baud){
	chain.begin(chain_baud);
}

// Auto chain discovery
void SuperPixie::discover_nodes(){
	send_packet(ADDRESS_BROADCAST, COM_RESET_CHAIN, NULL_DATA, 0, false);

	chain_length = 0;
	uint8_t address_to_assign = 0;
	bool solved = false;

	while(solved == false){
		uint8_t data[1] = { address_to_assign };
		if(send_packet_and_await_ack(ADDRESS_NULL, COM_ASSIGN_ADDRESS, data, 1) == true){
			send_packet(address_to_assign, COM_ENABLE_PROPAGATION, NULL_DATA, 0, false);
			address_to_assign += 1;
			chain_length = address_to_assign;
		}
		else{
			//leds[35] = CRGB(0,255,0);
			solved = true;
		}
	}
	
	announce_new_chain_length(chain_length);
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %% FUNCTIONS - WRITE %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// Send command packet to broadcast or a specific address, optionally requiring an ACK
uint16_t SuperPixie::send_packet(uint8_t dest_address, command_t packet_type, uint8_t* data, uint8_t data_length_in_bytes, bool get_ack) {
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

	// Return the ID the packet was assigned
	return packet_id;
}

bool SuperPixie::send_packet_and_await_ack(uint8_t dest_address, command_t packet_type, uint8_t* data, uint8_t data_length_in_bytes) {
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

	// If sent to BROADCAST (all nodes),
	// or specifically intended for this node,
	// then execute the command/data:
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
	else if (packet_type == COM_TOUCH_EVENT) {
		touch_status = packet_buffer[data_start_position + 0];
		touch_node = origin_address;
	}
	else if (packet_type == COM_READY_STATUS) {
		bool ready_status = packet_buffer[data_start_position + 0];
		if(ready_status == true){
			waiting = false;
		}
	}
}

void SuperPixie::announce_new_chain_length(uint8_t length){
	uint8_t data[1] = { length };
	send_packet(ADDRESS_BROADCAST, COM_SET_CHAIN_LENGTH, data, 1, false);
}

void SuperPixie::consume_serial() {
	// Data returning to chain commander
	while (chain.available() > 0) {
		uint8_t byte = chain.read();
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