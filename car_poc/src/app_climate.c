/* AUTOSAR SWS_Rte_01167: RTE-based Climate Control Application Component
 * QM (Quality Management) - Comfort feature with PI controller
 * Task Period: 50ms (20Hz) for temperature control loop
 * Function: Multi-zone climate control with humidity compensation */

#include "Rte_App_Climate.h"
#include "calib.h"
#include "config.h"
#include "platform.h"

/* AUTOSAR SWS_BSW_00171: PI controller configuration */
#define MAX_BLEND_PCT (100U)
#define INTEGRAL_CLAMP_MAX (1000)
#define INTEGRAL_CLAMP_MIN (-1000)
#define PI_OUTPUT_MAX (300)
#define PI_OUTPUT_MIN (-300)

/* AUTOSAR SWS_BSW_00158: Diagnostic event IDs for climate control */
#define CLIMATE_EVENT_CABIN_TEMP_INVALID    (0x08U)
#define CLIMATE_EVENT_AMBIENT_TEMP_INVALID  (0x09U)
#define CLIMATE_EVENT_HUMIDITY_INVALID      (0x0AU)

/* AUTOSAR SWS_MemMap_00003: Memory section for initialization code */
#define APP_CLIMATE_START_SEC_CODE
#include "MemMap.h"

/* AUTOSAR SWS_BSW_00101: Initialization function following Init/DeInit pattern
 * AUTOSAR SWS_Rte_02512: Module initialization with safe default values */
FUNC(void, APP_CLIMATE_CODE) App_Climate_Init(void) {
    Climate_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Initialize all state variables to safe defaults */
    platform_assert(state != NULL_PTR);

    state->setpoint_x10 = CLIMATE_TARGET_C_X10;
    state->integral_accumulator = 0;
    state->last_update_ms = 0U;
    state->current_fan_stage = 0U;
    state->current_ac_on = FALSE;
    state->current_blend_pct = 50U;
}

/* AUTOSAR SWS_BSW_00105: Helper function for PI output to fan stage mapping
 * C_CLIMATE_LIMITS control implementation */
static uint8 map_pi_output_to_fan_stage(int32 pi_output) {
    int32 abs_output = (pi_output < 0) ? (-pi_output) : pi_output;
    uint8 fan_stage = 0U;

    if (abs_output > 200) {
        fan_stage = FAN_STAGE_MAX;
    } else if (abs_output > 100) {
        fan_stage = 2U;
    } else if (abs_output > 50) {
        fan_stage = 1U;
    } else {
        fan_stage = 0U;
    }

    return fan_stage;
}

/* AUTOSAR SWS_BSW_00105: Helper function for blend door calculation */
static uint8 calculate_blend_percentage(int32 pi_output) {
    uint8 blend_pct = 50U;

    if (pi_output > 0) {
        blend_pct = 100U;
    } else if (pi_output < -50) {
        blend_pct = 0U;
    } else {
        blend_pct = 50U;
    }

    return blend_pct;
}

/* AUTOSAR SWS_Rte_01167: Runnable entity for periodic task execution
 * AUTOSAR SWS_BSW_00037: Cyclic function with PI controller
 * Task Period: 50ms (20Hz) for climate control loop
 * QM: Climate Control with multi-sensor inputs */
