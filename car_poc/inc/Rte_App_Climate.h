/**
 * @file Rte_App_Climate.h
 * @brief RTE Generated Header for Climate Control Software Component
 * @details AUTOSAR SWS_Rte_01167: Component-specific RTE API declarations
 *          Generated for App_Climate Application Software Component
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @swc_arxml_mapping
 * SWC: App_Climate
 * Runnable: App_Climate_Step (TimingEvent: 50ms)
 * Ports:
 *   - R-Port: In_CabinTemp (If_TemperatureSensor)
 *   - R-Port: In_AmbientTemp (If_TemperatureSensor)
 *   - R-Port: In_Humidity (If_HumiditySensor)
 *   - P-Port: Out_ClimateControl (If_ClimateActuator)
 *   - R-Port: Time (If_TimeService) [C/S]
 * PIM:
 *   - State (Climate_StateType)
 */

#ifndef RTE_APP_CLIMATE_H
#define RTE_APP_CLIMATE_H

#include "Rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Component Data Types
 ******************************************************************************/

/**
 * @brief Climate control actuator command structure
 */
typedef struct {
    uint8 fan_stage;              /**< Fan speed stage (0-4) */
    boolean ac_on;                /**< Air conditioning active */
    uint8 blend_pct;              /**< Temperature blend door position (0-100%) */
} ClimateControlDataType;

/**
 * @brief AUTOSAR SWS_Rte_01169: Per-Instance Memory (PIM) structure
 * @details State variables for Climate control functionality
 */
typedef struct {
    sint16 setpoint_x10;          /**< Target temperature * 10 (degC) */
    sint32 integral_accumulator;  /**< PI controller integral term */
    uint32 last_update_ms;        /**< Last control update timestamp */
    uint8 current_fan_stage;      /**< Current fan stage */
    boolean current_ac_on;        /**< Current AC status */
    uint8 current_blend_pct;      /**< Current blend door position */
} Climate_StateType;

/*******************************************************************************
 * RTE API: Sender/Receiver Ports (SWS_Rte_07139, SWS_Rte_07140)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_07139: Read cabin temperature
 * @details S/R Interface: If_TemperatureSensor, Data Element: Temperature
 *
 * @param[out] temp_x10 Pointer to temperature * 10 (degC)
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety QM
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_CabinTemp_Temperature(
    P2VAR(sint16, AUTOMATIC, RTE_APPL_DATA) temp_x10
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read cabin temperature timestamp
 *
 * @param[out] timestamp_ms Pointer to timestamp
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_CabinTemp_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read ambient temperature
 *
 * @param[out] temp_x10 Pointer to temperature * 10 (degC)
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_AmbientTemp_Temperature(
    P2VAR(sint16, AUTOMATIC, RTE_APPL_DATA) temp_x10
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read ambient temperature timestamp
 *
 * @param[out] timestamp_ms Pointer to timestamp
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_AmbientTemp_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read humidity percentage
 *
 * @param[out] humidity_pct Pointer to humidity percentage (0-100)
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_Humidity_Humidity(
    P2VAR(uint8, AUTOMATIC, RTE_APPL_DATA) humidity_pct
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read humidity timestamp
 *
 * @param[out] timestamp_ms Pointer to timestamp
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_Humidity_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07140: Write climate control commands
 * @details S/R Interface: If_ClimateActuator, Data Element: Control
 *
 * @param[in] fan_stage Fan speed stage (0-4)
 * @param[in] ac_on Air conditioning active flag
 * @param[in] blend_pct Temperature blend percentage (0-100)
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07140
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Write_Out_ClimateControl_Control(
    VAR(uint8, AUTOMATIC) fan_stage,
    VAR(boolean, AUTOMATIC) ac_on,
    VAR(uint8, AUTOMATIC) blend_pct
);

/*******************************************************************************
 * RTE API: Client/Server Ports (SWS_Rte_02001)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_02001: Get current system time
 *
 * @param[out] current_ms Pointer to current time
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_02001
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Call_Time_NowMs(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) current_ms
);

/*******************************************************************************
 * RTE API: Per-Instance Memory (SWS_Rte_01169)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01169: Access PIM state data
 *
 * @return Pointer to Climate state structure
 *
 * @trace AUTOSAR SWS_Rte_01169
 */
FUNC(P2VAR(Climate_StateType, AUTOMATIC, RTE_APPL_DATA), RTE_CODE) Rte_Pim_State(void);

/*******************************************************************************
 * RTE API: Runnable Declarations (SWS_Rte_01167)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01167: Climate cyclic runnable
 *
 * @timing Period: 50ms
 * @trace AUTOSAR SWS_Rte_01167
 */
FUNC(void, APP_CLIMATE_CODE) App_Climate_Step(void);

/**
 * @brief AUTOSAR SWS_Rte_02512: Climate initialization runnable
 *
 * @trace AUTOSAR SWS_Rte_02512
 */
FUNC(void, APP_CLIMATE_CODE) App_Climate_Init(void);

/*******************************************************************************
 * RTE Configuration Constants
 ******************************************************************************/

#define RTE_CLIMATE_MAX_DATA_AGE_MS     (200U)
#define RTE_CLIMATE_INSTANCE_ID         (4U)

#ifdef __cplusplus
}
#endif

#endif /* RTE_APP_CLIMATE_H */
