#include "debug_digit_polygons.h"

void draw_poly_stroke(float vertices[][2], uint16_t num_vertices, float x_offset, float y_offset, float x_scale, float y_scale, float angle_deg, float opacity) {
  if (x_offset > -7.0 && x_offset < 7.0) {
    if (y_offset > -15.0 && y_offset < 15.0) {
      y_scale *= -1.0;
      y_offset *= -1.0;

      // Line thickness
      float stroke_width = 1.0;                     // Alter this
      float stroke_width_inv = 1.0 / stroke_width;  // Don't alter this

      angle_deg = fmod(angle_deg, 360.0);

      // Clear temp mask
      memset(temp_mask, 0, sizeof(float) * (LEDS_X * LEDS_Y));

      // How wide of a neighborhood each checked position should have
      int16_t search_width = 1;

      // Used to store bounding box of final shape
      float min_x = LEDS_X;  // Will shrink to polygon bounds later in this function
      float min_y = LEDS_Y;
      float max_x = -LEDS_X;
      float max_y = -LEDS_Y;

      // Iterate over all vertices of the polygon
      for (uint16_t vert = 0; vert < num_vertices - 1; vert++) {
        float vert_x1 = vertices[vert][0] * x_scale;
        float vert_y1 = vertices[vert][1] * y_scale;
        float vert_x2 = vertices[vert + 1][0] * x_scale;
        float vert_y2 = vertices[vert + 1][1] * y_scale;

        float rotated_x1, rotated_y1, rotated_x2, rotated_y2;
        if (angle_deg == 0.0000) {
          // Preserve verts if no rotation needed
          rotated_x1 = vert_x1;
          rotated_y1 = vert_y1;
          rotated_x2 = vert_x2;
          rotated_y2 = vert_y2;
        } else {
          // Convert angle from degrees to radians
          float angle_rad = angle_deg * M_PI / 180.0;
          //angle_rad *= -1.0;

          // Rotate verts (More computationally expensive)
          rotated_x1 = vert_x1 * cos(angle_rad) - vert_y1 * sin(angle_rad);
          rotated_y1 = vert_x1 * sin(angle_rad) + vert_y1 * cos(angle_rad);
          rotated_x2 = vert_x2 * cos(angle_rad) - vert_y2 * sin(angle_rad);
          rotated_y2 = vert_x2 * sin(angle_rad) + vert_y2 * cos(angle_rad);
        }

        // Line start coord
        float x_start = rotated_x1 + x_offset + 3;
        float y_start = rotated_y1 + y_offset + 7;

        // Line end coord
        float x_end = rotated_x2 + x_offset + 3;
        float y_end = rotated_y2 + y_offset + 7;

        // Update polygon bounding box
        if (x_start < min_x) {
          min_x = x_start;
        }
        if (x_end < min_x) {
          min_x = x_end;
        }
        if (x_start > max_x) {
          max_x = x_start;
        }
        if (x_end > max_x) {
          max_x = x_end;
        }

        // Y axis too
        if (y_start < min_y) {
          min_y = y_start;
        }
        if (y_end < min_y) {
          min_y = y_end;
        }
        if (y_start > max_y) {
          max_y = y_start;
        }
        if (y_end > max_y) {
          max_y = y_end;
        }

        // Get exact length of line segment
        float vert_x_diff = x_end - x_start;
        float vert_y_diff = y_end - y_start;
        float line_segment_length = sqrt((vert_x_diff * vert_x_diff) + (vert_y_diff * vert_y_diff));

        // Convert line length to integer number of iterations drawing will take, with at least one step per line
        // This way, a line that stretches 7 pixels in the x or y axis will never have less than 8 drawing steps to avoid gaps while avoiding redundant work
        uint16_t num_steps = line_segment_length + 1;
        if (num_steps < 1) {
          num_steps = 1;
        }

        // Amount to increment along the line on each step
        float step_size = 1.0 / num_steps;

        // Iterate over length of line segment for num_steps
        float progress = 0.0;
        for (uint16_t step = 0; step <= num_steps; step++) {
          float brightness_multiplier = 1.0;

          // Half brightness at positions where line-segments meet,
          // except for the beginning and end of the shape
          if (num_steps > 1) {
            if (step == 0) {
              if (vert != 0) {
                brightness_multiplier = 0.5;
              }
            } else if (step == num_steps) {
              if (vert != num_vertices - 2) {
                brightness_multiplier = 0.5;
              }
            }
          }

          //brightness_multiplier *= num_steps;

          // Get exact intermediate position along line
          float x_step_pos = x_start * (1.0 - progress) + x_end * progress;
          float y_step_pos = y_start * (1.0 - progress) + y_end * progress;

          // Clear search markers
          memset(searched, false, sizeof(bool) * (LEDS_X * LEDS_Y));

          // Evaluate pixel neighborhood of current point to render step of line
          for (int16_t x_search = search_width * -1; x_search <= search_width; x_search++) {
            for (int16_t y_search = search_width * -1; y_search <= search_width; y_search++) {
              // Integer pixel position to check
              int16_t x_pos_current = x_step_pos + x_search;
              int16_t y_pos_current = y_step_pos + y_search;

              if (x_pos_current >= 0 && x_pos_current < LEDS_X) {
                if (y_pos_current >= 0 && y_pos_current < LEDS_Y) {

                  // If we haven't search this pixel yet
                  if (searched[y_pos_current][x_pos_current] == false) {
                    searched[y_pos_current][x_pos_current] = true;

                    // Calculate distance between currently searched pixel and exact position of line step point
                    float x_diff = fabs(x_pos_current - x_step_pos);
                    float y_diff = fabs(y_pos_current - y_step_pos);
                    float distance_to_line = sqrt((x_diff * x_diff) + (y_diff * y_diff));

                    // Convert distance to pixel mask brightness if close enough
                    if (distance_to_line < stroke_width) {
                      float brightness = 1.0 - (distance_to_line * stroke_width_inv);

                      brightness = (brightness*0.5)+((brightness*brightness)*0.5);

                      brightness *= brightness_multiplier;

                      // Line segments shorter than 1px should proportionally contribute less to the raster
                      if (line_segment_length < stroke_width) {
                        brightness *= (line_segment_length * stroke_width_inv);
                      }

                      // Apply added brightness to mask, saturating at 1.0 (white)
                      temp_mask[y_pos_current][x_pos_current] += brightness;
                      if (temp_mask[y_pos_current][x_pos_current] > 1.0) {
                        temp_mask[y_pos_current][x_pos_current] = 1.0;
                      }
                    }
                  }
                }
              }
            }
          }

          // Move one step forward along the line
          progress += step_size;
        }
      }

      // Apply search width padding to bounding box
      min_x -= search_width;
      max_x += search_width;
      min_y -= search_width;
      max_y += search_width;

      // Double check that we're clipped to the visible screen area
      if (min_x < 0) {
        min_x = 0;
      } else if (min_x > LEDS_X - 1) {
        min_x = LEDS_X - 1;
      }
      if (max_x < 0) {
        max_x = 0;
      } else if (max_x > LEDS_X - 1) {
        max_x = LEDS_X - 1;
      }

      if (min_y < 0) {
        min_y = 0;
      } else if (min_y > LEDS_Y - 1) {
        min_y = LEDS_Y - 1;
      }
      if (max_y < 0) {
        max_y = 0;
      } else if (max_y > LEDS_Y - 1) {
        max_y = LEDS_Y - 1;
      }

      // Use potentially smaller bounding box to write final shape more efficiently to the pixel mask
      for (int16_t x = min_x; x <= max_x; x++) {
        for (int16_t y = min_y; y <= max_y; y++) {
          // If not fully black
          if (temp_mask[y][x] > 0.0) {
            // Write out final pixels to the mask with proper opacity now applied
            mask[y][x] += temp_mask[y][x] * (opacity);

            // Saturate at 1.0 (white)
            if (mask[y][x] > 1.0) {
              mask[y][x] = 1.0;
            }
          }
        }
      }
    }
  }
}

