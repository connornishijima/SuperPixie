// Pull a specific byte from a 16-bit integer
uint8_t get_byte_from_uint16(uint16_t input, uint8_t byte_number) {
  uint8_t byte = uint32_t(input >> (byte_number << 3));
  return byte;
}

// Pull a specific byte from a 32-bit integer
uint8_t get_byte_from_uint32(uint32_t input, uint8_t byte_number) {
  uint8_t byte = uint32_t(input >> (byte_number << 3));
  return byte;
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