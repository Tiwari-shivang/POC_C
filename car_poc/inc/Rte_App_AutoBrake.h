/**
 * @file Rte_App_AutoBrake.h
 * @brief RTE Generated Header for AutoBrake Software Component
 * @details AUTOSAR SWS_Rte_01167: Component-specific RTE API declarations
 *          Generated for App_AutoBrake Application Software Component
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @swc_arxml_mapping
 * SWC: App_AutoBrake
 * Runnable: App_AutoBrake_Step (TimingEvent: 10ms)
 * Ports:
 *   - R-Port: In_Distance (If_Distance)
 *   - R-Port: In_VehicleReady (If_VehicleStatus)
 *   - R-Port: In_DriverBrake (If_DriverInput)
 *   - P-Port: Out_BrakeCmd (If_BrakeCommand)
 *   - R-Port: Time (If_TimeService) [C/S]
 *   - R-Port: Diag (If_DiagnosticEvents) [C/S]
 * PIM:
 *   - State (AutoBrake_StateType)
 */

#ifndef RTE_APP_AUTOBRAKE_H
#define RTE_APP_AUTOBRAKE_H

#include "Rte.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Component Data Types
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01169: Per-Instance Memory (PIM) structure
 * @details State variables for AutoBrake functionality
 */
typedef struct {
    uint8 hit_count;              /**< Debounce counter for hazard detection */
    boolean brake_active;         /**< Current brake activation status */
    boolean prev_below_thresh;    /**< Previous threshold state for edge detection */
} AutoBrake_StateType;

/**
 * @brief Distance sensor data structure
 */
typedef struct {
    uint16 distance_mm;           /**< Distance in millimeters */
    uint32 timestamp_ms;          /**< Sensor timestamp in milliseconds */
    boolean is_valid;             /**< Data validity flag */
} DistanceSensorDataType;

/**
 * @brief Diagnostic event identifiers for AutoBrake
 */
typedef enum {
    AUTOBRAKE_EVENT_SENSOR_INVALID = 1U,
    AUTOBRAKE_EVENT_SENSOR_STALE   = 2U,
    AUTOBRAKE_EVENT_HAZARD_DETECTED = 3U,
    AUTOBRAKE_EVENT_BRAKE_APPLIED  = 4U
} AutoBrake_DiagEventType;

/*******************************************************************************
 * RTE API: Sender/Receiver Ports (SWS_Rte_07139, SWS_Rte_07140)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_07139: Read distance sensor data
 * @details S/R Interface: If_Distance, Data Element: Distance
 *
 * @param[out] data Pointer to receive distance value in millimeters
 * @return Std_ReturnType
 *   - RTE_E_OK: Data read successfully
 *   - RTE_E_NO_DATA: No data available
 *   - RTE_E_NEVER_RECEIVED: Data never received since initialization
 *   - RTE_E_MAX_AGE_EXCEEDED: Data is stale
 *
 * @pre Rte_Start() must be called
 * @post Data is copied to output parameter
 * @trace AUTOSAR SWS_Rte_07139
 * @safety ASIL-D
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_Distance_Distance(
    P2VAR(uint16, AUTOMATIC, RTE_APPL_DATA) data
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read distance sensor timestamp
 * @details Retrieves timestamp associated with distance measurement
 *
 * @param[out] timestamp_ms Pointer to receive timestamp in milliseconds
 * @return Std_ReturnType
 *   - RTE_E_OK: Timestamp read successfully
 *   - RTE_E_NO_DATA: No data available
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety ASIL-D
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_Distance_Timestamp(
    P2VAR(uint32, AUTOMATIC, RTE_APPL_DATA) timestamp_ms
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read vehicle ready status
 * @details S/R Interface: If_VehicleStatus, Data Element: Ready
 *
 * @param[out] ready Pointer to receive vehicle ready status
 * @return Std_ReturnType
 *   - RTE_E_OK: Status read successfully
 *   - RTE_E_NO_DATA: No data available
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety ASIL-D
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_VehicleReady_Ready(
    P2VAR(boolean, AUTOMATIC, RTE_APPL_DATA) ready
);

/**
 * @brief AUTOSAR SWS_Rte_07139: Read driver brake pedal status
 * @details S/R Interface: If_DriverInput, Data Element: BrakePressed
 *
 * @param[out] pressed Pointer to receive brake pedal state
 * @return Std_ReturnType
 *   - RTE_E_OK: Status read successfully
 *   - RTE_E_NO_DATA: No data available
 *
 * @trace AUTOSAR SWS_Rte_07139
 * @safety ASIL-D
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Read_In_DriverBrake_BrakePressed(
    P2VAR(boolean, AUTOMATIC, RTE_APPL_DATA) pressed
);

/**
 * @brief AUTOSAR SWS_Rte_07140: Write brake command
 * @details S/R Interface: If_BrakeCommand, Data Element: Brake
 *
 * @param[in] brake_request Brake activation command (TRUE = apply, FALSE = release)
 * @return Std_ReturnType
 *   - RTE_E_OK: Data written successfully
 *   - RTE_E_UNCONNECTED: Port not connected
 *   - RTE_E_TRANSMIT_ACK: Data queued for transmission
 *
 * @pre Rte_Start() must be called
 * @post Brake command transmitted to actuator
 * @trace AUTOSAR SWS_Rte_07140
 * @safety ASIL-D
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Write_Out_BrakeCmd_Brake(
    VAR(boolean, AUTOMATIC) brake_request
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
 * @details Provides pointer to AutoBrake per-instance memory
 *
 * @return Pointer to AutoBrake state structure
 *
 * @pre Rte_Start() must be called
 * @post Returns pointer to component-private state
 * @note Data lifetime: Valid for entire component lifecycle
 * @trace AUTOSAR SWS_Rte_01169
 * @safety ASIL-D: State data protected by RTE
 */
FUNC(P2VAR(AutoBrake_StateType, AUTOMATIC, RTE_APPL_DATA), RTE_CODE) Rte_Pim_State(void);

/*******************************************************************************
 * RTE API: Runnable Declarations (SWS_Rte_01167)
 ******************************************************************************/

/**
 * @brief AUTOSAR SWS_Rte_01167: AutoBrake cyclic runnable
 * @details Periodic runnable triggered by TimingEvent every 10ms
 *          Implements emergency braking logic based on distance sensor
 *
 * @pre Rte_Start() must be called
 * @post Brake command updated based on sensor data
 * @timing Period: 10ms, WCET: < 2ms
 * @trace AUTOSAR SWS_Rte_01167, SWS_BSW_00037
 * @safety ASIL-D
 */
FUNC(void, APP_AUTOBRAKE_CODE) App_AutoBrake_Step(void);

/**
 * @brief AUTOSAR SWS_Rte_02512: AutoBrake initialization runnable
 * @details Called during RTE initialization phase
 *
 * @pre Rte_Start() initialization sequence
 * @post Component state initialized to safe defaults
 * @trace AUTOSAR SWS_Rte_02512, SWS_BSW_00101
 * @safety ASIL-D
 */
FUNC(void, APP_AUTOBRAKE_CODE) App_AutoBrake_Init(void);

/*******************************************************************************
 * RTE Configuration Constants
 ******************************************************************************/

/**
 * @brief Maximum data age threshold (ms) before considering stale
 */
#define RTE_AUTOBRAKE_MAX_DATA_AGE_MS   (100U)

/**
 * @brief RTE instance ID for AutoBrake component
 */
#define RTE_AUTOBRAKE_INSTANCE_ID       (1U)

#ifdef __cplusplus
}
#endif

#endif /* RTE_APP_AUTOBRAKE_H */