void parse_vector_from_bytes(uint8_t* input_data, int16_t offset, float pos_x, float pos_y, float scale_x, float scale_y, float rotation, float opacity){
  const uint8_t header_length = 6;

  int16_t num_bytes = (input_data[offset+0] << 8) + input_data[offset+1];
  int16_t num_verts = (input_data[offset+2] << 8) + input_data[offset+3];
  int16_t num_lines = (input_data[offset+4] << 8) + input_data[offset+5];

  /*
  Serial.print("NUM_BYTES: ");
  Serial.println(num_bytes);

  Serial.print("NUM_VERTS: ");
  Serial.println(num_verts);

  Serial.print("NUM_LINES: ");
  Serial.println(num_lines);
  */

  uint16_t lines_start_offset = header_length + num_verts*4;

  for(int16_t i = lines_start_offset; i < num_bytes; i+=4){
    int16_t line_start = (input_data[offset+i+0] << 8) + input_data[offset+i+1];
    int16_t line_end   = (input_data[offset+i+2] << 8) + input_data[offset+i+3];

    int16_t line_start_vertex_offset = header_length + (4*line_start);
    int16_t start_coord_x_int = (input_data[offset+line_start_vertex_offset + 0] << 8) + input_data[offset+line_start_vertex_offset + 1];
    int16_t start_coord_y_int = (input_data[offset+line_start_vertex_offset + 2] << 8) + input_data[offset+line_start_vertex_offset + 3];

    int16_t line_end_vertex_offset = header_length + (4*line_end);
    int16_t end_coord_x_int = (input_data[offset+line_end_vertex_offset + 0] << 8) + input_data[offset+line_end_vertex_offset + 1];
    int16_t end_coord_y_int = (input_data[offset+line_end_vertex_offset + 2] << 8) + input_data[offset+line_end_vertex_offset + 3];

    float start_coord_x = start_coord_x_int/100.0;
    float start_coord_y = start_coord_y_int/100.0;

    float end_coord_x = end_coord_x_int/100.0;
    float end_coord_y = end_coord_y_int/100.0;

    float verts[2][2] = {
      {start_coord_x, start_coord_y},
      {end_coord_x, end_coord_y},
    };

    draw_poly_stroke(verts, 2, pos_x, pos_y, scale_x, scale_y, rotation, opacity);
  }
}

void draw_character(character CHAR, float character_opacity) {
  float pos_x = CHAR.pos_x;
  float pos_y = CHAR.pos_y;
  float scale_x = CHAR.scale_x * CONFIG.CHARACTER_SCALE;
  float scale_y = CHAR.scale_y * CONFIG.CHARACTER_SCALE;
  float rotation = CHAR.rotation;
  float opacity = CHAR.opacity * CONFIG.MASTER_OPACITY * character_opacity;

  char c = CHAR.character;
  int32_t address = lookup_ascii_address(c);

  if(address == -1){
    parse_vector_from_bytes(ascii_font, 0, 0, 0, 1.0, 1.0, 0.0, 0.000001);
  }
  else{
    parse_vector_from_bytes(ascii_font, address, pos_x, pos_y, scale_x, scale_y, rotation, opacity);
  }
}