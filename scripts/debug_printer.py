import serial
import os
from datetime import datetime

var_names = [
    "CHAIN_CONFIG.LOCAL_ADDRESS",
    "CHAIN_CONFIG.CHAIN_LENGTH",
    "CHAIN_CONFIG.PROPAGATION_MODE",
    "CHAIN_CONFIG.BUS_MODE",
    "last_probe_tx_time_ms",
    "probe_timeout_ms",
    "probe_timeout_occurred",
    "probe_packet_received",
    "terminating_node",
    "propagation_queued",
    "assignment_complete",
    "discovery_complete",
    "rx_drop_start",
    "rx_drop_duration",
    "rx_low",
    "show_called_once",
    "upstream_packets_receieved",
    "time_ms_now",
    "time_us_now",
    "esp_get_free_heap_size()",
    "uxTaskGetStackHighWaterMark(cpu_task)",
    "uxTaskGetStackHighWaterMark(gpu_task)",
    "frame_blending_amount",
    "debug_led_opacity",
    "fade_in_complete",
    "GLOBAL_LED_BRIGHTNESS",
    "freeze_led_image",
    "transition_complete_flag",
    "system_state_changed",
    "current_system_state",
    "system_state_transition_progress",
    "system_state_transition_progress_shaped",
    "transition_start_ms",
    "transition_end_ms",
    "transition_running",
    "last_gpu_check_in",
    "system_ready",
    "SYSTEM_STATE.BRIGHTNESS",
    "SYSTEM_STATE.TRANSITION_TYPE",
    "SYSTEM_STATE.TRANSITION_DURATION_MS",
    "SYSTEM_STATE.TOUCH_ACTIVE",
    "SYSTEM_STATE.TOUCH_VALUE",
    "STORAGE.TOUCH_THRESHOLD",
    "STORAGE.TOUCH_HIGH_LEVEL",
    "STORAGE.TOUCH_LOW_LEVEL",
    "character_state_changed",
    "current_character_state",
    "dither_index",
]

def main():
    # Initialize serial connection at COM24 with 115200 Baud rate
    ser = serial.Serial('COM24', 115200)
    
    while True:
        # Read data until '\n'
        try:
            line = ser.readline().decode('utf-8').strip()
        
            
            # Clear terminal
            #os.system('cls' if os.name == 'nt' else 'clear')
            
            # Get the current local time
            current_time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            
            # Initialize an empty string for accumulating the output
            output_str = current_time + "\t\t\t\t" + line
            
            # Print to the console
            print(output_str)
            
            # Append to log.txt
            with open("log.txt", "a+") as f:
                f.write(output_str + "\n")
        except:
            print("ERR")
            pass

if __name__ == "__main__":
    main()
