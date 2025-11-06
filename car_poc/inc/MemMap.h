/**
 * @file MemMap.h
 * @brief AUTOSAR Memory Mapping Header
 * @details AUTOSAR SWS_MemMap_00003: Memory section mapping for code and data
 *          Compliant with AUTOSAR Classic Platform R22-11
 *
 * @copyright Mercedes-Benz AG
 * @version 1.0.0
 * @date 2025-10-16
 *
 * @requirements
 * - AUTOSAR SWS_MemMap_00003: Memory mapping abstraction
 * - AUTOSAR SWS_BSW_00006: Memory section definition for modules
 * - MISRA C:2012 Rule 20.1: Include directives usage
 * - ISO 26262: Memory protection for safety-critical software
 *
 * @usage
 * #define <MODULE>_START_SEC_CODE
 * #include "MemMap.h"
 * // Code section
 * #define <MODULE>_STOP_SEC_CODE
 * #include "MemMap.h"
 */

/*******************************************************************************
 * AUTOSAR Memory Mapping - RTE Module
 ******************************************************************************/

/* RTE Code Section */
#if defined(RTE_START_SEC_CODE)
    #undef RTE_START_SEC_CODE
    /* #pragma section code "RTE_CODE" */
#elif defined(RTE_STOP_SEC_CODE)
    #undef RTE_STOP_SEC_CODE
    /* #pragma section code restore */

/* RTE Constant Section */
#elif defined(RTE_START_SEC_CONST_UNSPECIFIED)
    #undef RTE_START_SEC_CONST_UNSPECIFIED
    /* #pragma section const "RTE_CONST" */
#elif defined(RTE_STOP_SEC_CONST_UNSPECIFIED)
    #undef RTE_STOP_SEC_CONST_UNSPECIFIED
    /* #pragma section const restore */

/* RTE Variable Section - Initialized */
#elif defined(RTE_START_SEC_VAR_INIT_UNSPECIFIED)
    #undef RTE_START_SEC_VAR_INIT_UNSPECIFIED
    /* #pragma section data "RTE_VAR_INIT" */
#elif defined(RTE_STOP_SEC_VAR_INIT_UNSPECIFIED)
    #undef RTE_STOP_SEC_VAR_INIT_UNSPECIFIED
    /* #pragma section data restore */

/* RTE Variable Section - Uninitialized */
#elif defined(RTE_START_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef RTE_START_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss "RTE_VAR_NOINIT" */
#elif defined(RTE_STOP_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef RTE_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss restore */

/*******************************************************************************
 * AUTOSAR Memory Mapping - AutoBrake SWC
 ******************************************************************************/

/* AutoBrake Code Section */
#elif defined(APP_AUTOBRAKE_START_SEC_CODE)
    #undef APP_AUTOBRAKE_START_SEC_CODE
    /* #pragma section code "APP_AUTOBRAKE_CODE" */
#elif defined(APP_AUTOBRAKE_STOP_SEC_CODE)
    #undef APP_AUTOBRAKE_STOP_SEC_CODE
    /* #pragma section code restore */

/* AutoBrake PIM Section */
#elif defined(APP_AUTOBRAKE_START_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_AUTOBRAKE_START_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss "APP_AUTOBRAKE_PIM" */
#elif defined(APP_AUTOBRAKE_STOP_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_AUTOBRAKE_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss restore */

/*******************************************************************************
 * AUTOSAR Memory Mapping - SpeedGovernor SWC
 ******************************************************************************/

/* SpeedGovernor Code Section */
#elif defined(APP_SPEEDGOV_START_SEC_CODE)
    #undef APP_SPEEDGOV_START_SEC_CODE
    /* #pragma section code "APP_SPEEDGOV_CODE" */
#elif defined(APP_SPEEDGOV_STOP_SEC_CODE)
    #undef APP_SPEEDGOV_STOP_SEC_CODE
    /* #pragma section code restore */

/* SpeedGovernor PIM Section */
#elif defined(APP_SPEEDGOV_START_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_SPEEDGOV_START_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss "APP_SPEEDGOV_PIM" */
#elif defined(APP_SPEEDGOV_STOP_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_SPEEDGOV_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss restore */

/*******************************************************************************
 * AUTOSAR Memory Mapping - AutoPark SWC
 ******************************************************************************/

/* AutoPark Code Section */
#elif defined(APP_AUTOPARK_START_SEC_CODE)
    #undef APP_AUTOPARK_START_SEC_CODE
    /* #pragma section code "APP_AUTOPARK_CODE" */
#elif defined(APP_AUTOPARK_STOP_SEC_CODE)
    #undef APP_AUTOPARK_STOP_SEC_CODE
    /* #pragma section code restore */

/* AutoPark PIM Section */
#elif defined(APP_AUTOPARK_START_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_AUTOPARK_START_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss "APP_AUTOPARK_PIM" */
#elif defined(APP_AUTOPARK_STOP_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_AUTOPARK_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss restore */

/*******************************************************************************
 * AUTOSAR Memory Mapping - Climate SWC
 ******************************************************************************/

/* Climate Code Section */
#elif defined(APP_CLIMATE_START_SEC_CODE)
    #undef APP_CLIMATE_START_SEC_CODE
    /* #pragma section code "APP_CLIMATE_CODE" */
#elif defined(APP_CLIMATE_STOP_SEC_CODE)
    #undef APP_CLIMATE_STOP_SEC_CODE
    /* #pragma section code restore */

/* Climate PIM Section */
#elif defined(APP_CLIMATE_START_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_CLIMATE_START_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss "APP_CLIMATE_PIM" */
#elif defined(APP_CLIMATE_STOP_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_CLIMATE_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss restore */

/*******************************************************************************
 * AUTOSAR Memory Mapping - Wipers SWC
 ******************************************************************************/

/* Wipers Code Section */
#elif defined(APP_WIPERS_START_SEC_CODE)
    #undef APP_WIPERS_START_SEC_CODE
    /* #pragma section code "APP_WIPERS_CODE" */
#elif defined(APP_WIPERS_STOP_SEC_CODE)
    #undef APP_WIPERS_STOP_SEC_CODE
    /* #pragma section code restore */

/* Wipers PIM Section */
#elif defined(APP_WIPERS_START_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_WIPERS_START_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss "APP_WIPERS_PIM" */
#elif defined(APP_WIPERS_STOP_SEC_VAR_NOINIT_UNSPECIFIED)
    #undef APP_WIPERS_STOP_SEC_VAR_NOINIT_UNSPECIFIED
    /* #pragma section bss restore */

/*******************************************************************************
 * Error Detection
 ******************************************************************************/

#else
    #error "MemMap.h: No valid memory section defined"
#endif

/**
 * @note For production use with specific compiler/linker:
 *       - Replace pragma comments with actual compiler directives
 *       - Examples:
 *         GCC/Clang: __attribute__((section("name")))
 *         MSVC: #pragma code_seg("name")
 *         IAR: #pragma location="name"
 *         Green Hills: #pragma ghs section text="name"
 *         ARM Compiler: __attribute__((section("name")))
 */
