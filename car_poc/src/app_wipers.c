/* AUTOSAR SWS_Rte_01167: RTE-based Wipers Application Component
 * QM (Quality Management) - Comfort feature with rain sensor
 * Task Period: 100ms (10Hz) for wiper control
 * Function: Automatic rain-sensing wiper control with hysteresis */

#include "Rte_App_Wipers.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

/* AUTOSAR SWS_BSW_00174: Hysteresis thresholds for rain sensor
 * C_WIPERS_DEBOUNCE control implementation */
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

/* AUTOSAR SWS_BSW_00158: Diagnostic event IDs for wipers */
#define WIPERS_EVENT_SENSOR_INVALID       (0x0BU)
#define WIPERS_EVENT_DATA_STALE           (0x0CU)

/* AUTOSAR SWS_MemMap_00003: Memory section for initialization code */
#define APP_WIPERS_START_SEC_CODE
#include "MemMap.h"

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Module initialization with safe default values */
FUNC(void, APP_WIPERS_CODE) App_Wipers_Init(void) {
    Wipers_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Initialize all state variables to safe defaults */
    platform_assert(state != NULL_PTR);

    state->current_mode = WIPER_MODE_OFF;
    state->debounce_counter = 0U;
    state->pending_mode = WIPER_MODE_OFF;
}

/* AUTOSAR SWS_BSW_00105: Helper function for wiper mode determination
 * AUTOSAR SWS_BSW_00174: Hysteresis implementation for stable transitions
 * C_WIPERS_DEBOUNCE control implementation */
static uint8 determine_wiper_mode(uint8 rain_pct, uint8 current_mode) {
    uint8 new_mode = current_mode;

    /* State machine with hysteresis for stable mode transitions */
    if (current_mode == WIPER_MODE_OFF) {
        /* OFF state - check if rain exceeds intermittent ON threshold */
        if (rain_pct >= RAIN_THR_INT_ON_PCT) {
            new_mode = WIPER_MODE_INT;
        }
    } else if (current_mode == WIPER_MODE_INT) {
        /* INTERMITTENT state - check for OFF or LOW transition */
        if (rain_pct < RAIN_THR_INT_OFF_PCT) {
            new_mode = WIPER_MODE_OFF;
        } else if (rain_pct >= RAIN_THR_LOW_ON_PCT) {
            new_mode = WIPER_MODE_LOW;
        }
    } else if (current_mode == WIPER_MODE_LOW) {
        /* LOW state - check for INT or HIGH transition */
        if (rain_pct < RAIN_THR_LOW_OFF_PCT) {
            new_mode = WIPER_MODE_INT;
        } else if (rain_pct >= RAIN_THR_HIGH_ON_PCT) {
            new_mode = WIPER_MODE_HIGH;
        }
    } else if (current_mode == WIPER_MODE_HIGH) {
        /* HIGH state - check for LOW transition */
        if (rain_pct < RAIN_THR_HIGH_OFF_PCT) {
            new_mode = WIPER_MODE_LOW;
        }
    } else {
        /* Invalid state - default to OFF */
        new_mode = WIPER_MODE_OFF;
    }

    return new_mode;
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Cyclic function with deterministic behavior
 * Task Period: 100ms (10Hz) for wiper control
 * QM: Wipers with rain sensor interface */
FUNC(void, APP_WIPERS_CODE) App_Wipers_Step(void) {
    /* AUTOSAR SWS_Rte_01169: Access Per-Instance Memory for state management */
    Wipers_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety */
    uint8 rain_pct = 0U;
    uint8 new_mode = WIPER_MODE_OFF;
    Std_ReturnType rte_status;

    /* AUTOSAR SWS_Rte_02588: Runtime assertion for PIM validity */
    platform_assert(state != NULL_PTR);

    /* AUTOSAR SWS_Rte_07139: Read rain level through RTE port */
    rte_status = Rte_Read_In_RainLevel_RainLevel(&rain_pct);

    /* AUTOSAR SWS_BSW_00167: Error handling for invalid/stale sensor data */
    if (rte_status != RTE_E_OK) {
        /* Sensor read failure - enter safe state (wipers OFF) */
        state->current_mode = WIPER_MODE_OFF;
        state->debounce_counter = 0U;
        state->pending_mode = WIPER_MODE_OFF;
        (void)Rte_Write_Out_WiperMode_Mode(state->current_mode);

        if (rte_status == RTE_E_NO_DATA) {
            (void)Rte_Call_Diag_ReportEvent(WIPERS_EVENT_SENSOR_INVALID, 1U);
        } else if (rte_status == RTE_E_MAX_AGE_EXCEEDED) {
            (void)Rte_Call_Diag_ReportEvent(WIPERS_EVENT_DATA_STALE, 1U);
        }
        return;
    }

    /* AUTOSAR SWS_BSW_00174: Determine new wiper mode with hysteresis */
    new_mode = determine_wiper_mode(rain_pct, state->current_mode);

    /* AUTOSAR SWS_BSW_00171: Debouncing mechanism for robust mode changes
     * C_WIPERS_DEBOUNCE control implementation */
    if (new_mode == state->pending_mode) {
        /* Same mode requested - increment debounce counter */
        if (state->debounce_counter < WIPERS_DEBOUNCE_COUNT) {
            state->debounce_counter++;
        }

        /* Debounce count reached - commit mode change */
        if (state->debounce_counter >= WIPERS_DEBOUNCE_COUNT) {
            state->current_mode = new_mode;
        }
    } else {
        /* Different mode requested - reset debounce counter */
        state->pending_mode = new_mode;
        state->debounce_counter = 1U;
    }

    /* AUTOSAR SWS_Rte_07140: Actuator control through RTE port */
    (void)Rte_Write_Out_WiperMode_Mode(state->current_mode);
}

/* AUTOSAR SWS_MemMap_00003: Close memory section for code */
#define APP_WIPERS_STOP_SEC_CODE
#include "MemMap.h"