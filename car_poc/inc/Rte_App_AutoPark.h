/**
 * @file Rte_App_AutoPark.h
 * @brief RTE Generated Header for AutoPark Software Component
 * @details AUTOSAR SWS_Rte_01167: Component-specific RTE API declarations
 *          Generated for App_AutoPark Application Software Component
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @swc_arxml_mapping
 * SWC: App_AutoPark
 * Runnable: App_AutoPark_Step (TimingEvent: 20ms)
 * Ports:
 *   - R-Port: In_ParkingGap (If_ParkingGapSensor)
 *   - R-Port: In_VehicleSpeed (If_VehicleSpeed)
 *   - P-Port: Out_ParkingPrompt (If_ParkingPromptActuator)
 *   - R-Port: Time (If_TimeService) [C/S]
 * PIM:
 *   - State (AutoPark_StateType)
 */

#ifndef RTE_APP_AUTOPARK_H
#define RTE_APP_AUTOPARK_H

#include "Rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Component Data Types
 ******************************************************************************/

/**
 * @brief Parking gap sensor data structure
 */
typedef struct {
    boolean found;                /**< Gap detected flag */
    uint16 width_mm;              /**< Gap width in millimeters */
} ParkGapDataType;

/**
 * @brief AUTOSAR SWS_Rte_01169: Per-Instance Memory (PIM) structure
 * @details State variables for AutoPark functionality
 */
typedef struct {
    uint8 state;                  /**< Current parking state machine state */
    uint16 step_counter;          /**< Step counter for state progression */
    uint8 gap_detections;         /**< Debounce counter for gap detection */
    boolean gap_suitable;         /**< Gap suitability flag */
} AutoPark_StateType;

/*******************************************************************************
 * RTE API: Sender/Receiver Ports (SWS_Rte_07139, SWS_Rte_07140)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_07139: Read parking gap data
 * @details S/R Interface: If_ParkingGapSensor, Data Element: GapData
 *
 * @param[out] gap_data Pointer to receive parking gap structure
 * @return Std_ReturnType
 *   - RTE_E_OK: Data read successfully
 *   - RTE_E_NO_DATA: No data available
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety QM
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_ParkingGap_GapData(
    P2VAR(ParkGapDataType, AUTOMATIC, RTE_APPL_DATA) gap_data
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read parking gap timestamp
 *
 * @param[out] timestamp_ms Pointer to receive timestamp
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_ParkingGap_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read vehicle speed
 * @details S/R Interface: If_VehicleSpeed, Data Element: Speed
 *
 * @param[out] speed_kph Pointer to receive speed value
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_VehicleSpeed_Speed(
    P2VAR(uint16, AUTOMATIC, RTE_APPL_DATA) speed_kph
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read vehicle speed timestamp
 *
 * @param[out] timestamp_ms Pointer to receive timestamp
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_VehicleSpeed_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07140: Write parking prompt code
 * @details S/R Interface: If_ParkingPromptActuator, Data Element: PromptCode
 *
 * @param[in] prompt_code HMI prompt code for parking guidance
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07140
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Write_Out_ParkingPrompt_PromptCode(
    VAR(uint8, AUTOMATIC) prompt_code
);

/*******************************************************************************
 * RTE API: Client/Server Ports (SWS_Rte_02001)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_02001: Get current system time
 * @details C/S Interface: If_TimeService, Operation: NowMs
 *
 * @param[out] current_ms Pointer to receive current time
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
 * @return Pointer to AutoPark state structure
 *
 * @trace AUTOSAR SWS_Rte_01169
 */
FUNC(P2VAR(AutoPark_StateType, AUTOMATIC, RTE_APPL_DATA), RTE_CODE) Rte_Pim_State(void);

/*******************************************************************************
 * RTE API: Runnable Declarations (SWS_Rte_01167)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01167: AutoPark cyclic runnable
 *
 * @timing Period: 20ms
 * @trace AUTOSAR SWS_Rte_01167
 */
FUNC(void, APP_AUTOPARK_CODE) App_AutoPark_Step(void);

/**
 * @brief AUTOSAR SWS_Rte_02512: AutoPark initialization runnable
 *
 * @trace AUTOSAR SWS_Rte_02512
 */
FUNC(void, APP_AUTOPARK_CODE) App_AutoPark_Init(void);

/*******************************************************************************
 * RTE Configuration Constants
 ******************************************************************************/

#define RTE_AUTOPARK_MAX_DATA_AGE_MS    (100U)
#define RTE_AUTOPARK_INSTANCE_ID        (3U)

#ifdef __cplusplus
}
#endif

#endif /* RTE_APP_AUTOPARK_H */
