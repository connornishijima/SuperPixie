//----------------------------------------------------------------
// These are defined later in the scope in SuperPixie_Firmware.ino
extern void loop_cpu(void *parameter);
extern void loop_gpu(void *parameter);

extern character_state CHARACTER_STATE[2];
extern bool character_state_changed;
extern uint8_t current_character_state;

extern void init_storage();

extern void init_chain_uart();
extern void send_touch_event();
//----------------------------------------------------------------

// Timekeeping on the GPU core
uint32_t time_us_now = 0;
uint32_t time_ms_now = 0;

// Halts the GPU core during sensitive changes
bool freeze_led_image = false;

// Set to true every time that a transition completes, and is
// read/reset every time check_transition_completion() is called.
// every call to trigger_transition() forces this to false again
bool transition_complete_flag = false;

//----------------------------------------------------------------------------------------------
// Three system_state structs are kept in memory, and work together during display transitions.
// When a display transition occurs, all values in the system_state struct are interpolated from
// the current state to the new one over a period of TRANSITION_MS.
//
// When the transition is complete, current_system_state's binary value inverts, making the new
// system_state the default one. The output of the transition and final settings are written to
// the lone SYSTEM_STATE struct for reading throughout the rest of the system.
system_state SYSTEM_STATE;
system_state SYSTEM_STATE_INTERNAL[2];
bool system_state_changed = false;
uint8_t current_system_state = 0;
float system_state_transition_progress = 1.0;
float system_state_transition_progress_shaped = 1.0;
uint32_t transition_start_ms = 0;
uint32_t transition_end_ms = 0;
bool transition_running = false;
//----------------------------------------------------------------------------------------------

TaskHandle_t cpu_task = NULL;
TaskHandle_t gpu_task = NULL;

uint32_t last_gpu_check_in = 0;

bool system_ready = false;

// #############################################################################################
// Init dual core system on startup
void init_cores() {
  //-------------------------------------
  // Initialize "GPU" core
  xTaskCreatePinnedToCore(
    loop_gpu,      // Task function
    "Loop (GPU)",  // Task name
    8000,          // Stack size
    NULL,          // Task parameter
    1,             // Priority
    &gpu_task,     // Task handle
    1              // Uses Core 0
  );
  //-------------------------------------

  //-------------------------------------
  // Initialize "CPU" core
  xTaskCreatePinnedToCore(
    loop_cpu,      // Task function
    "Loop (CPU)",  // Task name
    8000,          // Stack size
    NULL,          // Task parameter
    1,             // Priority
    &cpu_task,     // Task handle
    0              // Uses Core 1
  );
  //-------------------------------------
}
// #############################################################################################


// #############################################################################################
// Initialize the system_states to default values on boot
void init_system_states() {
  memcpy(&SYSTEM_STATE, &SYSTEM_STATE_DEFAULTS, sizeof(system_state));

  memcpy(&SYSTEM_STATE_INTERNAL[0], &SYSTEM_STATE, sizeof(system_state));
  memcpy(&SYSTEM_STATE_INTERNAL[1], &SYSTEM_STATE, sizeof(system_state));

  system_state_changed = false;
  current_system_state = 0;
  system_state_transition_progress = 1.0;
  system_state_transition_progress_shaped = 1.0;
  transition_start_ms = 0;
  transition_end_ms = 0;
}
// #############################################################################################


// #############################################################################################
// Initialize the character_states to default values on boot
void init_character_states() {
  memcpy(&CHARACTER_STATE[0], &CHARACTER_STATE_DEFAULTS, sizeof(character_state));
  memcpy(&CHARACTER_STATE[1], &CHARACTER_STATE_DEFAULTS, sizeof(character_state));

  character_state_changed = false;
  current_character_state = 0;
}
// #############################################################################################


// #############################################################################################
// Very first function, kicks off initializations for various hardware peripherals
void init_system() {
  //----------------------------
  // Initialize UART chain
  init_chain_uart();
  //----------------------------
  
  //------------------------------------
  // Initialize the system state structs
  init_storage();
  init_system_states();
  init_character_states();
  //------------------------------------

  init_fs();
}
// #############################################################################################


