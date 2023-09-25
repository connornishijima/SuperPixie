//--------------------------------------------------------------------------------------------------
// Three character_state structs are kept in memory, and work together during display transitions.
// When a display transition occurs, all values in the character_state struct are interpolated from
// the current state to the new one over a period of TRANSITION_MS.
//
// When the transition is complete, current_character_state's binary value inverts, making the new
// character_state the default one.

character_state CHARACTER_STATE[2];
bool character_state_changed = false;
uint8_t current_character_state = 0;
//--------------------------------------------------------------------------------------------------

// Line memory stores up to 128 line segments in two system states, useful for storing the
// current and next character to be drawn, with line_count[] marking the number of lines loaded
// into each half of line_memory.
line line_memory[2][128];
uint16_t line_count[2] = { 0 };


// #############################################################################################
// Using the vector currently defined in the [slot] of line_memory, offset its position, rotation,
// scale and opacity before rastering it to the character mask passed in as draw_mask[][].
void draw_vector_from_line_memory(uint8_t slot, float draw_mask[LEDS_X][LEDS_Y], vec2D pos, vec2D scale, float rotation, float opacity) {
  const float line_segment_width = 1.0;
  const float line_segment_width_squared = line_segment_width * line_segment_width;

  for (uint16_t line = 0; line < line_count[slot]; line++) {
    float line_start_x = line_memory[slot][line].x1;
    float line_start_y = line_memory[slot][line].y1;
    float line_end_x = line_memory[slot][line].x2;
    float line_end_y = line_memory[slot][line].y2;

    if (scale.x != 1.0 || scale.y != 1.0) {
      line_start_x *= scale.x;
      line_end_x *= scale.x;
      line_start_y *= scale.y;
      line_end_y *= scale.y;
    }

    float rotated_x1, rotated_y1, rotated_x2, rotated_y2;
    rotation = fmod(rotation, 360.0);
    if (rotation == 0.0000) {
      // Preserve verts if no rotation needed
      rotated_x1 = line_start_x;
      rotated_y1 = line_start_y;
      rotated_x2 = line_end_x;
      rotated_y2 = line_end_y;
    } else {
      // Convert angle from degrees to radians
      float angle_rad = rotation * M_PI / 180.0;
      //angle_rad *= -1.0;

      // Rotate verts (More computationally expensive)
      rotated_x1 = line_start_x * cos(angle_rad) - line_start_y * sin(angle_rad);
      rotated_y1 = line_start_x * sin(angle_rad) + line_start_y * cos(angle_rad);
      rotated_x2 = line_end_x * cos(angle_rad) - line_end_y * sin(angle_rad);
      rotated_y2 = line_end_x * sin(angle_rad) + line_end_y * cos(angle_rad);
    }

    line_start_x = rotated_x1 + pos.x + 3;
    line_start_y = rotated_y1 + pos.y + 7;
    line_end_x = rotated_x2 + pos.x + 3;
    line_end_y = rotated_y2 + pos.y + 7;

    // Determine the minimum and maximum values for x and y coordinates
    float x_min = (line_start_x < line_end_x) ? line_start_x : line_end_x;
    float x_max = (line_start_x > line_end_x) ? line_start_x : line_end_x;
    float y_min = (line_start_y < line_end_y) ? line_start_y : line_end_y;
    float y_max = (line_start_y > line_end_y) ? line_start_y : line_end_y;

    for (int16_t x = x_min - 1; x <= x_max + 1; x++) {
      for (int16_t y = y_min - 1; y <= y_max + 1; y++) {
        if (x >= 0 && x < LEDS_X) {
          if (y >= 0 && y < LEDS_Y) {
            float dist_squared = shortest_distance_to_segment(
              x, y,
              line_start_x, line_start_y,
              line_end_x, line_end_y);

            if (dist_squared <= line_segment_width_squared) {
              float brightness = (line_segment_width_squared - dist_squared);
              brightness *= brightness;
              brightness *= opacity;
              if (brightness > draw_mask[x][y]) {
                draw_mask[x][y] = brightness;
              }
            }
          }
        }
      }
    }
  }
}
// #############################################################################################


