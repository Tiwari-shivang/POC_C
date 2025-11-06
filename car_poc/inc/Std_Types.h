/**
 * @file Std_Types.h
 * @brief AUTOSAR Standard Types Definition
 * @details AUTOSAR SWS_Std_00005: Standard type definitions
 *          Compliant with AUTOSAR Classic Platform R22-11
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @requirements
 * - AUTOSAR SWS_Std_00005: Standard types shall be defined
 * - AUTOSAR SWS_Std_00006: Version information shall be available
 * - AUTOSAR SWS_Std_00011: Std_ReturnType for function returns
 * - MISRA C:2012 Rule 8.6: Functions shall be declared at file scope
 */

#ifndef STD_TYPES_H
#define STD_TYPES_H

/**
 * @brief AUTOSAR SWS_Std_00013: Platform types inclusion
 */
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * AUTOSAR Version Information
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Std_00006: Module version information
 */
#define STD_TYPES_AR_RELEASE_MAJOR_VERSION    (4U)
#define STD_TYPES_AR_RELEASE_MINOR_VERSION    (5U)
#define STD_TYPES_AR_RELEASE_REVISION_VERSION (0U)

/*******************************************************************************
 * AUTOSAR Standard Return Types (SWS_Std_00011)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Std_00011: Standard return type
 * @details This type shall be used for all API functions that return success/failure
 */
typedef uint8_t Std_ReturnType;

/**
 * @brief AUTOSAR SWS_Std_00011: Return value for successful operation
 */
#define E_OK      ((Std_ReturnType)0x00U)

/**
 * @brief AUTOSAR SWS_Std_00011: Return value for failed operation
 */
#define E_NOT_OK  ((Std_ReturnType)0x01U)

/*******************************************************************************
 * AUTOSAR Standard Base Types (SWS_Std_00005)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Std_00015: 8-bit unsigned integer
 */
typedef uint8_t   uint8;

/**
 * @brief AUTOSAR SWS_Std_00015: 16-bit unsigned integer
 */
typedef uint16_t  uint16;

/**
 * @brief AUTOSAR SWS_Std_00015: 32-bit unsigned integer
 */
typedef uint32_t  uint32;

/**
 * @brief AUTOSAR SWS_Std_00015: 8-bit signed integer
 */
typedef int8_t    sint8;

/**
 * @brief AUTOSAR SWS_Std_00015: 16-bit signed integer
 */
typedef int16_t   sint16;

/**
 * @brief AUTOSAR SWS_Std_00015: 32-bit signed integer
 */
typedef int32_t   sint32;

/**
 * @brief AUTOSAR SWS_Std_00015: Boolean type
 * @details Standard boolean type with TRUE/FALSE values
 */
typedef bool      boolean;

/**
 * @brief AUTOSAR SWS_Std_00010: Boolean TRUE value
 */
#ifndef TRUE
#define TRUE  ((boolean)true)
#endif

/**
 * @brief AUTOSAR SWS_Std_00010: Boolean FALSE value
 */
#ifndef FALSE
#define FALSE ((boolean)false)
#endif

/**
 * @brief AUTOSAR SWS_Std_00007: ON state definition
 */
#define STD_ON    (1U)

/**
 * @brief AUTOSAR SWS_Std_00007: OFF state definition
 */
#define STD_OFF   (0U)

/**
 * @brief AUTOSAR SWS_Std_00012: ACTIVE state definition
 */
#define STD_ACTIVE   (1U)

/**
 * @brief AUTOSAR SWS_Std_00013: IDLE state definition
 */
#define STD_IDLE     (0U)

/**
 * @brief AUTOSAR SWS_Std_00014: HIGH level definition
 */
#define STD_HIGH     (1U)

/**
 * @brief AUTOSAR SWS_Std_00014: LOW level definition
 */
#define STD_LOW      (0U)

/*******************************************************************************
 * AUTOSAR Version Information Structure (SWS_Std_00015)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Std_00015: Module version information structure
 * @details Used for GetVersionInfo API functions
 */
typedef struct {
    uint16 vendorID;              /**< Vendor ID */
    uint16 moduleID;              /**< Module ID */
    uint8  sw_major_version;      /**< Software major version */
    uint8  sw_minor_version;      /**< Software minor version */
    uint8  sw_patch_version;      /**< Software patch version */
} Std_VersionInfoType;

/*******************************************************************************
 * AUTOSAR Compiler Abstraction Macros (SWS_Std_00031)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Compiler_00004: Function definition macro
 * @param rettype Return type
 * @param memclass Memory class
 */
#ifndef FUNC
#define FUNC(rettype, memclass) rettype
#endif

/**
 * @brief AUTOSAR SWS_Compiler_00005: Pointer to variable macro
 * @param ptrtype Pointer type
 * @param memclass Memory class
 */
#ifndef P2VAR
#define P2VAR(ptrtype, memclass, ptrclass) ptrtype *
#endif

/**
 * @brief AUTOSAR SWS_Compiler_00006: Pointer to constant macro
 * @param ptrtype Pointer type
 * @param memclass Memory class
 */
#ifndef P2CONST
#define P2CONST(ptrtype, memclass, ptrclass) const ptrtype *
#endif

/**
 * @brief AUTOSAR SWS_Compiler_00013: Constant variable macro
 * @param type Variable type
 * @param memclass Memory class
 */
#ifndef CONST
#define CONST(type, memclass) const type
#endif

/**
 * @brief AUTOSAR SWS_Compiler_00023: Variable definition macro
 * @param type Variable type
 * @param memclass Memory class
 */
#ifndef VAR
#define VAR(type, memclass) type
#endif

/*******************************************************************************
 * AUTOSAR NULL Pointer Definition (SWS_Std_00031)
 ******************************************************************************/
/**
 * @brief AUTOSAR SWS_Std_00031: NULL pointer definition
 */
#ifndef NULL_PTR
#define NULL_PTR  ((void *)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* STD_TYPES_H */
