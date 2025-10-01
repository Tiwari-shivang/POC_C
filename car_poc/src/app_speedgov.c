#include "app_speedgov.h"
#include "hal.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

#ifndef SPEEDGOV_DEFAULT_LIMIT_KPH
#define SPEEDGOV_DEFAULT_LIMIT_KPH        (50U)
#endif

#ifndef SPEEDGOV_DEBOUNCE_COUNT
#define SPEEDGOV_DEBOUNCE_COUNT           (2U)
#endif

#ifndef SPEEDGOV_HYSTERESIS_KPH
#define SPEEDGOV_HYSTERESIS_KPH           (3U)
#endif

#ifndef STALE_MS
#define STALE_MS                          (100U)
#endif

/* AUTOSAR SWS_Rte_01169: Static module-internal state encapsulation
 * Speed limiter state variables following memory abstraction pattern */
typedef struct {
    uint16_t current_limit_kph;
    uint8_t overspeed_count;
    bool alarm_active;
} speedgov_state_t;

static speedgov_state_t state = {SPEEDGOV_DEFAULT_LIMIT_KPH, 0U, false};

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Module initialization with safe default values */
void app_speedgov_init(void) {
    state.current_limit_kph = SPEEDGOV_DEFAULT_LIMIT_KPH;
    state.overspeed_count = 0U;
    state.alarm_active = false;
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Cyclic function with deterministic behavior */
void app_speedgov_step(void) {
    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety-critical data */
    uint16_t vehicle_speed_kph = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    uint16_t new_limit = 0U;
    bool speed_limit_updated = false;
    uint16_t overspeed_threshold = 0U;
    uint16_t clear_threshold = 0U;
    bool should_alarm = false;
    
    /* AUTOSAR SWS_Rte_07139: Dynamic configuration update via port interface
     * AUTOSAR SWS_BSW_00106: Runtime parameter update with validation */
    speed_limit_updated = hal_poll_speed_limit_kph(&new_limit);
    if (speed_limit_updated && (new_limit > 0U)) {
        state.current_limit_kph = new_limit;
        state.overspeed_count = 0U;
        state.alarm_active = false;
    }
    
    /* AUTOSAR SWS_Rte_07139: Sensor data acquisition through port interface */
    sensor_valid = hal_read_vehicle_speed_kph(&vehicle_speed_kph, &sensor_ts_ms);

    /* AUTOSAR SWS_BSW_00167: Error handling for invalid sensor data
     * Safe state transition when speed reading fails */
    if (!sensor_valid) {
        state.overspeed_count = 0U;
        state.alarm_active = false;
        hal_set_alarm(false);
        return;
    }
    
    /* AUTOSAR SWS_BSW_00170: Time supervision for data freshness
     * Detect stale sensor readings and enter fail-safe mode */
    if ((current_time_ms - sensor_ts_ms) > STALE_MS) {
        state.overspeed_count = 0U;
        state.alarm_active = false;
        hal_set_alarm(false);
        return;
    }
    
    /* AUTOSAR SWS_BSW_00174: Hysteresis implementation for stability
     * Prevents oscillation around threshold values */
    overspeed_threshold = state.current_limit_kph;
    clear_threshold = state.current_limit_kph - SPEEDGOV_HYSTERESIS_KPH;
    
    /* AUTOSAR SWS_BSW_00172: State machine for alarm management
     * AUTOSAR SWS_BSW_00171: Debouncing mechanism for robust detection */
    if (state.alarm_active) {
        if (vehicle_speed_kph <= clear_threshold) {
            state.alarm_active = false;
            state.overspeed_count = 0U;
        }
    } else {
        if (vehicle_speed_kph > overspeed_threshold) {
            if (state.overspeed_count < SPEEDGOV_DEBOUNCE_COUNT) {
                state.overspeed_count++;
            }

            if (state.overspeed_count >= SPEEDGOV_DEBOUNCE_COUNT) {
                state.alarm_active = true;
            }
        } else {
            state.overspeed_count = 0U;
        }
    }
    
    /* AUTOSAR SWS_BSW_00103: Separation between decision and actuation */
    should_alarm = state.alarm_active;

    /* AUTOSAR SWS_Rte_07140: Actuator control through standardized interfaces
     * Update alarm state and communicate speed limit to vehicle systems */
    hal_set_alarm(should_alarm);
    hal_set_speed_limit_request(state.current_limit_kph);
}