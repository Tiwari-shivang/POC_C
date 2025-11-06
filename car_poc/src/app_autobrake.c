#include "app_autobrake.h"
#include "Rte_App_AutoBrake.h"
#include "calib.h"
#include "config.h"
#include "platform.h"
#include "eval_hooks.h"

/* AUTOSAR SWS_Rte_01169: Per-Instance Memory managed by RTE
 * State is now accessed via Rte_Pim_State() instead of static variable
 * This provides proper encapsulation and multi-instance support */

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Module initialization with deterministic state reset */
#define APP_AUTOBRAKE_START_SEC_CODE
#include "MemMap.h"

FUNC(void, APP_AUTOBRAKE_CODE) App_AutoBrake_Init(void) {
    /* AUTOSAR SWS_Rte_01169: Access Per-Instance Memory via RTE API */
    AutoBrake_StateType* state = Rte_Pim_State();

    state->hit_count = 0U;
    state->brake_active = FALSE;
    state->prev_below_thresh = FALSE;
}

/* Legacy wrapper for backward compatibility during transition */
void app_autobrake_init(void) {
    App_AutoBrake_Init();
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Cyclic function with deterministic behavior */
FUNC(void, APP_AUTOBRAKE_CODE) App_AutoBrake_Step(void) {
    /* AUTOSAR SWS_Rte_01169: Access Per-Instance Memory via RTE API */
    AutoBrake_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety-critical data */
    uint16 distance_mm = 0U;
    uint32 sensor_ts_ms = 0U;
    uint32 current_time_ms = 0U;
    boolean vehicle_ready = FALSE;
    boolean driver_brake = FALSE;
    boolean should_brake = FALSE;
    Std_ReturnType rte_status;

    /* AUTOSAR SWS_Rte_02001: Get current time via RTE C/S port */
    rte_status = Rte_Call_Time_NowMs(&current_time_ms);
    platform_assert(rte_status == RTE_E_OK);
    
    eval_loop_tick_begin(current_time_ms);

    /* AUTOSAR SWS_BSW_00134: Runtime assertion for defensive programming
     * MISRA Rule 16.5: Ensure critical state consistency */
    platform_assert(state->hit_count <= AUTOBRAKE_DEBOUNCE_COUNT);

    /* AUTOSAR SWS_BSW_00160: Check preconditions before processing
     * Vehicle readiness check via RTE S/R port */
    rte_status = Rte_Read_In_VehicleReady_Ready(&vehicle_ready);
    if ((rte_status != RTE_E_OK) || (vehicle_ready == FALSE)) {
        state->hit_count = 0U;
        state->brake_active = FALSE;
        (void)Rte_Write_Out_BrakeCmd_Brake(FALSE);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state->hit_count, state->brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }

    /* AUTOSAR SWS_BSW_00161: Driver override capability for safety
     * Manual brake input takes precedence via RTE S/R port */
    rte_status = Rte_Read_In_DriverBrake_BrakePressed(&driver_brake);
    if ((rte_status == RTE_E_OK) && (driver_brake != FALSE)) {
        state->hit_count = 0U;
        state->brake_active = FALSE;
        (void)Rte_Write_Out_BrakeCmd_Brake(FALSE);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state->hit_count, state->brake_active);
        eval_loop_tick_end(current_time_ms);
        return;
    }

    /* AUTOSAR SWS_Rte_07139: Data acquisition through RTE port interface */
    rte_status = Rte_Read_In_Distance_Distance(&distance_mm);

    /* AUTOSAR SWS_BSW_00167: Error handling for invalid sensor data
     * Fail-safe behavior when sensor communication fails */
    if (rte_status != RTE_E_OK) {
        state->hit_count = 0U;
        state->brake_active = FALSE;
        (void)Rte_Write_Out_BrakeCmd_Brake(FALSE);
        eval_autobrake_sample(current_time_ms, 0U, 0U, state->hit_count, state->brake_active);
        eval_loop_tick_end(current_time_ms);

        /* AUTOSAR SWS_Rte_02001: Report diagnostic event for sensor failure */
        if (rte_status == RTE_E_NO_DATA) {
            (void)Rte_Call_Diag_ReportEvent(AUTOBRAKE_EVENT_SENSOR_INVALID, 1U);
        }
        return;
    }

    /* AUTOSAR SWS_BSW_00170: Time supervision and plausibility checks
     * RTE already validates data age, but we read timestamp for eval logging */
    (void)Rte_Read_In_Distance_Timestamp(&sensor_ts_ms);

    /* AUTOSAR SWS_BSW_00104: Processed data derivation from raw sensor values */
    const uint32 sensor_age = (current_time_ms > sensor_ts_ms) ? (current_time_ms - sensor_ts_ms) : 0U;
    const boolean below = (distance_mm <= BRAKE_THRESH_MM);

    /* AUTOSAR SWS_Rte_02505: Event-driven processing for state transitions */
    if ((below != FALSE) && (state->prev_below_thresh == FALSE)) {
        eval_autobrake_event(EVAL_EVT_FIRST_BELOW_THRESH, current_time_ms);
    }
    state->prev_below_thresh = below;

    /* AUTOSAR SWS_BSW_00171: Debouncing mechanism for sensor signal filtering
     * Prevents spurious activations from transient sensor noise */
    if (below != FALSE) {
        if (state->hit_count < AUTOBRAKE_DEBOUNCE_COUNT) {
            state->hit_count++;
            if (state->hit_count == AUTOBRAKE_DEBOUNCE_COUNT) {
                eval_autobrake_event(EVAL_EVT_HAZARD_FLAG, current_time_ms);
            }
        }

        /* AUTOSAR SWS_BSW_00172: State machine implementation for control logic
         * Deterministic state transitions based on debounced inputs */
        if ((state->hit_count >= AUTOBRAKE_DEBOUNCE_COUNT) && (state->brake_active == FALSE)) {
            state->brake_active = TRUE;
            eval_autobrake_event(EVAL_EVT_BRAKE_ASSERT, current_time_ms);
            /* AUTOSAR SWS_Rte_02001: Report brake activation event */
            (void)Rte_Call_Diag_ReportEvent(AUTOBRAKE_EVENT_BRAKE_APPLIED, 1U);
        }
    } else {
        if (state->brake_active != FALSE) {
            state->brake_active = FALSE;
            eval_autobrake_event(EVAL_EVT_BRAKE_DEASSERT, current_time_ms);
        }
        state->hit_count = 0U;
    }

    /* AUTOSAR SWS_BSW_00103: Clear separation between decision and actuation */
    should_brake = state->brake_active;

    /* AUTOSAR SWS_Rte_07140: Actuator control through RTE port interface */
    (void)Rte_Write_Out_BrakeCmd_Brake(should_brake);

    /* AUTOSAR SWS_BSW_00202: Diagnostic data collection for monitoring
     * Record operational data for system health monitoring and diagnostics */
    eval_autobrake_sample(current_time_ms, distance_mm, sensor_age, state->hit_count, state->brake_active);
    eval_loop_tick_end(current_time_ms);
}

/* Legacy wrapper for backward compatibility during transition */
void app_autobrake_step(void) {
    App_AutoBrake_Step();
}

#define APP_AUTOBRAKE_STOP_SEC_CODE
#include "MemMap.h"