#include "app_autobrake.h"
#include "hal.h"
#include "calib.h"
#include "config.h"
#include "platform.h"
#include "eval_hooks.h"

/* AUTOSAR SWS_Rte_01169: Static module-internal state encapsulation
 * Following AUTOSAR memory abstraction - all state variables grouped in single structure */
typedef struct {
    uint8_t hit_count;
    bool brake_active;
    bool prev_below_thresh; /* NEW: detect first-below edge */
} autobrake_state_t;

static autobrake_state_t state = {0U, false, false};

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Module initialization with deterministic state reset */
void app_autobrake_init(void) {
    state.hit_count = 0U;
    state.brake_active = false;
    state.prev_below_thresh = false; /* NEW */
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Cyclic function with deterministic behavior */
void app_autobrake_step(void) {
    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety-critical data */
    uint16_t distance_mm = 0U;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    bool should_brake = false;
    
    eval_loop_tick_begin(current_time_ms); /* NEW */
    
    /* AUTOSAR SWS_BSW_00134: Runtime assertion for defensive programming
     * MISRA Rule 16.5: Ensure critical state consistency */
    platform_assert(state.hit_count <= AUTOBRAKE_DEBOUNCE_COUNT);

    /* AUTOSAR SWS_BSW_00160: Check preconditions before processing
     * Vehicle readiness check ensures system is in valid operational state */
    if (!hal_get_vehicle_ready()) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }
    
    /* AUTOSAR SWS_BSW_00161: Driver override capability for safety
     * Manual brake input takes precedence over automated system */
    if (hal_driver_brake_pressed()) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }

    /* AUTOSAR SWS_Rte_07139: Data acquisition through standardized port interface */
    sensor_valid = hal_read_distance_mm(&distance_mm, &sensor_ts_ms);
    
    /* AUTOSAR SWS_BSW_00167: Error handling for invalid sensor data
     * Fail-safe behavior when sensor communication fails */
    if (!sensor_valid) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }

    /* AUTOSAR SWS_BSW_00170: Time supervision and plausibility checks
     * Detect and handle stale sensor data to ensure temporal validity */
    if ((current_time_ms - sensor_ts_ms) > STALE_MS) {
        state.hit_count = 0U;
        state.brake_active = false;
        hal_set_brake_request(false);
        eval_autobrake_sample(current_time_ms, distance_mm, (current_time_ms - sensor_ts_ms), state.hit_count, state.brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }
    
    /* AUTOSAR SWS_BSW_00104: Processed data derivation from raw sensor values */
    const uint32_t sensor_age = current_time_ms - sensor_ts_ms;
    const bool below = (distance_mm <= BRAKE_THRESH_MM);

    /* AUTOSAR SWS_Rte_02505: Event-driven processing for state transitions */
    if ((below != false) && (state.prev_below_thresh == false)) {
        eval_autobrake_event(EVAL_EVT_FIRST_BELOW_THRESH, current_time_ms);
    }
    state.prev_below_thresh = below;

    /* AUTOSAR SWS_BSW_00171: Debouncing mechanism for sensor signal filtering
     * Prevents spurious activations from transient sensor noise */
    if (below) {
        if (state.hit_count < AUTOBRAKE_DEBOUNCE_COUNT) {
            state.hit_count++;
            if (state.hit_count == AUTOBRAKE_DEBOUNCE_COUNT) {
                eval_autobrake_event(EVAL_EVT_HAZARD_FLAG, current_time_ms);
            }
        }

        /* AUTOSAR SWS_BSW_00172: State machine implementation for control logic
         * Deterministic state transitions based on debounced inputs */
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
    
    /* AUTOSAR SWS_BSW_00103: Clear separation between decision and actuation */
    should_brake = state.brake_active;

    /* AUTOSAR SWS_Rte_07140: Actuator control through standardized port interface */
    hal_set_brake_request(should_brake);

    /* AUTOSAR SWS_BSW_00202: Diagnostic data collection for monitoring
     * Record operational data for system health monitoring and diagnostics */
    eval_autobrake_sample(current_time_ms, distance_mm, sensor_age, state.hit_count, state.brake_active);
    eval_loop_tick_end(current_time_ms);
}