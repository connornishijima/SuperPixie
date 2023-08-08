// Pull a specific byte from a 16-bit integer
uint8_t get_byte_from_uint16(uint16_t input, uint8_t byte_number) {
  uint8_t byte = (input >> (byte_number << 3)) & 0xFF;
  return byte;
}

// Pull a specific byte from a 32-bit integer
uint8_t get_byte_from_uint32(uint32_t input, uint8_t byte_number) {
  uint8_t byte = (input >> (byte_number << 3)) & 0xFF;
  return byte;
}