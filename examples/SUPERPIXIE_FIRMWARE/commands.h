// Possible UART commands
enum commands {
  COM_NULL,
  COM_ACK,
  COM_PROBE,
  COM_BLINK,
  COM_RESET_CHAIN,
  COM_ENABLE_PROPAGATION,
  COM_ASSIGN_ADDRESS,
  COM_SET_BAUD,
  COM_SET_DEBUG_LED,
  COM_SET_DEBUG_DIGIT,
  COM_SET_BRIGHTNESS,
  COM_SET_COLORS,
  COM_SHOW,
  COM_SEND_STRING,
  COM_ANNOUNCE,
  COM_SET_TRANSITION_TYPE,
  COM_SET_TRANSITION_DURATION_MS,
  COM_SET_FRAME_BLENDING,
  COM_SET_FX_COLOR,
  COM_SET_FX_OPACITY,
  COM_SET_FX_BLUR,
  COM_SET_GRADIENT_TYPE,
  COM_TOUCH_EVENT,
  COM_FORCE_TRANSITION,
  COM_SCROLL_STRING,
  COM_SET_CHAIN_LENGTH,
  COM_SET_SCROLL_SPEED,
  COM_GET_READY_STATUS,
  COM_READY_STATUS,

  NUM_COMMANDS
};
