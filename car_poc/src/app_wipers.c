#include "app_wipers.h"
#include "hal.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

#ifndef RAIN_THR_INT_ON_PCT
#define RAIN_THR_INT_ON_PCT      (20U)
#endif
#ifndef RAIN_THR_INT_OFF_PCT
#define RAIN_THR_INT_OFF_PCT     (15U)
#endif
#ifndef RAIN_THR_LOW_ON_PCT
#define RAIN_THR_LOW_ON_PCT      (40U)
#endif
#ifndef RAIN_THR_LOW_OFF_PCT
#define RAIN_THR_LOW_OFF_PCT     (35U)
#endif
#ifndef RAIN_THR_HIGH_ON_PCT
#define RAIN_THR_HIGH_ON_PCT     (70U)
#endif
#ifndef RAIN_THR_HIGH_OFF_PCT
#define RAIN_THR_HIGH_OFF_PCT    (60U)
#endif
#ifndef WIPERS_DEBOUNCE_COUNT
#define WIPERS_DEBOUNCE_COUNT    (2U)
#endif
#ifndef STALE_MS
#define STALE_MS                 (100U)
#endif

typedef enum {
    WIPER_MODE_OFF = 0U,
    WIPER_MODE_INT = 1U,
    WIPER_MODE_LOW = 2U,
    WIPER_MODE_HIGH = 3U
} wiper_mode_e;

typedef struct {
    uint8_t current_mode;
    uint8_t debounce_counter;
    uint8_t pending_mode;
} wipers_state_t;

static wipers_state_t state = {WIPER_MODE_OFF, 0U, WIPER_MODE_OFF};

void app_wipers_init(void) {
    state.current_mode = WIPER_MODE_OFF;
    state.debounce_counter = 0U;
    state.pending_mode = WIPER_MODE_OFF;
}

static uint8_t determine_wiper_mode(uint8_t rain_pct, uint8_t current_mode) {
    uint8_t new_mode = current_mode;
    
    if (current_mode == WIPER_MODE_OFF) {
        if (rain_pct >= RAIN_THR_INT_ON_PCT) {
            new_mode = WIPER_MODE_INT;
        }
    } else if (current_mode == WIPER_MODE_INT) {
        if (rain_pct < RAIN_THR_INT_OFF_PCT) {
            new_mode = WIPER_MODE_OFF;
        } else if (rain_pct >= RAIN_THR_LOW_ON_PCT) {
            new_mode = WIPER_MODE_LOW;
        }
    } else if (current_mode == WIPER_MODE_LOW) {
        if (rain_pct < RAIN_THR_LOW_OFF_PCT) {
            new_mode = WIPER_MODE_INT;
        } else if (rain_pct >= RAIN_THR_HIGH_ON_PCT) {
            new_mode = WIPER_MODE_HIGH;
        }
    } else if (current_mode == WIPER_MODE_HIGH) {
        if (rain_pct < RAIN_THR_HIGH_OFF_PCT) {
            new_mode = WIPER_MODE_LOW;
        }
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
        state.debounce_counter = 0U;
        state.pending_mode = WIPER_MODE_OFF;
        hal_set_wiper_mode(state.current_mode);
        return;
    }
    
    if ((current_time_ms - sensor_ts_ms) > STALE_MS) {
        hal_set_wiper_mode(state.current_mode);
        return;
    }
    
    new_mode = determine_wiper_mode(rain_pct, state.current_mode);
    
    if (new_mode == state.pending_mode) {
        if (state.debounce_counter < WIPERS_DEBOUNCE_COUNT) {
            state.debounce_counter++;
        }
        
        if (state.debounce_counter >= WIPERS_DEBOUNCE_COUNT) {
            state.current_mode = new_mode;
        }
    } else {
        state.pending_mode = new_mode;
        state.debounce_counter = 1U;
    }
    
    hal_set_wiper_mode(state.current_mode);
}