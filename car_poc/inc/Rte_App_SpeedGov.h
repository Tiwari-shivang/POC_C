/**
 * @file Rte_App_SpeedGov.h
 * @brief RTE Generated Header for SpeedGovernor Software Component
 * @details AUTOSAR SWS_Rte_01167: Component-specific RTE API declarations
 *          Generated for App_SpeedGov Application Software Component
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @swc_arxml_mapping
 * SWC: App_SpeedGov
 * Runnable: App_SpeedGov_Step (TimingEvent: 10ms)
 * Ports:
 *   - R-Port: In_VehicleSpeed (If_VehicleSpeed)
 *   - P-Port: Out_Alarm (If_AlarmStatus)
 *   - P-Port: Out_SpeedLimit (If_SpeedLimitRequest)
 *   - R-Port: Config (If_ConfigService) [C/S]
 *   - R-Port: Time (If_TimeService) [C/S]
 *   - R-Port: Diag (If_DiagnosticEvents) [C/S]
 * PIM:
 *   - State (SpeedGov_StateType)
 */

#ifndef RTE_APP_SPEEDGOV_H
#define RTE_APP_SPEEDGOV_H

#include "Rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Component Data Types
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01169: Per-Instance Memory (PIM) structure
 * @details State variables for SpeedGovernor functionality
 */
typedef struct {
    uint16 current_limit_kph;     /**< Active speed limit in km/h */
    uint8 overspeed_count;        /**< Debounce counter for overspeed detection */
    boolean alarm_active;         /**< Current alarm status */
} SpeedGov_StateType;

/**
 * @brief Vehicle speed sensor data structure
 */
typedef struct {
    uint16 speed_kph;             /**< Vehicle speed in km/h */
    uint32 timestamp_ms;          /**< Sensor timestamp in milliseconds */
    boolean is_valid;             /**< Data validity flag */
} VehicleSpeedDataType;

/**
 * @brief Diagnostic event identifiers for SpeedGovernor
 */
typedef enum {
    SPEEDGOV_EVENT_SENSOR_INVALID  = 10U,
    SPEEDGOV_EVENT_SENSOR_STALE    = 11U,
    SPEEDGOV_EVENT_OVERSPEED       = 12U,
    SPEEDGOV_EVENT_LIMIT_UPDATED   = 13U
} SpeedGov_DiagEventType;

