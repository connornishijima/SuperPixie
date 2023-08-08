/*! 
 * @file SuperPixie.h
 *
 * Designed specifically to work with SuperPixie:
 * ----> www.lixielabs.com/superpixie
 *
 * Last Updated by Connor Nishijima on 8/7/23
 */

#ifndef superpixie_h
#define superpixie_h

#include "Arduino.h"
#include "FastLED.h"

// Used to signify start and end of packets
#define SYNC_PATTERN_1 0b11001100
#define SYNC_PATTERN_2 0b00110011
#define SYNC_PATTERN_3 0b10101010
#define SYNC_PATTERN_4 0b01010101

// Reserved addresses
#define ADDRESS_BROADCAST 255
#define ADDRESS_NULL      254
#define ADDRESS_COMMANDER 253

// UART objects
#define chain Serial

#define ACK_TIMEOUT_MS 100

typedef enum {
  TRANSITION_INSTANT,
  TRANSITION_FADE,
  TRANSITION_FADE_OUT,
  TRANSITION_FLIP_HORIZONTAL,
  TRANSITION_FLIP_VERTICAL,
  TRANSITION_SPIN_LEFT,
  TRANSITION_SPIN_RIGHT,
  TRANSITION_SPIN_HALF_LEFT,
  TRANSITION_SPIN_HALF_RIGHT,
  TRANSITION_SHRINK,
  TRANSITION_PUSH_UP,
  TRANSITION_PUSH_DOWN,
  TRANSITION_PUSH_LEFT,
  TRANSITION_PUSH_RIGHT,

  NUM_TRANSITIONS
} transition_type_t;

typedef enum {
  GRADIENT_NONE,
  GRADIENT_HORIZONTAL,
  GRADIENT_VERTICAL,
  GRADIENT_BRIGHTNESS,
} gradient_type_t;

// Possible UART commands
typedef enum {
  COM_NULL,
  COM_ACK,
  COM_PROBE,
  COM_BLINK,
  COM_RESET_CHAIN,
  COM_ENABLE_PROPAGATION,
  COM_ASSIGN_ADDRESS,
  COM_SET_BAUD,
  COM_SET_DEBUG_LED,
  COM_SET_DEBUG_DIGIT,
  COM_SET_BRIGHTNESS,
  COM_SET_COLORS,
  COM_SHOW,
  COM_SEND_STRING,
  COM_ANNOUNCE,
  COM_SET_TRANSITION_TYPE,
  COM_SET_TRANSITION_DURATION_MS,
  COM_SET_FRAME_BLENDING,
  COM_SET_FX_COLOR,
  COM_SET_FX_OPACITY,
  COM_SET_FX_BLUR,
  COM_SET_GRADIENT_TYPE,
  COM_TOUCH_EVENT,
  COM_FORCE_TRANSITION,
  COM_SCROLL_STRING,
  COM_SET_CHAIN_LENGTH,
  COM_SET_SCROLL_SPEED,
  COM_GET_READY_STATUS,
  COM_READY_STATUS,

  NUM_COMMANDS
} command_t;

/*! ############################################################################
    @brief
    This is the software documentation for using SuperPixie functions on
    Arduino! **For full example usage, see File > Examples > SuperPixie
	inside Arduino!**.
*///............................................................................
class SuperPixie{
	public:
		/** @brief Construct a SuperPixie class object */
		SuperPixie(); 
		
		/*+-- Functions - Setup ------------------------------------------------------------*/ 
		/*|*/ void begin( uint32_t chain_baud );
		/*+-- Functions - print(  ) --------------------------------------------------------*/ 
		/*|*/ void set_string( char* string );
		/*|*/ void scroll_string( char* string );
		/*|*/ uint16_t send_packet( uint8_t dest_address, command_t packet_type, uint8_t* data, uint8_t data_length_in_bytes, bool get_ack );
		/*|*/ bool send_packet_and_await_ack( uint8_t dest_address, command_t packet_type, uint8_t* data, uint8_t data_length_in_bytes );

		/*+-- Functions - Updating the mask/LEDs -------------------------------------------*/
		/*|*/ void set_brightness( float brightness );
		/*|*/ void set_scroll_speed( uint16_t scroll_time_ms, uint16_t hold_time_ms );
		/*|*/ void set_transition_type( transition_type_t type );
		/*|*/ void set_transition_duration_ms( uint16_t duration_ms );
		/*|*/ void set_frame_blending( float blend_val );
		/*|*/ void set_color( CRGB color );
		/*|*/ void set_color( CRGB color_a, CRGB color_b );
		/*|*/ void set_gradient_type( gradient_type_t type );
		/*|*/ void clear();
		/*|*/ void show();
		/*|*/ void wait();
		/*+-- Functions - Debug ------------------------------------------------------------*/

		/*+---------------------------------------------------------------------------------*/

		// Variables -------------------------------------------------------------------------
		
		// How many nodes are detected in the chain
		uint16_t chain_length = 0;
		
		// Current packet ID (sequential)
		uint16_t packet_id = 0;

	private:
		// Functions ----------------------------------
		void init_system(uint32_t chain_baud);
		void init_uart(uint32_t chain_baud);
		void discover_nodes();
		bool await_ack(uint32_t packet_id);
		void feed_byte_into_sync_buffer(uint8_t incoming_byte);
		void feed_byte_into_packet_buffer(uint8_t incoming_byte);
		void init_packet();
		void parse_packet();
		void execute_packet(uint16_t packet_id, uint8_t origin_address, uint8_t first_hop_address, command_t packet_type, bool needs_ack, uint8_t data_length_in_bytes, uint8_t data_start_position);
		void consume_serial();
		void announce_new_chain_length(uint8_t length);
		
		uint8_t NULL_DATA[1] = {0};
		
		// Holds incoming data
		uint8_t packet_buffer[256] = { 0 };
		
		uint8_t packet_buffer_index = 0;
		bool packet_started = false;
		bool packet_got_header = false;
		int16_t packet_read_counter = 0;

		uint8_t sync_buffer[4] = { 0 };

		uint16_t packet_acks[8][2] = { 0 };
		uint8_t packet_acks_index = 0;
		
		bool touch_status = false;
		uint8_t touch_node = 0;
		
		bool waiting = false;
	};

#endif
