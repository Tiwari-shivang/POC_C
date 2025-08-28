#include "app_autopark.h"
#include "hal.h"
#include "calib.h"
#include "platform.h"

typedef enum {
    PARK_STATE_SCANNING = 0U,
    PARK_STATE_REVERSING_RIGHT = 1U,
    PARK_STATE_STRAIGHTENING = 2U,
    PARK_STATE_REVERSING_LEFT = 3U,
    PARK_STATE_DONE = 4U
} park_state_e;

typedef struct {
    uint8_t state;
    uint16_t step_counter;
    uint8_t gap_detections;
    bool gap_suitable;
} autopark_state_t;

static autopark_state_t state = {PARK_STATE_SCANNING, 0U, 0U, false};

void app_autopark_init(void) {
    state.state = PARK_STATE_SCANNING;
    state.step_counter = 0U;
    state.gap_detections = 0U;
    state.gap_suitable = false;
}

static bool is_speed_suitable_for_parking(void) {
    uint16_t speed_kph = 0U;
    uint32_t ts_ms = 0U;
    
    if (!hal_read_vehicle_speed_kph(&speed_kph, &ts_ms)) {
        return false;
    }
    
    return (speed_kph <= 10U);
}

void app_autopark_step(void) {
    park_gap_t gap_data;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    uint8_t prompt_code = 0U;
    
    if (!is_speed_suitable_for_parking()) {
        app_autopark_init();
        hal_actuate_parking_prompt(0U);
        return;
    }
    
    sensor_valid = hal_parking_gap_read(&gap_data, &sensor_ts_ms);
    
    if (!sensor_valid || ((current_time_ms - sensor_ts_ms) > SENSOR_STALE_MS)) {
        if (state.state != PARK_STATE_SCANNING) {
            app_autopark_init();
        }
        hal_actuate_parking_prompt(0U);
        return;
    }
    
    switch (state.state) {
        case PARK_STATE_SCANNING:
            if (gap_data.found && (gap_data.width_mm >= PARK_MIN_GAP_MM)) {
                state.gap_detections++;
                if (state.gap_detections >= 3U) {
                    state.gap_suitable = true;
                    state.state = PARK_STATE_REVERSING_RIGHT;
                    state.step_counter = 0U;
                }
            } else {
                state.gap_detections = 0U;
                state.gap_suitable = false;
            }
            break;
            
        case PARK_STATE_REVERSING_RIGHT:
            state.step_counter++;
            if (state.step_counter >= 50U) {
                state.state = PARK_STATE_STRAIGHTENING;
                state.step_counter = 0U;
            }
            break;
            
        case PARK_STATE_STRAIGHTENING:
            state.step_counter++;
            if (state.step_counter >= 20U) {
                state.state = PARK_STATE_REVERSING_LEFT;
                state.step_counter = 0U;
            }
            break;
            
        case PARK_STATE_REVERSING_LEFT:
            state.step_counter++;
            if (state.step_counter >= 50U) {
                state.state = PARK_STATE_DONE;
                state.step_counter = 0U;
            }
            break;
            
        case PARK_STATE_DONE:
            break;
            
        default:
            app_autopark_init();
            break;
    }
    
    /* Set prompt code based on current state after transitions */
    switch (state.state) {
        case PARK_STATE_SCANNING:
            prompt_code = 1U;
            break;
        case PARK_STATE_REVERSING_RIGHT:
            prompt_code = 2U;
            break;
        case PARK_STATE_STRAIGHTENING:
            prompt_code = 3U;
            break;
        case PARK_STATE_REVERSING_LEFT:
            prompt_code = 4U;
            break;
        case PARK_STATE_DONE:
            prompt_code = 5U;
            break;
        default:
            prompt_code = 0U;
            break;
    }
    
    hal_actuate_parking_prompt(prompt_code);
}