/*******************************************************************************
 * RTE API: Sender/Receiver Ports (SWS_Rte_07139, SWS_Rte_07140)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_07139: Read vehicle speed
 * @details S/R Interface: If_VehicleSpeed, Data Element: Speed
 *
 * @param[out] speed_kph Pointer to receive speed value in km/h
 * @return Std_ReturnType
 *   - RTE_E_OK: Data read successfully
 *   - RTE_E_NO_DATA: No data available
 *   - RTE_E_NEVER_RECEIVED: Data never received since initialization
 *   - RTE_E_MAX_AGE_EXCEEDED: Data is stale
 *
 * @pre Rte_Start() must be called
 * @post Data is copied to output parameter
 * @trace AUTOSAR SWS_Rte_07139
 * @safety ASIL-B
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_VehicleSpeed_Speed(
    P2VAR(uint16, AUTOMATIC, RTE_APPL_DATA) speed_kph
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read vehicle speed timestamp
 * @details Retrieves timestamp associated with speed measurement
 *
 * @param[out] timestamp_ms Pointer to receive timestamp in milliseconds
 * @return Std_ReturnType
 *   - RTE_E_OK: Timestamp read successfully
 *   - RTE_E_NO_DATA: No data available
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety ASIL-B
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_VehicleSpeed_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07140: Write alarm status
 * @details S/R Interface: If_AlarmStatus, Data Element: Alarm
 *
 * @param[in] alarm_on Alarm activation status (TRUE = on, FALSE = off)
 * @return Std_ReturnType
 *   - RTE_E_OK: Data written successfully
 *   - RTE_E_UNCONNECTED: Port not connected
 *
 * @pre Rte_Start() must be called
 * @post Alarm status transmitted to HMI/actuator
 * @trace AUTOSAR SWS_Rte_07140
 * @safety ASIL-B
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Write_Out_Alarm_Alarm(
    VAR(boolean, AUTOMATIC) alarm_on
);

/**
 * @brief AUTOSAR SWS_Rte_07140: Write speed limit request
 * @details S/R Interface: If_SpeedLimitRequest, Data Element: Limit
 *
 * @param[in] limit_kph Speed limit value in km/h
 * @return Std_ReturnType
 *   - RTE_E_OK: Data written successfully
 *   - RTE_E_UNCONNECTED: Port not connected
 *
 * @pre Rte_Start() must be called
 * @post Speed limit communicated to vehicle control systems
 * @trace AUTOSAR SWS_Rte_07140
 * @safety ASIL-B
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Write_Out_SpeedLimit_Limit(
    VAR(uint16, AUTOMATIC) limit_kph
);

/*******************************************************************************
 * RTE API: Client/Server Ports (SWS_Rte_02001)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_02001: Get current system time
 * @details C/S Interface: If_TimeService, Operation: NowMs
 *
 * @param[out] current_ms Pointer to receive current time in milliseconds
 * @return Std_ReturnType
 *   - RTE_E_OK: Time retrieved successfully
 *   - RTE_E_UNCONNECTED: Service not connected
 *   - RTE_E_TIMEOUT: Service call timeout
 *
 * @pre Rte_Start() must be called
 * @post Current time returned in milliseconds
 * @trace AUTOSAR SWS_Rte_02001
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Call_Time_NowMs(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) current_ms
);

/**
 * @brief AUTOSAR SWS_Rte_02001: Try get configuration parameter
 * @details C/S Interface: If_ConfigService, Operation: TryGetSpeedLimit
 *
 * @param[out] limit_kph Pointer to receive configured speed limit
 * @param[out] available Pointer to boolean indicating if new limit available
 * @return Std_ReturnType
 *   - RTE_E_OK: Operation successful
 *   - RTE_E_UNCONNECTED: Service not connected
 *
 * @pre Rte_Start() must be called
 * @post Configuration parameter retrieved if available
 * @trace AUTOSAR SWS_Rte_02001
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Call_Config_TryGetSpeedLimit(
    P2VAR(uint16, AUTOMATIC, RTE_APPL_DATA) limit_kph,
    P2VAR(boolean, AUTOMATIC, RTE_APPL_DATA) available
);

/**
 * @brief AUTOSAR SWS_Rte_02001: Report diagnostic event
 * @details C/S Interface: If_DiagnosticEvents, Operation: ReportEvent
 *
 * @param[in] event_id Diagnostic event identifier
 * @param[in] event_status Event status (occurred/passed)
 * @return Std_ReturnType
 *   - RTE_E_OK: Event reported successfully
 *   - RTE_E_UNCONNECTED: Service not connected
 *
 * @pre Rte_Start() must be called
 * @post Event logged to diagnostic subsystem
 * @trace AUTOSAR SWS_Rte_02001
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Call_Diag_ReportEvent(
    VAR(uint16, AUTOMATIC) event_id,
    VAR(uint8, AUTOMATIC) event_status
);

/*******************************************************************************
 * RTE API: Per-Instance Memory (SWS_Rte_01169)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01169: Access PIM state data
 * @details Provides pointer to SpeedGovernor per-instance memory
 *
 * @return Pointer to SpeedGovernor state structure
 *
 * @pre Rte_Start() must be called
 * @post Returns pointer to component-private state
 * @note Data lifetime: Valid for entire component lifecycle
 * @trace AUTOSAR SWS_Rte_01169
 * @safety ASIL-B: State data protected by RTE
 */
FUNC(P2VAR(SpeedGov_StateType, AUTOMATIC, RTE_APPL_DATA), RTE_CODE) Rte_Pim_State(void);

/*******************************************************************************
 * RTE API: Runnable Declarations (SWS_Rte_01167)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01167: SpeedGovernor cyclic runnable
 * @details Periodic runnable triggered by TimingEvent every 10ms
 *          Monitors vehicle speed and activates alarm on overspeed
 *
 * @pre Rte_Start() must be called
 * @post Alarm status updated based on speed and limit
 * @timing Period: 10ms, WCET: < 1ms
 * @trace AUTOSAR SWS_Rte_01167, SWS_BSW_00037
 * @safety ASIL-B
 */
FUNC(void, APP_SPEEDGOV_CODE) App_SpeedGov_Step(void);

/**
 * @brief AUTOSAR SWS_Rte_02512: SpeedGovernor initialization runnable
 * @details Called during RTE initialization phase
 *
 * @pre Rte_Start() initialization sequence
 * @post Component state initialized to safe defaults
 * @trace AUTOSAR SWS_Rte_02512, SWS_BSW_00101
 * @safety ASIL-B
 */
FUNC(void, APP_SPEEDGOV_CODE) App_SpeedGov_Init(void);

/*******************************************************************************
 * RTE Configuration Constants
 ******************************************************************************/

/**
 * @brief Maximum data age threshold (ms) before considering stale
 */
#define RTE_SPEEDGOV_MAX_DATA_AGE_MS    (100U)

/**
 * @brief RTE instance ID for SpeedGovernor component
 */
#define RTE_SPEEDGOV_INSTANCE_ID        (2U)

/**
 * @brief Default speed limit (km/h) if not configured
 */
#define RTE_SPEEDGOV_DEFAULT_LIMIT_KPH  (50U)

#ifdef __cplusplus
}
#endif

#endif /* RTE_APP_SPEEDGOV_H */