FUNC(void, APP_CLIMATE_CODE) App_Climate_Step(void) {
    /* AUTOSAR SWS_Rte_01169: Access Per-Instance Memory for state management */
    Climate_StateType* state = Rte_Pim_State();

    /* AUTOSAR SWS_BSW_00115: Local variable initialization for safety */
    int16 cabin_temp_x10 = 0;
    int16 ambient_temp_x10 = 0;
    uint8 humidity_pct = 0U;
    uint32 current_time_ms = 0U;
    int32 error_x10 = 0;
    int32 pi_output = 0;
    uint32 dt_ms = 0U;
    boolean ac_required = FALSE;
    ClimateControl_DataType climate_cmd;
    Std_ReturnType rte_status;

    /* AUTOSAR SWS_Rte_02588: Runtime assertion for PIM validity */
    platform_assert(state != NULL_PTR);

    /* AUTOSAR SWS_Rte_07139: Get current time through RTE Client/Server port */
    rte_status = Rte_Call_Time_NowMs(&current_time_ms);
    platform_assert(rte_status == RTE_E_OK);

    /* AUTOSAR SWS_Rte_07139: Read cabin temperature through RTE port */
    rte_status = Rte_Read_In_CabinTemp_Temperature(&cabin_temp_x10);
    if (rte_status != RTE_E_OK) {
        /* Cabin temperature read failure - enter safe state */
        state->current_fan_stage = 0U;
        state->current_ac_on = FALSE;
        state->current_blend_pct = 50U;

        climate_cmd.fan_stage = state->current_fan_stage;
        climate_cmd.ac_on = state->current_ac_on;
        climate_cmd.blend_pct = state->current_blend_pct;
        (void)Rte_Write_Out_ClimateControl_Control(&climate_cmd);

        if (rte_status == RTE_E_NO_DATA) {
            (void)Rte_Call_Diag_ReportEvent(CLIMATE_EVENT_CABIN_TEMP_INVALID, 1U);
        }
        return;
    }

    /* AUTOSAR SWS_Rte_07139: Read ambient temperature (optional sensor) */
    rte_status = Rte_Read_In_AmbientTemp_Temperature(&ambient_temp_x10);
    /* Note: Ambient temp is optional, continue if not available */

    /* AUTOSAR SWS_Rte_07139: Read humidity (optional sensor) */
    (void)Rte_Read_In_Humidity_Humidity(&humidity_pct);
    /* Note: Humidity is optional, continue if not available */
    
    /* AUTOSAR SWS_BSW_00171: Update timing for PI controller */
    dt_ms = (state->last_update_ms == 0U) ? CONTROL_DT_MS : (current_time_ms - state->last_update_ms);

    if (dt_ms < CONTROL_DT_MS) {
        /* Not enough time elapsed - output previous command */
        climate_cmd.fan_stage = state->current_fan_stage;
        climate_cmd.ac_on = state->current_ac_on;
        climate_cmd.blend_pct = state->current_blend_pct;
        (void)Rte_Write_Out_ClimateControl_Control(&climate_cmd);
        return;
    }

    state->last_update_ms = current_time_ms;

    /* AUTOSAR SWS_BSW_00174: PI controller implementation
     * Calculate error: setpoint - actual */
    error_x10 = state->setpoint_x10 - cabin_temp_x10;

    /* Integral term accumulation with anti-windup */
    state->integral_accumulator += ((int32)error_x10 * CLIMATE_KI);

    /* AUTOSAR SWS_BSW_00171: Integral clamping for stability */
    if (state->integral_accumulator > INTEGRAL_CLAMP_MAX) {
        state->integral_accumulator = INTEGRAL_CLAMP_MAX;
    } else if (state->integral_accumulator < INTEGRAL_CLAMP_MIN) {
        state->integral_accumulator = INTEGRAL_CLAMP_MIN;
    }

    /* PI controller output: Kp*error + integral */
    pi_output = ((int32)error_x10 * CLIMATE_KP) + state->integral_accumulator;

    /* Output clamping with integral back-calculation */
    if (pi_output > PI_OUTPUT_MAX) {
        pi_output = PI_OUTPUT_MAX;
        state->integral_accumulator -= ((int32)error_x10 * CLIMATE_KI);
    } else if (pi_output < PI_OUTPUT_MIN) {
        pi_output = PI_OUTPUT_MIN;
        state->integral_accumulator -= ((int32)error_x10 * CLIMATE_KI);
    }

    /* AUTOSAR SWS_BSW_00103: Map PI output to actuator commands */
    state->current_fan_stage = map_pi_output_to_fan_stage(pi_output);
    state->current_blend_pct = calculate_blend_percentage(pi_output);

    /* AUTOSAR SWS_BSW_00105: AC compressor activation logic
     * C_CLIMATE_LIMITS control implementation */
    ac_required = FALSE;

    /* AC required if cabin significantly warmer than setpoint */
    if (error_x10 < -20) {
        ac_required = TRUE;
    }

    /* AC required if high humidity detected */
    if (humidity_pct > CLIMATE_HUMIDITY_HIGH_PCT) {
        ac_required = TRUE;
    }

    /* AC required if ambient temperature significantly above setpoint */
    if (rte_status == RTE_E_OK) {
        if (ambient_temp_x10 > (state->setpoint_x10 + 50)) {
            ac_required = TRUE;
        }
    }

    state->current_ac_on = ac_required;

    /* AUTOSAR SWS_Rte_07140: Actuator control through RTE port */
    climate_cmd.fan_stage = state->current_fan_stage;
    climate_cmd.ac_on = state->current_ac_on;
    climate_cmd.blend_pct = state->current_blend_pct;
    (void)Rte_Write_Out_ClimateControl_Control(&climate_cmd);
}

/* AUTOSAR SWS_MemMap_00003: Close memory section for code */
#define APP_CLIMATE_STOP_SEC_CODE
#include "MemMap.h"