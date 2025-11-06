/**
 * @file Rte.h
 * @brief AUTOSAR Run-Time Environment (RTE) Base Header
 * @details AUTOSAR SWS_Rte_01167: RTE public interface definitions
 *          Compliant with AUTOSAR Classic Platform R22-11
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @requirements
 * - AUTOSAR SWS_Rte_01167: RTE shall provide APIs for SWC communication
 * - AUTOSAR SWS_Rte_07139: RTE Read operations for S/R ports
 * - AUTOSAR SWS_Rte_07140: RTE Write operations for S/R ports
 * - AUTOSAR SWS_Rte_02001: RTE Call operations for C/S ports
 * - ISO 26262 ASIL-D: Safety-critical automotive systems
 */

#ifndef RTE_H
#define RTE_H

/**
 * @brief AUTOSAR SWS_Std_00013: Include standard types
 */
#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * RTE Version Information
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_00001: RTE module version information
 */
#define RTE_AR_RELEASE_MAJOR_VERSION    (4U)
#define RTE_AR_RELEASE_MINOR_VERSION    (5U)
#define RTE_AR_RELEASE_REVISION_VERSION (0U)

#define RTE_SW_MAJOR_VERSION            (1U)
#define RTE_SW_MINOR_VERSION            (0U)
#define RTE_SW_PATCH_VERSION            (0U)

/*******************************************************************************
 * RTE Return Values (SWS_Rte_02002)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_02002: RTE-specific return values
 */
#define RTE_E_OK              ((Std_ReturnType)0x00U)  /**< Operation successful */
#define RTE_E_INVALID         ((Std_ReturnType)0x01U)  /**< Invalid data */
#define RTE_E_NO_DATA         ((Std_ReturnType)0x02U)  /**< No data available */
#define RTE_E_TRANSMIT_ACK    ((Std_ReturnType)0x03U)  /**< Transmit acknowledged */
#define RTE_E_NEVER_RECEIVED  ((Std_ReturnType)0x04U)  /**< Data never received */
#define RTE_E_UNCONNECTED     ((Std_ReturnType)0x05U)  /**< Port not connected */
#define RTE_E_TIMEOUT         ((Std_ReturnType)0x06U)  /**< Operation timeout */
#define RTE_E_LIMIT           ((Std_ReturnType)0x07U)  /**< Buffer limit reached */
#define RTE_E_LOST_DATA       ((Std_ReturnType)0x08U)  /**< Data lost */
#define RTE_E_MAX_AGE_EXCEEDED ((Std_ReturnType)0x09U) /**< Data age limit exceeded */

/*******************************************************************************
 * RTE Mode Values (SWS_Rte_02505)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_02505: Mode management definitions
 */
#define RTE_MODE_INVALID      (0xFFU)
#define RTE_TRANSITION_OCCURRED (0x01U)

/*******************************************************************************
 * RTE Port Communication Interface Types
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_07139: Sender/Receiver Port Handle Type
 * @details Opaque handle for S/R port instances
 */
typedef struct {
    uint8 portId;
    uint8 elementId;
    boolean isQueued;
    boolean isConnected;
} Rte_PortHandle_Type;

/**
 * @brief AUTOSAR SWS_Rte_02001: Client/Server Operation Handle Type
 * @details Opaque handle for C/S operation instances
 */
typedef struct {
    uint8 serviceId;
    uint8 operationId;
    boolean isAsync;
    boolean isConnected;
} Rte_OperationHandle_Type;

/*******************************************************************************
 * RTE Initialization & Lifecycle (SWS_Rte_02500)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_02500: RTE initialization
 * @details Must be called before any RTE API usage
 * @return E_OK: Initialization successful
 *         E_NOT_OK: Initialization failed
 *
 * @trace AUTOSAR SWS_Rte_02500, SWS_Rte_02512
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Start(void);

/**
 * @brief AUTOSAR SWS_Rte_02501: RTE de-initialization
 * @details Stops all RTE activities and releases resources
 * @return E_OK: De-initialization successful
 *         E_NOT_OK: De-initialization failed
 *
 * @trace AUTOSAR SWS_Rte_02501
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_Stop(void);

/*******************************************************************************
 * RTE Diagnostic & Debug Support
 ******************************************************************************/
/**
 * @brief RTE diagnostic information structure
 */
typedef struct {
    uint32 readOperations;
    uint32 writeOperations;
    uint32 callOperations;
    uint32 invalidAccess;
    uint32 timeoutErrors;
} Rte_DiagnosticInfo_Type;

/**
 * @brief Retrieve RTE diagnostic information
 * @param[out] diagInfo Pointer to diagnostic information structure
 * @return E_OK: Success, E_NOT_OK: Invalid parameter
 */
FUNC(Std_ReturnType, RTE_CODE) Rte_GetDiagnosticInfo(
    P2VAR(Rte_DiagnosticInfo_Type, AUTOMATIC, RTE_APPL_DATA) diagInfo
);

/*******************************************************************************
 * RTE Memory Class Definitions (SWS_Compiler)
 ******************************************************************************/
/**
 * @brief AUTOSAR memory classification for RTE code sections
 */
#define RTE_CODE
#define RTE_CONST
#define RTE_APPL_DATA
#define RTE_APPL_CONST
#define RTE_APPL_CODE
#define RTE_VAR_NOINIT
#define RTE_VAR_INIT

/*******************************************************************************
 * RTE Vendor & Module IDs
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_00001: Mercedes-Benz vendor ID
 */
#define RTE_VENDOR_ID                   (1000U)

/**
 * @brief AUTOSAR SWS_Rte_00001: RTE module ID
 */
#define RTE_MODULE_ID                   (2U)

/**
 * @brief AUTOSAR SWS_Rte_00001: RTE instance ID
 */
#define RTE_INSTANCE_ID                 (0U)

/*******************************************************************************
 * RTE Compatibility & Feature Switches
 ******************************************************************************/
/**
 * @brief Enable/disable RTE features for configuration
 */
#define RTE_SUPPORT_QUEUED_COMMUNICATION    STD_OFF
#define RTE_SUPPORT_NV_DATA                 STD_OFF
#define RTE_SUPPORT_INTER_PARTITION_COM     STD_OFF
#define RTE_SUPPORT_MULTI_INSTANTIATION     STD_OFF

/*******************************************************************************
 * RTE Data Consistency (SWS_Rte_07661)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Rte_07661: Data consistency level definitions
 */
typedef enum {
    RTE_CONSISTENCY_LEVEL_NONE = 0U,     /**< No consistency guarantee */
    RTE_CONSISTENCY_LEVEL_SIMPLE = 1U,   /**< Simple consistency */
    RTE_CONSISTENCY_LEVEL_FULL = 2U      /**< Full consistency */
} Rte_ConsistencyLevel_Type;

#ifdef __cplusplus
}
#endif

#endif /* RTE_H */