// #############################################################################################
// Interpolate the system states between old and new, swapping them when transitions are complete
void run_system_transition() {
  freeze_led_image = true;

  if (fade_in_complete == false) {
    GLOBAL_LED_BRIGHTNESS += 0.01;

    if (GLOBAL_LED_BRIGHTNESS >= 1.0) {  // If target met
      GLOBAL_LED_BRIGHTNESS = 1.0;
      fade_in_complete = true;
    } else if (debug_led_opacity > 0.0) {  // If fade in should be instant for debugging
      GLOBAL_LED_BRIGHTNESS = 1.0;
      fade_in_complete = true;
    }
  }

  /*
  Serial.print("{ ");
  Serial.print(CHARACTER_STATE[current_character_state].OPACITY);
  Serial.print(", ");
  Serial.print(CHARACTER_STATE[!current_character_state].OPACITY);
  Serial.println(" }");
  */

  if (transition_running == true) {
    uint32_t time_elapsed_ms = time_ms_now - transition_start_ms;
    uint32_t transition_duration_ms = SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_DURATION_MS;
    system_state_transition_progress = clip_float(float(time_elapsed_ms) / float(transition_duration_ms));

    if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_INSTANT) {
      system_state_transition_progress = 1.0;
      //here_flag = true;
    }

    system_state_transition_progress_shaped = interpolate_curve(system_state_transition_progress, SYSTEM_STATE.TRANSITION_INTERPOLATION);

    SYSTEM_STATE.BRIGHTNESS = interpolate_float(
      SYSTEM_STATE_INTERNAL[current_system_state].BRIGHTNESS,
      SYSTEM_STATE_INTERNAL[!current_system_state].BRIGHTNESS,
      system_state_transition_progress_shaped);

    SYSTEM_STATE.BACKLIGHT_COLOR = interpolate_CRGBF(
      SYSTEM_STATE_INTERNAL[current_system_state].BACKLIGHT_COLOR,
      SYSTEM_STATE_INTERNAL[!current_system_state].BACKLIGHT_COLOR,
      system_state_transition_progress_shaped);

    SYSTEM_STATE.BACKLIGHT_BRIGHTNESS = interpolate_float(
      SYSTEM_STATE_INTERNAL[current_system_state].BACKLIGHT_BRIGHTNESS,
      SYSTEM_STATE_INTERNAL[!current_system_state].BACKLIGHT_BRIGHTNESS,
      system_state_transition_progress_shaped);

    SYSTEM_STATE.DISPLAY_COLOR_A = interpolate_CRGBF(
      SYSTEM_STATE_INTERNAL[current_system_state].DISPLAY_COLOR_A,
      SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_COLOR_A,
      system_state_transition_progress_shaped);

    SYSTEM_STATE.DISPLAY_COLOR_B = interpolate_CRGBF(
      SYSTEM_STATE_INTERNAL[current_system_state].DISPLAY_COLOR_B,
      SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_COLOR_B,
      system_state_transition_progress_shaped);

    SYSTEM_STATE.DISPLAY_BACKGROUND_COLOR_A = interpolate_CRGBF(
      SYSTEM_STATE_INTERNAL[current_system_state].DISPLAY_BACKGROUND_COLOR_A,
      SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_BACKGROUND_COLOR_A,
      system_state_transition_progress_shaped);

    SYSTEM_STATE.DISPLAY_BACKGROUND_COLOR_B = interpolate_CRGBF(
      SYSTEM_STATE_INTERNAL[current_system_state].DISPLAY_BACKGROUND_COLOR_B,
      SYSTEM_STATE_INTERNAL[!current_system_state].DISPLAY_BACKGROUND_COLOR_B,
      system_state_transition_progress_shaped);

    if (system_state_transition_progress >= 1.0) {
      if (transition_running == true) {
        transition_running = false;
        transition_complete_flag = true;
        system_state_transition_progress = 0.0;
        system_state_transition_progress_shaped = 0.0;
      }

      if (system_state_changed == true) {
        memcpy(&SYSTEM_STATE_INTERNAL[current_system_state], &SYSTEM_STATE_INTERNAL[!current_system_state], sizeof(system_state));
        current_system_state = !current_system_state;
        system_state_changed = false;
      }

      if (character_state_changed == true) {
        memcpy(&CHARACTER_STATE[current_character_state], &CHARACTER_STATE[!current_character_state], sizeof(current_character_state));
        current_character_state = !current_character_state;
        //CHARACTER_STATE[current_character_state].OPACITY = 1.0;
        //CHARACTER_STATE[!current_character_state].OPACITY = 0.0;
        character_state_changed = false;
      }

      transition_running = false;
    }
  }

  freeze_led_image = false;
}
// #############################################################################################


// #############################################################################################
// Causes run_system_transition() to begin interpolating the system states
// run_character_transitions() is also triggered by these changes
// This is functionally a "show()" equivalent
void trigger_transition() {
  transition_start_ms = time_ms_now;

  if (SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE == TRANSITION_INSTANT) {
    system_state_transition_progress = 1.0;
    system_state_transition_progress_shaped = 1.0;
  } else {
    system_state_transition_progress = 0.0;
    system_state_transition_progress_shaped = 0.0;
  }

  transition_complete_flag = false;
  transition_running = true;
}
// #############################################################################################


// #############################################################################################
// Set the transition time in milliseconds
void set_transition_time_ms(uint16_t time_ms) {
  SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_DURATION_MS = time_ms;
  system_state_changed = true;
}
// #############################################################################################


// #############################################################################################
// Set the transition type
void set_transition_type(uint8_t transition_type) {
  SYSTEM_STATE_INTERNAL[!current_system_state].TRANSITION_TYPE = transition_type;
  system_state_changed = true;
}
// #############################################################################################


void check_touch() {
  SYSTEM_STATE.TOUCH_VALUE = touchRead(TOUCH_PIN);

  float touch_strength = 1.0-clip_float((SYSTEM_STATE.TOUCH_VALUE - STORAGE.TOUCH_LOW_LEVEL) / (STORAGE.TOUCH_HIGH_LEVEL - STORAGE.TOUCH_LOW_LEVEL));

  bool touching = false;
  if (touch_strength <= STORAGE.TOUCH_THRESHOLD) {
    touching = true;
  }

  if (touching != SYSTEM_STATE.TOUCH_ACTIVE) {
    if (time_ms_now >= 500) {  // If >= half second since boot
      SYSTEM_STATE.TOUCH_ACTIVE = touching;
      send_touch_event();
    }
  }
}