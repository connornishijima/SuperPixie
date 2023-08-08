bool debug_mode = false;

// Holds incoming data
uint8_t packet_buffer[256] = { 0 };
uint8_t packet_buffer_index = 0;
bool packet_started = false;
bool packet_got_header = false;
int16_t packet_read_counter = 0;

// Holds incoming data
uint8_t return_packet_buffer[256] = { 0 };
uint8_t return_packet_buffer_index = 0;
bool return_packet_started = false;
bool return_packet_got_header = false;
int16_t return_packet_read_counter = 0;

uint8_t sync_buffer[4] = { 0 };
uint8_t return_sync_buffer[4] = { 0 };

// Stores node configuration
struct conf {
  uint8_t  LOCAL_ADDRESS;
  bool     PROPAGATION;
  uint32_t BAUD;
  uint8_t  DEBUG_LED;
  CRGB     DISPLAY_COLOR_A;
  CRGB     DISPLAY_COLOR_B;
  uint8_t  GRADIENT_TYPE;
  float    CHARACTER_SCALE;
  uint8_t  TRANSITION_TYPE;
  uint16_t TRANSITION_DURATION_MS;
  float    FRAME_BLENDING;

  uint16_t TOUCH_VAL;
  uint16_t TOUCH_THRESHOLD;
  bool     TOUCH_ACTIVE;

  float    FPS;

  CRGB     FX_COLOR;
  float    FX_OPACITY;
  float    FX_BLUR;

  character CURRENT_CHARACTER;
  character NEW_CHARACTER;

  bool FORCE_TRANSITION;

  uint8_t CHAIN_LENGTH;

  uint16_t SCROLL_TIME_MS;
  uint16_t SCROLL_HOLD_TIME_MS;

  bool READY;

  float MASTER_OPACITY;
};
conf CONFIG;

bool mask_needs_update = false;
bool freeze_image = false;

bool transition_complete = true;
float transition_progress = 0.0;
uint32_t transition_start = 0;
uint32_t transition_end = 0;

uint32_t current_frame_duration_us;

bool touch_active = false;

char character_queue[256] = { 0 };
uint8_t charcter_queue_index = 0;

void debug(uint8_t val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(uint16_t val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(uint32_t val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(int8_t val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(int16_t val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(int32_t val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(float val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(char val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debug(char* val){
  if(debug_mode == true){
    Serial.print(val);
  }
}

void debugln(uint8_t val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(uint16_t val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(uint32_t val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(int8_t val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debulng(int16_t val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(int32_t val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(float val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(char val){
  if(debug_mode == true){
    Serial.println(val);
  }
}

void debugln(char* val){
  if(debug_mode == true){
    Serial.println(val);
  }
}
