extern int16_t get_sine_float_index(float index);

void run_transition(uint32_t t_now) {
  if (CONFIG.TRANSITION_TYPE == TRANSITION_INSTANT) {
    CONFIG.CURRENT_CHARACTER.character = CONFIG.NEW_CHARACTER.character;
    CONFIG.CURRENT_CHARACTER.opacity = 1.0;
    transition_complete = true;
  } else {
    CONFIG.MASTER_OPACITY = 1.0;
    CONFIG.CURRENT_CHARACTER.opacity = 1.0;
    CONFIG.NEW_CHARACTER.opacity = 0.0;

    float transition_progress = (t_now - transition_start) / float(transition_end - transition_start);

    if (transition_progress < 0.0) {
      transition_progress = 0.0;
    } else if (transition_progress >= 1.0) {
      CONFIG.CURRENT_CHARACTER.character = CONFIG.NEW_CHARACTER.character;
      transition_progress = 1.0;
      transition_complete = true;
      CONFIG.CURRENT_CHARACTER.opacity = 1.0;
      CONFIG.NEW_CHARACTER.opacity = 0.0;

      CONFIG.CURRENT_CHARACTER.pos_x = 0.0;
      CONFIG.CURRENT_CHARACTER.pos_y = 0.0;
      CONFIG.CURRENT_CHARACTER.scale_x = 1.0;
      CONFIG.CURRENT_CHARACTER.scale_y = 1.0;
      CONFIG.CURRENT_CHARACTER.rotation = 0.0;

      CONFIG.NEW_CHARACTER.pos_x = 0.0;
      CONFIG.NEW_CHARACTER.pos_y = 0.0;
      CONFIG.NEW_CHARACTER.scale_x = 1.0;
      CONFIG.NEW_CHARACTER.scale_y = 1.0;
      CONFIG.NEW_CHARACTER.rotation = 0.0;
    }

    if (CONFIG.TRANSITION_TYPE == TRANSITION_FLIP_HORIZONTAL) {
      if(transition_progress >= 0.5){
        CONFIG.CURRENT_CHARACTER.character = CONFIG.NEW_CHARACTER.character;
      }
      CONFIG.CURRENT_CHARACTER.scale_x = (get_sine_float_index(transition_progress + 0.25) / 32768.0) * 0.5 + 0.5;
      CONFIG.NEW_CHARACTER.scale_x = CONFIG.CURRENT_CHARACTER.scale_x;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_FLIP_VERTICAL) {
      if(transition_progress >= 0.5){
        CONFIG.CURRENT_CHARACTER.character = CONFIG.NEW_CHARACTER.character;
      }
      CONFIG.CURRENT_CHARACTER.scale_y = (get_sine_float_index(transition_progress + 0.25) / 32768.0) * 0.5 + 0.5;
      CONFIG.NEW_CHARACTER.scale_y = CONFIG.CURRENT_CHARACTER.scale_y;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_FADE) {
      CONFIG.CURRENT_CHARACTER.opacity = 1.0-transition_progress;
      CONFIG.NEW_CHARACTER.opacity = transition_progress;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_FADE_OUT) {
      if(transition_progress >= 0.5){
        CONFIG.CURRENT_CHARACTER.character = CONFIG.NEW_CHARACTER.character;
      }
      CONFIG.MASTER_OPACITY = (get_sine_float_index(transition_progress + 0.25) / 32768.0) * 0.5 + 0.5;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_SPIN_LEFT) {
      CONFIG.CURRENT_CHARACTER.rotation = ((1.0 - ((get_sine_float_index((transition_progress*0.5) + 0.25) / 32768.0) * 0.5 + 0.5))*360.0)*-1.0;
      CONFIG.NEW_CHARACTER.rotation = CONFIG.CURRENT_CHARACTER.rotation;
      CONFIG.CURRENT_CHARACTER.opacity = 1.0-transition_progress;
      CONFIG.NEW_CHARACTER.opacity = transition_progress;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_SPIN_RIGHT) {
      CONFIG.CURRENT_CHARACTER.rotation = (1.0 - ((get_sine_float_index((transition_progress*0.5) + 0.25) / 32768.0) * 0.5 + 0.5))*360.0;
      CONFIG.NEW_CHARACTER.rotation = CONFIG.CURRENT_CHARACTER.rotation;
      CONFIG.CURRENT_CHARACTER.opacity = 1.0-transition_progress;
      CONFIG.NEW_CHARACTER.opacity = transition_progress;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_SPIN_HALF_LEFT) {
      CONFIG.CURRENT_CHARACTER.rotation = ((1.0 - ((get_sine_float_index((transition_progress*0.5) + 0.25) / 32768.0) * 0.5 + 0.5))*180.0)*-1.0;
      CONFIG.NEW_CHARACTER.rotation = CONFIG.CURRENT_CHARACTER.rotation-180.0;
      CONFIG.CURRENT_CHARACTER.opacity = 1.0-transition_progress;
      CONFIG.NEW_CHARACTER.opacity = transition_progress;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_SPIN_HALF_RIGHT) {
      CONFIG.CURRENT_CHARACTER.rotation = (1.0 - ((get_sine_float_index((transition_progress*0.5) + 0.25) / 32768.0) * 0.5 + 0.5))*180.0;
      CONFIG.NEW_CHARACTER.rotation = CONFIG.CURRENT_CHARACTER.rotation+180.0;
      CONFIG.CURRENT_CHARACTER.opacity = 1.0-transition_progress;
      CONFIG.NEW_CHARACTER.opacity = transition_progress;
    }
    
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_SHRINK) {
      if(transition_progress >= 0.5){
        CONFIG.CURRENT_CHARACTER.character = CONFIG.NEW_CHARACTER.character;
      }
      CONFIG.CURRENT_CHARACTER.scale_x = (get_sine_float_index(transition_progress + 0.25) / 32768.0) * 0.5 + 0.5;
      CONFIG.CURRENT_CHARACTER.scale_y = CONFIG.CURRENT_CHARACTER.scale_x;
      CONFIG.NEW_CHARACTER.scale_x = CONFIG.CURRENT_CHARACTER.scale_x;
      CONFIG.NEW_CHARACTER.scale_y = CONFIG.CURRENT_CHARACTER.scale_y;
    }

    else if (CONFIG.TRANSITION_TYPE == TRANSITION_PUSH_DOWN) {
      float push = 1.0 - ((get_sine_float_index(transition_progress*0.5 + 0.25) / 32768.0) * 0.5 + 0.5);
      CONFIG.CURRENT_CHARACTER.pos_y = -22.0 * push;
      CONFIG.NEW_CHARACTER.pos_y     = CONFIG.CURRENT_CHARACTER.pos_y+22.0;

      CONFIG.CURRENT_CHARACTER.opacity = 1.0-push;
      CONFIG.NEW_CHARACTER.opacity = 1.0;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_PUSH_UP) {
      float push = 1.0 - ((get_sine_float_index(transition_progress*0.5 + 0.25) / 32768.0) * 0.5 + 0.5);
      CONFIG.CURRENT_CHARACTER.pos_y = 22.0 * push;
      CONFIG.NEW_CHARACTER.pos_y     = CONFIG.CURRENT_CHARACTER.pos_y-22.0;

      CONFIG.CURRENT_CHARACTER.opacity = 1.0-push;
      CONFIG.NEW_CHARACTER.opacity = 1.0;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_PUSH_LEFT) {
      float push = 1.0 - ((get_sine_float_index(transition_progress*0.5 + 0.25) / 32768.0) * 0.5 + 0.5);
      CONFIG.CURRENT_CHARACTER.pos_x = -11.0 * push;
      CONFIG.NEW_CHARACTER.pos_x     = CONFIG.CURRENT_CHARACTER.pos_x+11.0;

      CONFIG.CURRENT_CHARACTER.opacity = 1.0-push;
      CONFIG.NEW_CHARACTER.opacity = 1.0;
    }
    else if (CONFIG.TRANSITION_TYPE == TRANSITION_PUSH_RIGHT) {
      float push = 1.0 - ((get_sine_float_index(transition_progress*0.5 + 0.25) / 32768.0) * 0.5 + 0.5);
      CONFIG.CURRENT_CHARACTER.pos_x = 11.0 * push;
      CONFIG.NEW_CHARACTER.pos_x     = CONFIG.CURRENT_CHARACTER.pos_x-11.0;

      CONFIG.CURRENT_CHARACTER.opacity = 1.0-push;
      CONFIG.NEW_CHARACTER.opacity = 1.0;
    }
  }
}