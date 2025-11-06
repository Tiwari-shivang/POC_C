/* AUTOSAR SWS_Rte_01167: RTE-based Auto Park Application Component
 * QM (Quality Management) - Non-safety-critical parking assistance
 * Task Period: 20ms (50Hz) for parking sensor monitoring
 * Function: Automated parallel parking with gap detection */

#include "Rte_App_AutoPark.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

/* AUTOSAR SWS_BSW_00158: Diagnostic event IDs for auto park */
#define AUTOPARK_EVENT_SENSOR_INVALID     (0x05U)
#define AUTOPARK_EVENT_SPEED_UNSAFE       (0x06U)
#define AUTOPARK_EVENT_GAP_LOST           (0x07U)

/* AUTOSAR SWS_MemMap_00003: Memory section for initialization code */
#define APP_AUTOPARK_START_SEC_CODE
#include "MemMap.h"

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Deterministic state reset to known safe values */
FUNC(void, APP_AUTOPARK_CODE) App_AutoPark_Init(void) {
    AutoPark_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Initialize all state variables to safe defaults */
    platform_assert(state != NULL_PTR);

    state->state = PARK_STATE_SCANNING;
    state->step_counter = 0U;
    state->gap_detections = 0U;
    state->gap_suitable = FALSE;
}

/* AUTOSAR SWS_BSW_00105: Helper function for safety condition evaluation
 * Validates vehicle speed is within safe operational envelope for parking */
static boolean is_speed_suitable_for_parking(void) {
    uint16 speed_kph = 0U;
    Std_ReturnType rte_status;

    /* AUTOSAR SWS_Rte_07139: Data acquisition through RTE port interface */
    rte_status = Rte_Read_In_VehicleSpeed_Speed(&speed_kph);

    if (rte_status != RTE_E_OK) {
        /* Sensor read failure or stale data */
        return FALSE;
    }

    /* AUTOSAR SWS_BSW_00160: Safety condition check */
    return (speed_kph <= AUTOPARK_MAX_SPEED_KPH);
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Deterministic cyclic processing function
 * Task Period: 20ms (50Hz) for parking gap detection
 * QM: Auto Park with parking gap sensor interface */
FUNC(void, APP_AUTOPARK_CODE) App_AutoPark_Step(void) {
    /* AUTOSAR SWS_Rte_01169: Access Per-Instance Memory for state management */
    AutoPark_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety */
    ParkingGap_DataType gap_data;
    uint8 prompt_code = 0U;
    Std_ReturnType rte_status;

    /* AUTOSAR SWS_Rte_02588: Runtime assertion for PIM validity */
    platform_assert(state != NULL_PTR);

    /* AUTOSAR SWS_BSW_00160: Precondition validation before processing
     * Ensure vehicle speed is safe for parking maneuver */
    if (!is_speed_suitable_for_parking()) {
        /* Speed too high - reset to scanning state */
        App_AutoPark_Init();
        (void)Rte_Write_Out_ParkingPrompt_PromptCode(0U);
        (void)Rte_Call_Diag_ReportEvent(AUTOPARK_EVENT_SPEED_UNSAFE, 1U);
        return;
    }

    /* AUTOSAR SWS_Rte_07139: Sensor data acquisition via RTE port interface */
    rte_status = Rte_Read_In_ParkingGap_GapData(&gap_data);

    /* AUTOSAR SWS_BSW_00167: Error handling for invalid/stale sensor data */
    if (rte_status != RTE_E_OK) {
        /* Sensor read failure - reset if not in scanning state */
        if (state->state != PARK_STATE_SCANNING) {
            App_AutoPark_Init();
        }
        (void)Rte_Write_Out_ParkingPrompt_PromptCode(0U);

        if (rte_status == RTE_E_NO_DATA) {
            (void)Rte_Call_Diag_ReportEvent(AUTOPARK_EVENT_SENSOR_INVALID, 1U);
        }
        return;
    }
    
    /* AUTOSAR SWS_BSW_00172: State machine implementation
     * AUTOSAR SWS_Rte_02505: Event-driven state transitions
     * C_AUTOPARK_SAFETY control implementation */
    switch (state->state) {
        case PARK_STATE_SCANNING:
            /* AUTOSAR SWS_BSW_00171: Debouncing for robust gap detection */
            if (gap_data.found && (gap_data.width_mm >= AUTOPARK_MIN_GAP_MM)) {
                state->gap_detections++;
                if (state->gap_detections >= AUTOPARK_DEBOUNCE_COUNT) {
                    state->gap_suitable = TRUE;
                    state->state = PARK_STATE_REVERSING_RIGHT;
                    state->step_counter = 0U;
                }
            } else {
                state->gap_detections = 0U;
                state->gap_suitable = FALSE;
            }
            break;

        case PARK_STATE_REVERSING_RIGHT:
            /* AUTOSAR SWS_BSW_00168: Continuous monitoring of safety conditions
             * Abort maneuver if parking gap becomes invalid */
            if (!gap_data.found || (gap_data.width_mm < AUTOPARK_MIN_GAP_MM)) {
                App_AutoPark_Init();
                (void)Rte_Call_Diag_ReportEvent(AUTOPARK_EVENT_GAP_LOST, 1U);
                break;
            }
            state->step_counter++;
            if (state->step_counter >= 50U) {
                state->state = PARK_STATE_STRAIGHTENING;
                state->step_counter = 0U;
            }
            break;

        case PARK_STATE_STRAIGHTENING:
            /* Check if gap is still valid, revert to scanning if lost */
            if (!gap_data.found || (gap_data.width_mm < AUTOPARK_MIN_GAP_MM)) {
                App_AutoPark_Init();
                (void)Rte_Call_Diag_ReportEvent(AUTOPARK_EVENT_GAP_LOST, 1U);
                break;
            }
            state->step_counter++;
            if (state->step_counter >= 20U) {
                state->state = PARK_STATE_REVERSING_LEFT;
                state->step_counter = 0U;
            }
            break;

        case PARK_STATE_REVERSING_LEFT:
            /* Check if gap is still valid, revert to scanning if lost */
            if (!gap_data.found || (gap_data.width_mm < AUTOPARK_MIN_GAP_MM)) {
                App_AutoPark_Init();
                (void)Rte_Call_Diag_ReportEvent(AUTOPARK_EVENT_GAP_LOST, 1U);
                break;
            }
            state->step_counter++;
            if (state->step_counter >= 50U) {
                state->state = PARK_STATE_DONE;
                state->step_counter = 0U;
            }
            break;

        case PARK_STATE_DONE:
            /* Parking complete - maintain current state */
            break;

        default:
            /* Invalid state - reset to scanning */
            App_AutoPark_Init();
            break;
    }

    /* AUTOSAR SWS_BSW_00103: Separation of control logic and actuation
     * Map internal states to HMI prompt codes */
    switch (state->state) {
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

    /* AUTOSAR SWS_Rte_07140: HMI actuator control via RTE port interface */
    (void)Rte_Write_Out_ParkingPrompt_PromptCode(prompt_code);
}

/* AUTOSAR SWS_MemMap_00003: Close memory section for code */
#define APP_AUTOPARK_STOP_SEC_CODE
#include "MemMap.h"