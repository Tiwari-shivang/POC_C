#include "app_speedgov.h"
#include "hal.h"
#include "calib.h"
#include "platform.h"

typedef struct {
    uint16_t current_limit_kph;
    uint8_t overspeed_count;
    bool alarm_active;
} speedgov_state_t;

static speedgov_state_t state = {50U, 0U, false};

void app_speedgov_init(void) {
    state.current_limit_kph = 50U;
    state.overspeed_count = 0U;
    state.alarm_active = false;
}

void app_speedgov_step(void) {
    uint16_t vehicle_speed_kph = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    uint16_t new_limit = 0U;
    bool speed_limit_updated = false;
    uint16_t overspeed_threshold = 0U;
    uint16_t clear_threshold = 0U;
    bool should_alarm = false;
    
    speed_limit_updated = hal_poll_speed_limit_kph(&new_limit);
    if (speed_limit_updated) {
        state.current_limit_kph = new_limit;
        state.overspeed_count = 0U;
        state.alarm_active = false;
    }
    
    sensor_valid = hal_read_vehicle_speed_kph(&vehicle_speed_kph, &sensor_ts_ms);
    
    if (!sensor_valid) {
        state.overspeed_count = 0U;
        state.alarm_active = false;
        hal_set_alarm(false);
        return;
    }
    
    if ((current_time_ms - sensor_ts_ms) > SENSOR_STALE_MS) {
        state.overspeed_count = 0U;
        state.alarm_active = false;
        hal_set_alarm(false);
        return;
    }
    
    overspeed_threshold = state.current_limit_kph + SPEED_ALARM_TOL_KPH;
    clear_threshold = state.current_limit_kph - SPEED_HYSTERESIS_KPH;
    
    if (state.alarm_active) {
        if (vehicle_speed_kph < clear_threshold) {
            state.alarm_active = false;
            state.overspeed_count = 0U;
        }
    } else {
        if (vehicle_speed_kph > overspeed_threshold) {
            if (state.overspeed_count < SPEED_ALARM_DEBOUNCE) {
                state.overspeed_count++;
            }
            
            if (state.overspeed_count >= SPEED_ALARM_DEBOUNCE) {
                state.alarm_active = true;
            }
        } else {
            state.overspeed_count = 0U;
        }
    }
    
    should_alarm = state.alarm_active;
    hal_set_alarm(should_alarm);
    hal_set_speed_limit_request(state.current_limit_kph);
}