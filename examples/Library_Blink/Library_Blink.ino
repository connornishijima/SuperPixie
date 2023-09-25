// Import the SuperPixie Arduino library
#include "SuperPixie.h"
SuperPixie pix; // Call it "pix"

// The GPIO pins connected to the DATA_A and DATA_B
// pins of your first SuperPixie
#define DATA_A_PIN ( 4 ) // Outgoing data
#define DATA_B_PIN ( 5 ) // Incoming data

// Stores the value that we've counted to
uint32_t count = 0;

// Set up the SuperPixie chain once
void setup() {  
  // Discover connected displays
  pix.begin( DATA_A_PIN, DATA_B_PIN );

  // Set transition type
  pix.set_transition_type( TRANSITION_PUSH_DOWN );
}

// Count up forever
void loop() {
  // Clear the display buffer
  pix.clear();

  // Walk the display color through the rainbow
  pix.set_color( CHSV( count % 256, 255, 255 ) );

  // Print the current count in the specified color
  pix.print( count );

  // Send the updated image data to the displays
  pix.show( true );

  // Count up
  count++;
}
