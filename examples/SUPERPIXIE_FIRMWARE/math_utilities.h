#include <math.h>

// #############################################################################################
// Calculate the shortest distance between a point and a line segment
inline float shortest_distance_to_segment(float px, float py, float x1, float y1, float x2, float y2) {
  float dx = x2 - x1, dy = y2 - y1;
  float dpx = px - x1, dpy = py - y1;
  float t, projection_x, projection_y;
  
  // Avoid the division and directly compare with zero to handle the degenerate case
  if ((dx == 0) && (dy == 0)) {
    return (dpx * dpx + dpy * dpy);
  }

  // Directly compute the projection scalar (t) without function calls or extra variables
  t = (dpx * dx + dpy * dy) / (dx * dx + dy * dy);

  // Min/max clipping for t in a single line
  t = t < 0 ? 0 : (t > 1 ? 1 : t);

  // Compute the projection point directly in the distance calculation
  projection_x = x1 + t * dx - px;
  projection_y = y1 + t * dy - py;

  // Directly return the distance avoiding extra variables
  return (projection_x * projection_x + projection_y * projection_y);
}

// Function to check if a point (x, y) is inside a bounding box (x1, y1, x2, y2)
inline bool is_point_in_bounding_box(float x, float y, float x1, float y1, float x2, float y2) {
  float search_range = 1;

  // Determine the minimum and maximum values for x and y coordinates
  float x_min = (x1 < x2) ? x1 : x2;
  float x_max = (x1 > x2) ? x1 : x2;
  float y_min = (y1 < y2) ? y1 : y2;
  float y_max = (y1 > y2) ? y1 : y2;

  x_min -= search_range;
  y_min -= search_range;

  x_max += search_range;
  y_max += search_range;

  // Using a single conditional statement to test the conditions
  // This allows the compiler to optimize the code as much as possible
  return (x >= x_min && x <= x_max && y >= y_min && y <= y_max);
}
// #############################################################################################


// #############################################################################################
// Interpolate between two CRGBF colors
CRGBF interpolate_CRGBF( CRGBF color_a, CRGBF color_b, float blend ){
  CRGBF color_out;
  color_out.r = (color_a.r * (1.0-blend)) + (color_b.r * (blend));
  color_out.g = (color_a.g * (1.0-blend)) + (color_b.g * (blend));
  color_out.b = (color_a.b * (1.0-blend)) + (color_b.b * (blend));

  return color_out;
}
// #############################################################################################


// #############################################################################################
// Interpolate between two floating point values
float interpolate_float( float value_a, float value_b, float blend ){
  return (value_a * (1.0-blend)) + (value_b * (blend));
}
// #############################################################################################


// #############################################################################################
// Add to a float value, saturating at 1.0
float add_clipped_float(float a, float b){
  a += b;
  if( a > 1.0 ){ a = 1.0; }

  return a;
}
// #############################################################################################


// #############################################################################################
// Add two CRGBF colors, clipping at 1.0
CRGBF add_clipped_CRGBF(CRGBF col_a, CRGBF col_b){
  CRGBF output = {
    add_clipped_float( col_a.r, col_b.r ),
    add_clipped_float( col_a.g, col_b.g ),
    add_clipped_float( col_a.b, col_b.b )
  };

  return output;
}
// #############################################################################################


// #############################################################################################
// Waveshape a sawtooth value to a triangle value, useful for things like TRANSITION_SHRINK
// where a value goes up and down again as a counter increases
float saw_to_tri(float sample) {
  return (sample < 0.5) ? (sample * 2) : (2 * (1.0 - sample));
}
// #############################################################################################


// #############################################################################################
// Waveshape a sawtooth value to an s-curve value, with smooth acceleration and deceleration
float saw_to_s_curve(float sample) {
  return 1.0 - (1.0 / (1.0 + exp(-10.0 * (sample - 0.5))));
}
// #############################################################################################


// #############################################################################################
// Restrict a float to the 0.0-1.0 range
float clip_float(float input){
  float output = input;
  
  if(output < 0.0){
    output = 0.0;
  }
  else if(output > 1.0){
    output = 1.0;
  }

  return output;
}
// #############################################################################################

float interpolate_curve(float input_linear, uint8_t interpolation_type){
  input_linear = clip_float(input_linear);
  if(interpolation_type == LINEAR){
    return input_linear;
  }
  else if(interpolation_type == EASE_IN){
    return input_linear * input_linear;
  }
  else if(interpolation_type == EASE_OUT){
    return sqrt(input_linear);
  }
  else if(interpolation_type == EASE_IN_SOFT){
    return input_linear * input_linear * input_linear;
  }
  else if(interpolation_type == EASE_OUT_SOFT){
    return sqrt(sqrt(input_linear));
  }
  else if(interpolation_type == S_CURVE){
    float a_val = input_linear * input_linear;
    float b_val = sqrt(input_linear);
    return ((a_val * (1.0 - input_linear)) + (b_val * (input_linear)));
  }
  else if(interpolation_type == S_CURVE_SOFT){
    float a_val = input_linear * input_linear * input_linear;
    float b_val = sqrt(sqrt(input_linear));
    return ((a_val * (1.0 - input_linear)) + (b_val * (input_linear)));
  }
  else{
    return 0.0F;
  }
}