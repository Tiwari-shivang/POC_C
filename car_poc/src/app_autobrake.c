#include "app_autobrake.h"
#include "hal.h"
#include "calib.h"
#include "platform.h"

typedef struct {
    uint8_t hit_count;
    bool brake_active;
} autobrake_state_t;

static autobrake_state_t state = {0U, false};

void app_autobrake_init(void) {
    state.hit_count = 0U;
    state.brake_active = false;
}

void app_autobrake_step(void) {
    uint16_t distance_mm = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    bool should_brake = false;
    
    if (!hal_get_vehicle_ready()) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        return;
    }
    
    if (hal_driver_brake_pressed()) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        return;
    }
    
    sensor_valid = hal_read_distance_mm(&distance_mm, &sensor_ts_ms);
    
    if (!sensor_valid) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        return;
    }
    
    if ((current_time_ms - sensor_ts_ms) > SENSOR_STALE_MS) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        return;
    }
    
    if (distance_mm <= AB_THRESHOLD_MM) {
        if (state.hit_count < AB_DEBOUNCE_HITS) {
            state.hit_count++;
        }
        
        if (state.hit_count >= AB_DEBOUNCE_HITS) {
            should_brake = true;
            state.brake_active = true;
        }
    } else {
        state.hit_count = 0U;
        state.brake_active = false;
    }
    
    hal_set_brake_request(should_brake);
}