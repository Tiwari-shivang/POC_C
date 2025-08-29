#include "app_autobrake.h"
#include "hal.h"
#include "calib.h"
#include "config.h"
#include "platform.h"
#include "eval_hooks.h"

typedef struct {
    uint8_t hit_count;
    bool brake_active;
    bool prev_below_thresh; /* NEW: detect first-below edge */
} autobrake_state_t;

static autobrake_state_t state = {0U, false, false};

void app_autobrake_init(void) {
    state.hit_count = 0U;
    state.brake_active = false;
    state.prev_below_thresh = false; /* NEW */
}

void app_autobrake_step(void) {
    uint16_t distance_mm = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    bool should_brake = false;
    
    eval_loop_tick_begin(current_time_ms); /* NEW */
    
    /* MISRA Rule 16.5: Ensure critical state consistency */
    platform_assert(state.hit_count <= AUTOBRAKE_DEBOUNCE_COUNT);
    
    if (!hal_get_vehicle_ready()) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }
    
    if (hal_driver_brake_pressed()) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }
    
    sensor_valid = hal_read_distance_mm(&distance_mm, &sensor_ts_ms);
    
    if (!sensor_valid) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }
    
    if ((current_time_ms - sensor_ts_ms) > STALE_MS) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, distance_mm, (current_time_ms - sensor_ts_ms), state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }
    
    /* After obtaining sensor values */
    const uint32_t sensor_age = current_time_ms - sensor_ts_ms;
    const bool below = (distance_mm <= BRAKE_THRESH_MM);

    /* First-below edge detection */
    if ((below != false) && (state.prev_below_thresh == false)) {
        eval_autobrake_event(EVAL_EVT_FIRST_BELOW_THRESH, current_time_ms);
    }
    state.prev_below_thresh = below;

    /* Main control logic with eval hooks */
    if (below) {
        if (state.hit_count < AUTOBRAKE_DEBOUNCE_COUNT) {
            state.hit_count++;
            if (state.hit_count == AUTOBRAKE_DEBOUNCE_COUNT) {
                eval_autobrake_event(EVAL_EVT_HAZARD_FLAG, current_time_ms);
            }
        }
        
        if ((state.hit_count >= AUTOBRAKE_DEBOUNCE_COUNT) && (state.brake_active == false)) {
            state.brake_active = true;
            eval_autobrake_event(EVAL_EVT_BRAKE_ASSERT, current_time_ms);
        }
    } else {
        if (state.brake_active != false) {
            state.brake_active = false;
            eval_autobrake_event(EVAL_EVT_BRAKE_DEASSERT, current_time_ms);
        }
        state.hit_count = 0U;
    }
    
    should_brake = state.brake_active;
    
    hal_set_brake_request(should_brake);
    
    /* Per-tick sample & loop end */
    eval_autobrake_sample(current_time_ms, distance_mm, sensor_age, state.hit_count, state.brake_active);
    eval_loop_tick_end(current_time_ms);
}