// #############################################################################################
// Load a vector from the compressed format in ascii.h into native floating point values
// cached in line_memory[][] to be used by the rasterizer above
void load_vector_to_line_memory(uint8_t slot, uint8_t* input_data, int16_t offset) {
  const uint8_t header_length = 6;

  int16_t num_bytes = (input_data[offset + 0] << 8) + input_data[offset + 1];
  int16_t num_verts = (input_data[offset + 2] << 8) + input_data[offset + 3];

  uint16_t lines_start_offset = header_length + num_verts * 4;

  for (int16_t i = lines_start_offset; i < num_bytes; i += 4) {
    int16_t line_start = (input_data[offset + i + 0] << 8) + input_data[offset + i + 1];
    int16_t line_end = (input_data[offset + i + 2] << 8) + input_data[offset + i + 3];

    int16_t line_start_vertex_offset = header_length + (4 * line_start);
    int16_t start_coord_x_int = (input_data[offset + line_start_vertex_offset + 0] << 8) + input_data[offset + line_start_vertex_offset + 1];
    int16_t start_coord_y_int = (input_data[offset + line_start_vertex_offset + 2] << 8) + input_data[offset + line_start_vertex_offset + 3];

    int16_t line_end_vertex_offset = header_length + (4 * line_end);
    int16_t end_coord_x_int = (input_data[offset + line_end_vertex_offset + 0] << 8) + input_data[offset + line_end_vertex_offset + 1];
    int16_t end_coord_y_int = (input_data[offset + line_end_vertex_offset + 2] << 8) + input_data[offset + line_end_vertex_offset + 3];

    float start_coord_x = start_coord_x_int / 100.0;
    float start_coord_y = start_coord_y_int / 100.0;

    float end_coord_x = end_coord_x_int / 100.0;
    float end_coord_y = end_coord_y_int / 100.0;

    line_memory[slot][line_count[slot]].x1 = start_coord_x;
    line_memory[slot][line_count[slot]].y1 = start_coord_y;
    line_memory[slot][line_count[slot]].x2 = end_coord_x;
    line_memory[slot][line_count[slot]].y2 = end_coord_y;

    line_count[slot]++;
  }
}
// #############################################################################################


// #############################################################################################
// Queues a new character to be drawn in the opposite of the current character_state being used,
// to be shown after the next transition is triggered
void set_new_character(char character) {
  int32_t address = lookup_ascii_address(character);

  memset(line_memory[!current_character_state], 0, sizeof(line) * 128);
  line_count[!current_character_state] = 0;

  if (address == -1) {
    // Draw nothing if character provided has ascii address out of range
  } else {
    load_vector_to_line_memory(!current_character_state, ascii_font, address);
  }

  CHARACTER_STATE[!current_character_state].OPACITY = 0.0;

  character_state_changed = true;
}
// #############################################################################################


// #############################################################################################
// If a transition is in progress, use a specific function defined in transitions.h to run
// the show based on what transition type SuperPixie is currently configured to use in the
// upcoming system state the transition is interpolating to.
void run_character_transitions() {
  if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_INSTANT) {
    run_instant_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_FADE) {
    run_fade_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_FADE_OUT) {
    run_fade_out_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_SPIN_LEFT) {
    run_spin_left_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_SPIN_RIGHT) {
    run_spin_right_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_SPIN_LEFT_HALF) {
    run_spin_left_half_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_SPIN_RIGHT_HALF) {
    run_spin_right_half_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_PUSH_UP) {
    run_push_up_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_PUSH_DOWN) {
    run_push_down_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_PUSH_LEFT) {
    run_push_left_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_PUSH_RIGHT) {
    run_push_right_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_SHRINK) {
    run_shrink_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_FLIP_HORIZONTAL) {
    run_flip_horizontal_transition();
  }
  else if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_FLIP_VERTICAL) {
    run_flip_vertical_transition();
  }
}
// #############################################################################################


// #############################################################################################
// Draw both character states and the contents of line_memory[][] to the character masks, to 
// be summed up later in the GPU loop
void draw_characters() {
  run_character_transitions();

  memset(character_mask[0], 0, sizeof(float) * LEDS_X * LEDS_Y);
  memset(character_mask[1], 0, sizeof(float) * LEDS_X * LEDS_Y);

  draw_vector_from_line_memory(0, character_mask[0], CHARACTER_STATE[0].POSITION, CHARACTER_STATE[0].SCALE, CHARACTER_STATE[0].ROTATION, CHARACTER_STATE[0].OPACITY);
  draw_vector_from_line_memory(1, character_mask[1], CHARACTER_STATE[1].POSITION, CHARACTER_STATE[1].SCALE, CHARACTER_STATE[1].ROTATION, CHARACTER_STATE[1].OPACITY);

  draw_mask_to_leds(character_mask[0]);
  draw_mask_to_leds(character_mask[1]);
}
// #############################################################################################