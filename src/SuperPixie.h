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
#include <SoftwareSerial.h>
#include "FastLED.h"

// ISR libraries
#include <Ticker.h>

// Used to signify start and end of packets
#define SYNC_PATTERN_1 0b11001100
#define SYNC_PATTERN_2 0b00110011
#define SYNC_PATTERN_3 0b10101010
#define SYNC_PATTERN_4 0b01010101

// Reserved addresses
#define ADDRESS_BROADCAST 255
#define ADDRESS_NULL      254
#define ADDRESS_COMMANDER 253

#define DEFAULT_CHAIN_BAUD (38400)
#define DEBUG_BAUD (230400)

#define ACK_TIMEOUT_MS (60)

#define RESET_PULSE_DURATION_MS (10)

//#define debug Serial
#define debug_mode 1

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
  /* 0  */ COM_NULL,
  /* 1  */ COM_ACK,
  /* 2  */ COM_PROBE,
  /* 3  */ COM_BLINK,
  /* 4  */ COM_RESET_CHAIN,
  /* 5  */ COM_ENABLE_PROPAGATION,
  /* 6  */ COM_ASSIGN_ADDRESS,
  /* 7  */ COM_SET_BAUD,
  /* 8  */ COM_SET_DEBUG_LED,
  /* 9  */ COM_SET_DEBUG_DIGIT,
  /* 10 */ COM_SET_BRIGHTNESS,
  /* 11 */ COM_SET_COLORS,
  /* 12 */ COM_SHOW,
  /* 13 */ COM_SEND_STRING,
  /* 14 */ COM_ANNOUNCE,
  /* 15 */ COM_SET_TRANSITION_TYPE,
  /* 16 */ COM_SET_TRANSITION_DURATION_MS,
  /* 17 */ COM_SET_FRAME_BLENDING,
  /* 18 */ COM_SET_FX_COLOR,
  /* 19 */ COM_SET_FX_OPACITY,
  /* 20 */ COM_SET_FX_BLUR,
  /* 21 */ COM_SET_GRADIENT_TYPE,
  /* 22 */ COM_TOUCH_EVENT,
  /* 23 */ COM_FORCE_TRANSITION,
  /* 24 */ COM_SCROLL_STRING,
  /* 25 */ COM_SET_CHAIN_LENGTH,
  /* 26 */ COM_SET_SCROLL_SPEED,
  /* 27 */ COM_GET_READY_STATUS,
  /* 28 */ COM_READY_STATUS,
  /* 29 */ COM_HOLD_IMAGE,
  /* 30 */ COM_GET_TOUCH_STATE,
  /* 31 */ COM_TOUCH_STATE,
  /* 32 */ COM_ENABLE_FAST_MODE,
  /* 33 */ COM_PANIC,
  
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
		/*|*/ void begin(uint8_t data_a_pin, uint8_t data_b_pin, uint32_t baud_rate = DEFAULT_CHAIN_BAUD);
		/*|*/ void set_baud_rate( uint32_t new_baud );
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
		/*|*/ void hold();
		/*|*/ void show();
		/*|*/ void wait();
		/*+-- Functions - Debug ------------------------------------------------------------*/

		int8_t read_touch(uint8_t chain_index);
		void read();

		/*+---------------------------------------------------------------------------------*/

		// Variables -------------------------------------------------------------------------
		
		// How many nodes are detected in the chain
		uint16_t chain_length = 0;
		
		// Current packet ID (sequential)
		uint16_t packet_id = 0;

	private:		
		uint8_t data_a_pin_;
		uint8_t data_b_pin_;
		SoftwareSerial chain;
		
		// Functions ----------------------------------
		void init_system(uint8_t data_a_pin, uint8_t data_b_pin, uint32_t baud_rate);
		void init_uart(uint8_t data_a_pin, uint8_t data_b_pin);
		void discover_nodes();
		void consume_serial();
		bool await_ack(uint32_t packet_id);
		void feed_byte_into_sync_buffer(uint8_t incoming_byte);
		void feed_byte_into_packet_buffer(uint8_t incoming_byte);
		void init_packet();
		void parse_packet();
		void execute_packet(uint16_t packet_id, uint8_t origin_address, uint8_t first_hop_address, command_t packet_type, bool needs_ack, uint8_t data_length_in_bytes, uint8_t data_start_position);
		void announce_new_chain_length(uint8_t length);
		void measure_chain_length();
		void init_serial_isr();
		
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
		
		bool *touch_response;
		int8_t *touch_state;
		
		bool touch_status = false;
		uint8_t touch_node = 0;
		
		bool waiting = false;
		
		uint16_t serial_read_hz = 20;
		
		// ISR variables
		Ticker serial_checker;
		
		bool chain_initialized = false;
	};
	
#endif
