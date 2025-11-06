/**
 * @file Rte_App_Wipers.h
 * @brief RTE Generated Header for Wipers Software Component
 * @details AUTOSAR SWS_Rte_01167: Component-specific RTE API declarations
 *          Generated for App_Wipers Application Software Component
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @swc_arxml_mapping
 * SWC: App_Wipers
 * Runnable: App_Wipers_Step (TimingEvent: 100ms)
 * Ports:
 *   - R-Port: In_RainLevel (If_RainSensor)
 *   - P-Port: Out_WiperMode (If_WiperActuator)
 *   - R-Port: Time (If_TimeService) [C/S]
 * PIM:
 *   - State (Wipers_StateType)
 */

#ifndef RTE_APP_WIPERS_H
#define RTE_APP_WIPERS_H

#include "Rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Component Data Types
 ******************************************************************************/

/**
 * @brief Wiper mode enumeration
 */
typedef enum {
    WIPER_MODE_OFF  = 0U,         /**< Wipers off */
    WIPER_MODE_INT  = 1U,         /**< Intermittent mode */
    WIPER_MODE_LOW  = 2U,         /**< Low speed continuous */
    WIPER_MODE_HIGH = 3U          /**< High speed continuous */
} WiperModeType;

/**
 * @brief AUTOSAR SWS_Rte_01169: Per-Instance Memory (PIM) structure
 * @details State variables for Wipers functionality
 */
typedef struct {
    uint8 current_mode;           /**< Current wiper mode */
    uint8 debounce_counter;       /**< Debounce counter for mode changes */
    uint8 pending_mode;           /**< Pending mode awaiting debounce */
} Wipers_StateType;

/*******************************************************************************
 * RTE API: Sender/Receiver Ports (SWS_Rte_07139, SWS_Rte_07140)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_07139: Read rain level percentage
 * @details S/R Interface: If_RainSensor, Data Element: RainLevel
 *
 * @param[out] rain_pct Pointer to rain level percentage (0-100)
 * @return Std_ReturnType
 *   - RTE_E_OK: Data read successfully
 *   - RTE_E_NO_DATA: No data available
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety QM
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_RainLevel_RainLevel(
    P2VAR(uint8, AUTOMATIC, RTE_APPL_DATA) rain_pct
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read rain sensor timestamp
 *
 * @param[out] timestamp_ms Pointer to timestamp
 * @return Std_ReturnType
 *
 * @trace AUTOSAR SWS_Rte_07139
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_RainLevel_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07140: Write wiper mode command
 * @details S/R Interface: If_WiperActuator, Data Element: Mode
 *
 * @param[in] mode Wiper mode value (0-3)
 * @return Std_ReturnType
 *   - RTE_E_OK: Data written successfully
 *   - RTE_E_UNCONNECTED: Port not connected
 *
 * @trace AUTOSAR SWS_Rte_07140
 * @safety QM
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Write_Out_WiperMode_Mode(
    VAR(uint8, AUTOMATIC) mode
);

/*******************************************************************************
 * RTE API: Client/Server Ports (SWS_Rte_02001)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_02001: Get current system time
 * @details C/S Interface: If_TimeService, Operation: NowMs
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
 * @details Provides pointer to Wipers per-instance memory
 *
 * @return Pointer to Wipers state structure
 *
 * @trace AUTOSAR SWS_Rte_01169
 */
FUNC(P2VAR(Wipers_StateType, AUTOMATIC, RTE_APPL_DATA), RTE_CODE) Rte_Pim_State(void);

/*******************************************************************************
 * RTE API: Runnable Declarations (SWS_Rte_01167)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01167: Wipers cyclic runnable
 * @details Periodic runnable triggered by TimingEvent every 100ms
 *          Implements automatic wiper control based on rain sensor
 *
 * @timing Period: 100ms
 * @trace AUTOSAR SWS_Rte_01167
 * @safety QM
 */
FUNC(void, APP_WIPERS_CODE) App_Wipers_Step(void);

/**
 * @brief AUTOSAR SWS_Rte_02512: Wipers initialization runnable
 * @details Called during RTE initialization phase
 *
 * @trace AUTOSAR SWS_Rte_02512
 */
FUNC(void, APP_WIPERS_CODE) App_Wipers_Init(void);

/*******************************************************************************
 * RTE Configuration Constants
 ******************************************************************************/

/**
 * @brief Maximum data age threshold (ms) before considering stale
 */
#define RTE_WIPERS_MAX_DATA_AGE_MS      (100U)

/**
 * @brief RTE instance ID for Wipers component
 */
#define RTE_WIPERS_INSTANCE_ID          (5U)

#ifdef __cplusplus
}
#endif

#endif /* RTE_APP_WIPERS_H */
