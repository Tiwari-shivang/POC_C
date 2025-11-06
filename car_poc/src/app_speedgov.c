/* AUTOSAR SWS_Rte_01167: RTE-based Speed Governor Application Component
 * ASIL-B compliance with configurable speed limit enforcement
 * Task Period: 10ms (100Hz) for real-time speed monitoring
 * Safety Function: Overspeed alarm with hysteresis and debouncing */

#include "Rte_App_SpeedGov.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

/* AUTOSAR SWS_BSW_00171: Safety-critical configuration parameters */
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

/* AUTOSAR SWS_BSW_00158: Diagnostic event IDs for speed governor */
#define SPEEDGOV_EVENT_SENSOR_INVALID     (0x02U)
#define SPEEDGOV_EVENT_DATA_STALE         (0x03U)
#define SPEEDGOV_EVENT_CONFIG_ERROR       (0x04U)

/* AUTOSAR SWS_MemMap_00003: Memory section for initialization code */
#define APP_SPEEDGOV_START_SEC_CODE
#include "MemMap.h"

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Module initialization with safe default values
 * This function initializes the Speed Governor PIM to safe defaults */
FUNC(void, APP_SPEEDGOV_CODE) App_SpeedGov_Init(void) {
    SpeedGov_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Initialize all state variables to safe defaults */
    platform_assert(state != NULL_PTR);

    state->current_limit_kph = SPEEDGOV_DEFAULT_LIMIT_KPH;
    state->overspeed_count = 0U;
    state->alarm_active = FALSE;
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Cyclic function with deterministic behavior
 * Task Period: 10ms (100Hz) for responsive speed monitoring
 * ASIL-B: Speed Governor with configurable limits and overspeed detection */
FUNC(void, APP_SPEEDGOV_CODE) App_SpeedGov_Step(void) {
    /* AUTOSAR SWS_Rte_01169: Access Per-Instance Memory for state management */
    SpeedGov_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety-critical data */
    uint16 vehicle_speed_kph = 0U;
    uint32 current_time_ms = 0U;
    uint16 new_limit = 0U;
    uint16 overspeed_threshold = 0U;
    uint16 clear_threshold = 0U;
    boolean should_alarm = FALSE;
    Std_ReturnType rte_status;

    /* AUTOSAR SWS_Rte_02588: Runtime assertion for PIM validity */
    platform_assert(state != NULL_PTR);

    /* AUTOSAR SWS_Rte_07139: Get current time through RTE Client/Server port */
    rte_status = Rte_Call_Time_NowMs(&current_time_ms);
    platform_assert(rte_status == RTE_E_OK);

    /* AUTOSAR SWS_Rte_07139: Dynamic configuration update via Client/Server port
     * AUTOSAR SWS_BSW_00106: Runtime parameter update with validation */
    rte_status = Rte_Call_Config_TryGetSpeedLimit(&new_limit);
    if ((rte_status == RTE_E_OK) && (new_limit > 0U)) {
        /* Valid speed limit configuration received - update state */
        state->current_limit_kph = new_limit;
        state->overspeed_count = 0U;
        state->alarm_active = FALSE;
    }

    /* AUTOSAR SWS_Rte_07139: Sensor data acquisition through Sender/Receiver port */
    rte_status = Rte_Read_In_VehicleSpeed_Speed(&vehicle_speed_kph);

    /* AUTOSAR SWS_BSW_00167: Error handling for invalid sensor data
     * Safe state transition when speed reading fails */
    if (rte_status != RTE_E_OK) {
        /* Sensor read failure - report diagnostic event and enter safe state */
        if (rte_status == RTE_E_NO_DATA) {
            (void)Rte_Call_Diag_ReportEvent(SPEEDGOV_EVENT_SENSOR_INVALID, 1U);
        } else if (rte_status == RTE_E_MAX_AGE_EXCEEDED) {
            /* AUTOSAR SWS_BSW_00170: Stale data detection */
            (void)Rte_Call_Diag_ReportEvent(SPEEDGOV_EVENT_DATA_STALE, 1U);
        }

        /* Enter safe state: disable alarm, clear debounce counter */
        state->overspeed_count = 0U;
        state->alarm_active = FALSE;
        (void)Rte_Write_Out_Alarm_Alarm(FALSE);
        return;
    }

    /* AUTOSAR SWS_BSW_00174: Hysteresis implementation for stability
     * Prevents oscillation around threshold values
     * C_SPEEDGOV_HYSTERESIS control implementation */
    overspeed_threshold = state->current_limit_kph;
    clear_threshold = state->current_limit_kph - SPEEDGOV_HYSTERESIS_KPH;

    /* AUTOSAR SWS_BSW_00172: State machine for alarm management
     * AUTOSAR SWS_BSW_00171: Debouncing mechanism for robust detection */
    if (state->alarm_active) {
        /* Currently in alarm state - check if speed has dropped below clear threshold */
        if (vehicle_speed_kph <= clear_threshold) {
            state->alarm_active = FALSE;
            state->overspeed_count = 0U;
        }
    } else {
        /* Not in alarm state - check if speed exceeds threshold */
        if (vehicle_speed_kph > overspeed_threshold) {
            /* Speed exceeds limit - increment debounce counter */
            if (state->overspeed_count < SPEEDGOV_DEBOUNCE_COUNT) {
                state->overspeed_count++;
            }

            /* Activate alarm if debounce count reached */
            if (state->overspeed_count >= SPEEDGOV_DEBOUNCE_COUNT) {
                state->alarm_active = TRUE;
            }
        } else {
            /* Speed within limits - reset debounce counter */
            state->overspeed_count = 0U;
        }
    }

    /* AUTOSAR SWS_BSW_00103: Separation between decision and actuation */
    should_alarm = state->alarm_active;

    /* AUTOSAR SWS_Rte_07140: Actuator control through standardized RTE ports
     * Update alarm state through Sender/Receiver port */
    (void)Rte_Write_Out_Alarm_Alarm(should_alarm);
}

/* AUTOSAR SWS_MemMap_00003: Close memory section for code */
#define APP_SPEEDGOV_STOP_SEC_CODE
#include "MemMap.h"