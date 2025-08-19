#include "app_climate.h"
#include "hal.h"
#include "calib.h"
#include "platform.h"

#define MAX_FAN_STAGE (3U)
#define MAX_BLEND_PCT (100U)
#define INTEGRAL_CLAMP_MAX (1000)
#define INTEGRAL_CLAMP_MIN (-1000)
#define PI_OUTPUT_MAX (300)
#define PI_OUTPUT_MIN (-300)
#define HIGH_HUMIDITY_THRESHOLD (70U)

typedef struct {
    int16_t setpoint_x10;
    int32_t integral_accumulator;
    uint32_t last_update_ms;
    uint8_t current_fan_stage;
    bool current_ac_on;
    uint8_t current_blend_pct;
} climate_state_t;

static climate_state_t state = {220, 0, 0U, 0U, false, 50U};

void app_climate_init(void) {
    state.setpoint_x10 = 220;
    state.integral_accumulator = 0;
    state.last_update_ms = 0U;
    state.current_fan_stage = 0U;
    state.current_ac_on = false;
    state.current_blend_pct = 50U;
}

static uint8_t map_pi_output_to_fan_stage(int32_t pi_output) {
    int32_t abs_output = (pi_output < 0) ? (-pi_output) : pi_output;
    uint8_t fan_stage = 0U;
    
    if (abs_output > 200) {
        fan_stage = MAX_FAN_STAGE;
    } else if (abs_output > 100) {
        fan_stage = 2U;
    } else if (abs_output > 50) {
        fan_stage = 1U;
    } else {
        fan_stage = 0U;
    }
    
    return fan_stage;
}

static uint8_t calculate_blend_percentage(int32_t pi_output) {
    uint8_t blend_pct = 50U;
    
    if (pi_output > 0) {
        blend_pct = 100U;
    } else if (pi_output < -50) {
        blend_pct = 0U;
    } else {
        blend_pct = 50U;
    }
    
    return blend_pct;
}

void app_climate_step(void) {
    int16_t cabin_temp_x10 = 0;
    int16_t ambient_temp_x10 = 0;
    uint8_t humidity_pct = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool cabin_valid = false;
    bool ambient_valid = false;
    bool humidity_valid = false;
    int32_t error_x10 = 0;
    int32_t pi_output = 0;
    uint32_t dt_ms = 0U;
    bool ac_required = false;
    
    cabin_valid = hal_read_cabin_temp_c(&cabin_temp_x10, &sensor_ts_ms);
    if (!cabin_valid || ((current_time_ms - sensor_ts_ms) > SENSOR_STALE_MS)) {
        state.current_fan_stage = 0U;
        state.current_ac_on = false;
        state.current_blend_pct = 50U;
        hal_set_climate(state.current_fan_stage, state.current_ac_on, state.current_blend_pct);
        return;
    }
    
    ambient_valid = hal_read_ambient_temp_c(&ambient_temp_x10, &sensor_ts_ms);
    humidity_valid = hal_read_humidity_pct(&humidity_pct, &sensor_ts_ms);
    
    if (state.last_update_ms == 0U) {
        state.last_update_ms = current_time_ms;
        hal_set_climate(state.current_fan_stage, state.current_ac_on, state.current_blend_pct);
        return;
    }
    
    dt_ms = current_time_ms - state.last_update_ms;
    if (dt_ms < CLIMATE_DT_MS) {
        hal_set_climate(state.current_fan_stage, state.current_ac_on, state.current_blend_pct);
        return;
    }
    
    state.last_update_ms = current_time_ms;
    
    error_x10 = state.setpoint_x10 - cabin_temp_x10;
    
    state.integral_accumulator += ((int32_t)error_x10 * CLIMATE_KI);
    
    if (state.integral_accumulator > INTEGRAL_CLAMP_MAX) {
        state.integral_accumulator = INTEGRAL_CLAMP_MAX;
    } else if (state.integral_accumulator < INTEGRAL_CLAMP_MIN) {
        state.integral_accumulator = INTEGRAL_CLAMP_MIN;
    } else {
    }
    
    pi_output = ((int32_t)error_x10 * CLIMATE_KP) + state.integral_accumulator;
    
    if (pi_output > PI_OUTPUT_MAX) {
        pi_output = PI_OUTPUT_MAX;
        state.integral_accumulator -= ((int32_t)error_x10 * CLIMATE_KI);
    } else if (pi_output < PI_OUTPUT_MIN) {
        pi_output = PI_OUTPUT_MIN;
        state.integral_accumulator -= ((int32_t)error_x10 * CLIMATE_KI);
    } else {
    }
    
    state.current_fan_stage = map_pi_output_to_fan_stage(pi_output);
    state.current_blend_pct = calculate_blend_percentage(pi_output);
    
    ac_required = false;
    if (error_x10 < -20) {
        ac_required = true;
    }
    
    if (humidity_valid && (humidity_pct > HIGH_HUMIDITY_THRESHOLD)) {
        ac_required = true;
    }
    
    if (ambient_valid && (ambient_temp_x10 > (state.setpoint_x10 + 50))) {
        ac_required = true;
    }
    
    state.current_ac_on = ac_required;
    
    hal_set_climate(state.current_fan_stage, state.current_ac_on, state.current_blend_pct);
}