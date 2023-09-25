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
#define PREAMBLE_PATTERN_1 (B10111000)
#define PREAMBLE_PATTERN_2 (B10000111)
#define PREAMBLE_PATTERN_3 (B10101010)
#define PREAMBLE_PATTERN_4 (B10010101)

// Reserved addresses
#define ADDRESS_CHAIN_HEAD (0)
#define ADDRESS_COMMANDER  (253)
#define ADDRESS_NULL       (254)
#define ADDRESS_BROADCAST  (255)

#define DEFAULT_CHAIN_BAUD (9600)
#define DEBUG_BAUD (9600)

#define RESET_PULSE_DURATION_MS (50)

#define SERIAL_READ_HZ (1000)

#define debug_mode 0

#define MAXIMUM_UPDATE_FPS (50)
#define MINIMUM_UPDATE_MICROS (1000000 / MAXIMUM_UPDATE_FPS)

typedef enum {
  TRANSITION_INSTANT,
  TRANSITION_FADE,
  TRANSITION_FADE_OUT,
  TRANSITION_FLIP_HORIZONTAL,
  TRANSITION_FLIP_VERTICAL,
  TRANSITION_SPIN_LEFT,
  TRANSITION_SPIN_RIGHT,
  TRANSITION_SPIN_LEFT_HALF,
  TRANSITION_SPIN_RIGHT_HALF,
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
  GRADIENT_HORIZONTAL_MIRRORED,
  GRADIENT_VERTICAL,
  GRADIENT_VERTICAL_MIRRORED,
  GRADIENT_BRIGHTNESS
} gradient_type_t;

// A list of all possible interpolation types
enum interpolation_types {
  LINEAR,
  EASE_IN,
  EASE_OUT,
  EASE_IN_SOFT,
  EASE_OUT_SOFT,
  S_CURVE,
  S_CURVE_SOFT,
};

// Possible UART commands
typedef enum {
  /* 0  */ COM_TEST,
  /* 1  */ COM_PROBE,
  /* 2  */ COM_PROBE_RESPONSE,
  /* 3  */ COM_ENABLE_PROPAGATION,
  /* 4  */ COM_LENGTH_INQUIRY,
  /* 5  */ COM_LENGTH_RESPONSE,
  /* 6  */ COM_INFORM_CHAIN_LENGTH,
  /* 7  */ COM_SET_BACKLIGHT_COLOR,
  /* 8  */ COM_SET_FRAME_BLENDING,
  /* 9  */ COM_SET_BRIGHTNESS,
  /* 10 */ COM_SHOW,
  /* 11 */ COM_SET_TRANSITION_TYPE,
  /* 12 */ COM_SET_TRANSITION_DURATION_MS,
  /* 13 */ COM_SET_CHARACTER,
  /* 14 */ COM_START_BUS_MODE,
  /* 15 */ COM_END_BUS_MODE,
  /* 16 */ COM_BUS_READY,
  /* 17 */ COM_SET_DEBUG_OVERLAY_OPACITY,
  /* 18 */ COM_SET_DISPLAY_COLORS,
  /* 19 */ COM_SET_GRADIENT_TYPE,
  /* 20 */ COM_GET_FPS,
  /* 21 */ COM_SET_STRING,
  /* 22 */ COM_TRANSITION_COMPLETE,
  /* 23 */ COM_GET_VERSION,
  /* 24 */ COM_TOUCH_EVENT,
  /* 25 */ COM_VERSION_RESPONSE,
  /* 26 */ COM_SET_TRANSITION_INTERPOLATION,
  /* 27 */ COM_SET_TOUCH_GLOW_POSITION,
  /* 28 */ COM_SET_TOUCH_GLOW_COLOR,
  /* 29 */ COM_READ_TOUCH,
  /* 30 */ COM_CALIBRATE_TOUCH,
  /* 31 */ COM_READ_TOUCH_RESPONSE,
  /* 32 */ COM_SET_TOUCH_THRESHOLD,
  /* 33 */ COM_SAVE_STORAGE,
  
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
		/*|*/ void begin(uint8_t data_a_pin, uint8_t data_b_pin);
		/*|*/ void reset_chain();
		/*+-- Functions - print(  ) --------------------------------------------------------*/ 
		/*|*/ void set_string( char* string );
		/*|*/ void set_character( uint8_t new_character, uint8_t destination_address = ADDRESS_BROADCAST );		
		/*+-- Functions - Updating the mask/LEDs -------------------------------------------*/
		/*|*/ uint16_t send_packet(uint8_t command_type, uint8_t destination_address, uint8_t data_length_in_bytes, uint8_t* command_data);

		/*|*/ void set_brightness( float brightness, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_scroll_speed( uint16_t scroll_time_ms, uint16_t hold_time_ms, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_transition_type( transition_type_t type, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_transition_duration_ms( uint16_t duration_ms, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_frame_blending( float blend_val, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_color( CRGB color, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_color( CRGB color_a, CRGB color_b, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_gradient_type( gradient_type_t type, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_backlight_color( CRGB col, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_debug_overlay_opacity( float opacity, uint8_t destination_address = ADDRESS_BROADCAST );
		/*|*/ void set_transition_interpolation( uint8_t interpolation_type );
		/*|*/ void clear();
		/*|*/ void show();
		/*|*/ void wait();
		/*+-- Functions - Debug ------------------------------------------------------------*/

		/*+---------------------------------------------------------------------------------*/

		// Variables -------------------------------------------------------------------------
		
		// How many nodes are detected in the chain
		uint16_t chain_length = 0;

	private:
		uint8_t data_a_pin;
		uint8_t data_b_pin;
		
		SoftwareSerial chain;
		
		bool chain_initialized = false;
		bool bus_ready = false;
		
		bool show_complete = true;
		bool show_called_once = false;
		
		uint8_t NULL_DATA[1] = {0};
		
		// Holds incoming data
		uint8_t sync_buffer[4];
		uint8_t packet_buffer[128];
		uint8_t packet_data[64];
		uint8_t packet_buffer_index = 0;
		bool packet_started = false;
		
		// ISR variables
		Ticker serial_checker;
		
		// Functions ----------------------------------
		void init_system();
		void init_uart();
		
		void start_bus_mode();
		void end_bus_mode();
		
		void send_probe_response(uint8_t origin_address);
		void execute_packet(uint8_t origin_address, uint16_t packet_id, uint8_t command_type, uint8_t data_length_in_bytes);
		void parse_packet();
		void finalize_packet();
		void init_packet();
		void feed_byte_into_sync_buffer(uint8_t incoming_byte);
		void feed_byte_into_packet_buffer(uint8_t incoming_byte);
		void parse_incoming_data();
		void receive_chain_data();
		
		void init_serial_isr();
	};
	
#endif
