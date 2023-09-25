// Instantly switch to the new system state
void run_instant_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress;
}

// Crossfade between characters
void run_fade_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;
}

// Fade to black between characters
void run_fade_out_transition(){
  float opacity_a = 0.0;
  float opacity_b = 0.0;

  if(system_state_transition_progress_shaped < 0.5){
    opacity_a = 0.0;
    opacity_b = 1.0;
  }
  else{
    opacity_a = 1.0;
    opacity_b = 0.0;
  }

  float fade_val = 1.0-saw_to_tri(system_state_transition_progress_shaped);

  CHARACTER_STATE[!current_character_state].OPACITY = opacity_a * fade_val;
  CHARACTER_STATE[current_character_state].OPACITY  = opacity_b * fade_val;
}

// Spin 360 left during transition
void run_spin_left_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;

  CHARACTER_STATE[!current_character_state].ROTATION =  1.0 * (360+360*system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].ROTATION  =  1.0 * (360*system_state_transition_progress_shaped);
}

// Spin 360 right during transition
void run_spin_right_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;

  CHARACTER_STATE[!current_character_state].ROTATION = -1.0 * (360+360*system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].ROTATION  = -1.0 * (360*system_state_transition_progress_shaped);
}

// Spin 180 left during transition
void run_spin_left_half_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;

  CHARACTER_STATE[!current_character_state].ROTATION =  1.0 * (180+180*system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].ROTATION  =  1.0 * (180*system_state_transition_progress_shaped);
}

// Spin 180 right during transition
void run_spin_right_half_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;

  CHARACTER_STATE[!current_character_state].ROTATION = -1.0 * (180+180*system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].ROTATION  = -1.0 * (180*system_state_transition_progress_shaped);
}

// Scroll new character up into view
void run_push_up_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;

  CHARACTER_STATE[!current_character_state].POSITION.y = -17.0 * (1.0-system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].POSITION.y  =  17.0 * (system_state_transition_progress_shaped);
}

// Scroll new character down into view
void run_push_down_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = system_state_transition_progress_shaped;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0 - system_state_transition_progress_shaped;

  CHARACTER_STATE[!current_character_state].POSITION.y =  17.0 * (1.0-system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].POSITION.y  = -17.0 * (system_state_transition_progress_shaped);
}

// Scroll new character left into view
void run_push_left_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = 1.0;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0;

  CHARACTER_STATE[!current_character_state].POSITION.x =  12.0 * (1.0-system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].POSITION.x  = -12.0 * (system_state_transition_progress_shaped);
}

// Scroll new character right into view
void run_push_right_transition(){
  CHARACTER_STATE[!current_character_state].OPACITY = 1.0;
  CHARACTER_STATE[current_character_state].OPACITY = 1.0;

  CHARACTER_STATE[!current_character_state].POSITION.x = -12.0 * (1.0-system_state_transition_progress_shaped);
  CHARACTER_STATE[current_character_state].POSITION.x  =  12.0 * (system_state_transition_progress_shaped);
}

// Shrink old character to point, swap the characters, then scale back up during transitions
void run_shrink_transition(){
  if(system_state_transition_progress_shaped < 0.5){
    CHARACTER_STATE[!current_character_state].OPACITY = 0.0;
    CHARACTER_STATE[current_character_state].OPACITY  = 1.0;
  }
  else{
    CHARACTER_STATE[!current_character_state].OPACITY = 1.0;
    CHARACTER_STATE[current_character_state].OPACITY  = 0.0;
  }

  float shrink_val = 1.0-saw_to_tri(system_state_transition_progress_shaped);

  CHARACTER_STATE[!current_character_state].SCALE.x = shrink_val;
  CHARACTER_STATE[!current_character_state].SCALE.y = shrink_val;

  CHARACTER_STATE[current_character_state].SCALE.x = shrink_val;
  CHARACTER_STATE[current_character_state].SCALE.y = shrink_val;
}

// Flip character horizontally to reveal new character
void run_flip_horizontal_transition(){
  if(system_state_transition_progress_shaped < 0.5){
    CHARACTER_STATE[!current_character_state].OPACITY = 0.0;
    CHARACTER_STATE[current_character_state].OPACITY  = 1.0;
  }
  else{
    CHARACTER_STATE[!current_character_state].OPACITY = 1.0;
    CHARACTER_STATE[current_character_state].OPACITY  = 0.0;
  }

  float flip_val = 1.0-saw_to_tri(system_state_transition_progress_shaped);

  CHARACTER_STATE[!current_character_state].SCALE.x = flip_val;
  //CHARACTER_STATE[!current_character_state].SCALE.y = flip_val;

  CHARACTER_STATE[current_character_state].SCALE.x = flip_val;
  //CHARACTER_STATE[current_character_state].SCALE.y = flip_val;
}

// Flip character vertically to reveal new character
void run_flip_vertical_transition(){
  if(system_state_transition_progress_shaped < 0.5){
    CHARACTER_STATE[!current_character_state].OPACITY = 0.0;
    CHARACTER_STATE[current_character_state].OPACITY  = 1.0;
  }
  else{
    CHARACTER_STATE[!current_character_state].OPACITY = 1.0;
    CHARACTER_STATE[current_character_state].OPACITY  = 0.0;
  }

  float flip_val = 1.0-saw_to_tri(system_state_transition_progress_shaped);

  //CHARACTER_STATE[!current_character_state].SCALE.x = flip_val;
  CHARACTER_STATE[!current_character_state].SCALE.y = flip_val;

  //CHARACTER_STATE[current_character_state].SCALE.x = flip_val;
  CHARACTER_STATE[current_character_state].SCALE.y = flip_val;
}