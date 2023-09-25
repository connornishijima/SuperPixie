inline uint8_t get_byte_from_16_bit(uint16_t input, uint8_t byte_half) {
  uint8_t input_high = input >> 8;
  uint8_t input_low = (input << 8) >> 8;

  if (byte_half == HIGH) {
    return input_high;
  } else if (byte_half == LOW) {
    return input_low;
  }

  // Normally never reached
  return 0;
}

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


void integer_to_ascii(uint32_t input_integer, char* output_array) {
  // Restrict the integer to the range of 0 to 999
  input_integer = input_integer % 1000;

  // Extract individual digits from the restricted-range integer
  uint32_t hundreds = input_integer / 100;
  uint32_t tens = (input_integer / 10) % 10;
  uint32_t ones = input_integer % 10;

  // Convert digits to ASCII characters and store them in the output array
  output_array[0] = '0' + hundreds; // Convert the digit to its ASCII equivalent
  output_array[1] = '0' + tens;
  output_array[2] = '0' + ones;

  // Null-terminate the array to make it a valid C-style string
  output_array[3] = '\0';
}







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