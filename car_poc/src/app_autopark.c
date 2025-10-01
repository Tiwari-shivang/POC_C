#include "app_autopark.h"
#include "hal.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

/* AUTOSAR SWS_BSW_00173: Enumerated state definition for finite state machine
 * Clear state encoding with explicit values for traceability */
typedef enum {
    PARK_STATE_SCANNING = 0U,
    PARK_STATE_REVERSING_RIGHT = 1U,
    PARK_STATE_STRAIGHTENING = 2U,
    PARK_STATE_REVERSING_LEFT = 3U,
    PARK_STATE_DONE = 4U
} park_state_e;

/* AUTOSAR SWS_Rte_01169: Static module-internal state encapsulation
 * Context data structure for parking assistance state management */
typedef struct {
    uint8_t state;
    uint16_t step_counter;
    uint8_t gap_detections;
    bool gap_suitable;
} autopark_state_t;

static autopark_state_t state = {PARK_STATE_SCANNING, 0U, 0U, false};

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Deterministic state reset to known safe values */
void app_autopark_init(void) {
    state.state = PARK_STATE_SCANNING;
    state.step_counter = 0U;
    state.gap_detections = 0U;
    state.gap_suitable = false;
}

/* AUTOSAR SWS_BSW_00105: Helper function for safety condition evaluation
 * Validates vehicle speed is within safe operational envelope for parking */
static bool is_speed_suitable_for_parking(void) {
    uint16_t speed_kph = 0U;
    uint32_t ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();

    /* AUTOSAR SWS_Rte_07139: Data acquisition through standardized interface */
    if (!hal_read_vehicle_speed_kph(&speed_kph, &ts_ms)) {
        return false;
    }

    /* AUTOSAR SWS_BSW_00170: Temporal validity check for sensor data */
    if ((current_time_ms - ts_ms) > STALE_MS) {
        return false;
    }

    return (speed_kph <= AUTOPARK_MAX_SPEED_KPH);
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Deterministic cyclic processing function */
void app_autopark_step(void) {
    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety */
    park_gap_t gap_data;
    uint32_t sensor_ts_ms = 0U;
    uint32_t current_time_ms = hal_now_ms();
    bool sensor_valid = false;
    uint8_t prompt_code = 0U;
    
    /* AUTOSAR SWS_BSW_00160: Precondition validation before processing
     * Ensure vehicle speed is safe for parking maneuver */
    if (!is_speed_suitable_for_parking()) {
        app_autopark_init();
        hal_actuate_parking_prompt(0U);
        return;
    }
    
    /* AUTOSAR SWS_Rte_07139: Sensor data acquisition via port interface */
    sensor_valid = hal_parking_gap_read(&gap_data, &sensor_ts_ms);

    /* AUTOSAR SWS_BSW_00167: Error handling for invalid/stale sensor data
     * AUTOSAR SWS_BSW_00170: Time supervision for data freshness */
    if (!sensor_valid || ((current_time_ms - sensor_ts_ms) > STALE_MS)) {
        if (state.state != PARK_STATE_SCANNING) {
            app_autopark_init();
        }
        hal_actuate_parking_prompt(0U);
        return;
    }
    
    /* AUTOSAR SWS_BSW_00172: State machine implementation
     * AUTOSAR SWS_Rte_02505: Event-driven state transitions */
    switch (state.state) {
        case PARK_STATE_SCANNING:
            /* AUTOSAR SWS_BSW_00171: Debouncing for robust gap detection */
            if (gap_data.found && (gap_data.width_mm >= AUTOPARK_MIN_GAP_MM)) {
                state.gap_detections++;
                if (state.gap_detections >= AUTOPARK_DEBOUNCE_COUNT) {
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
            /* AUTOSAR SWS_BSW_00168: Continuous monitoring of safety conditions
             * Abort maneuver if parking gap becomes invalid */
            if (!gap_data.found || (gap_data.width_mm < AUTOPARK_MIN_GAP_MM)) {
                app_autopark_init();
                break;
            }
            state.step_counter++;
            if (state.step_counter >= 50U) {
                state.state = PARK_STATE_STRAIGHTENING;
                state.step_counter = 0U;
            }
            break;
            
        case PARK_STATE_STRAIGHTENING:
            /* Check if gap is still valid, revert to scanning if lost */
            if (!gap_data.found || (gap_data.width_mm < AUTOPARK_MIN_GAP_MM)) {
                app_autopark_init();
                break;
            }
            state.step_counter++;
            if (state.step_counter >= 20U) {
                state.state = PARK_STATE_REVERSING_LEFT;
                state.step_counter = 0U;
            }
            break;
            
        case PARK_STATE_REVERSING_LEFT:
            /* Check if gap is still valid, revert to scanning if lost */
            if (!gap_data.found || (gap_data.width_mm < AUTOPARK_MIN_GAP_MM)) {
                app_autopark_init();
                break;
            }
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
    
    /* AUTOSAR SWS_BSW_00103: Separation of control logic and actuation
     * Map internal states to HMI prompt codes */
    switch (state.state) {
        case PARK_STATE_SCANNING:
            prompt_code = AUTOPARK_PROMPT_SCAN;
            break;
        case PARK_STATE_REVERSING_RIGHT:
        case PARK_STATE_STRAIGHTENING:
        case PARK_STATE_REVERSING_LEFT:
            prompt_code = AUTOPARK_PROMPT_ALIGN;
            break;
        case PARK_STATE_DONE:
            prompt_code = 0U;
            break;
        default:
            prompt_code = 0U;
            break;
    }
    
    /* AUTOSAR SWS_Rte_07140: HMI actuator control via port interface */
    hal_actuate_parking_prompt(prompt_code);
}