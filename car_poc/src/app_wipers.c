#include "app_wipers.h"
#include "hal.h"
#include "calib.h"
#include "platform.h"

typedef enum {
    WIPER_MODE_OFF = 0U,
    WIPER_MODE_INT = 1U,
    WIPER_MODE_LOW = 2U,
    WIPER_MODE_HIGH = 3U
} wiper_mode_e;

typedef struct {
    uint8_t current_mode;
} wipers_state_t;

static wipers_state_t state = {WIPER_MODE_OFF};

void app_wipers_init(void) {
    state.current_mode = WIPER_MODE_OFF;
}

static uint8_t determine_wiper_mode(uint8_t rain_pct, uint8_t current_mode) {
    uint8_t new_mode = current_mode;
    
    if (current_mode == WIPER_MODE_OFF) {
        if (rain_pct >= WIPER_T_RAIN_INT) {
            new_mode = WIPER_MODE_INT;
        }
    } else if (current_mode == WIPER_MODE_INT) {
        if (rain_pct < (WIPER_T_RAIN_INT - 5U)) {
            new_mode = WIPER_MODE_OFF;
        } else if (rain_pct >= WIPER_T_RAIN_LOW) {
            new_mode = WIPER_MODE_LOW;
        } else {
        }
    } else if (current_mode == WIPER_MODE_LOW) {
        if (rain_pct < (WIPER_T_RAIN_INT - 5U)) {
            new_mode = WIPER_MODE_INT;
        } else if (rain_pct >= WIPER_T_RAIN_HIGH) {
            new_mode = WIPER_MODE_HIGH;
        } else {
        }
    } else if (current_mode == WIPER_MODE_HIGH) {
        if (rain_pct < (WIPER_T_RAIN_LOW - 5U)) {
            new_mode = WIPER_MODE_LOW;
        }
    } else {
    }
    
    return new_mode;
}

void app_wipers_step(void) {
    uint8_t rain_pct = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    uint8_t new_mode = WIPER_MODE_OFF;
    
    sensor_valid = hal_read_rain_level_pct(&rain_pct, &sensor_ts_ms);
    
    if (!sensor_valid) {
        state.current_mode = WIPER_MODE_OFF;
        hal_set_wiper_mode(state.current_mode);
        return;
    }
    
    if ((current_time_ms - sensor_ts_ms) > SENSOR_STALE_MS) {
        state.current_mode = WIPER_MODE_OFF;
        hal_set_wiper_mode(state.current_mode);
        return;
    }
    
    new_mode = determine_wiper_mode(rain_pct, state.current_mode);
    state.current_mode = new_mode;
    
    hal_set_wiper_mode(state.current_mode);
}