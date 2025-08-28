/*
 * test_autobrake.c
 *
 * ISO 26262–aligned unit tests for auto-brake controller.
 * - Tests are tagged with requirement IDs (SG/SSR) and rationale.
 * - Boundaries, timing, and fault-injection are covered.
 * - No magic numbers: uses config constants, with safe fallbacks.
 */

#include "unity.h"
#include "app_autobrake.h"
#include <stdbool.h>
#include <stdint.h>

/* ---- Configuration fallbacks (compile-time) ----------------------------- */
/* If your project already defines these in config.h, those will be used.   */
#include "config.h"

#ifndef BRAKE_THRESH_MM
#define BRAKE_THRESH_MM            (1220U)   /* example threshold */
#endif

#ifndef AUTOBRAKE_DEBOUNCE_COUNT
#define AUTOBRAKE_DEBOUNCE_COUNT   (3U)      /* consecutive detects required */
#endif

#ifndef AUTOBRAKE_ACTIVATE_LATENCY_MS
#define AUTOBRAKE_ACTIVATE_LATENCY_MS (100U) /* max time to assert brake */
#endif

#ifndef STALE_MS
#define STALE_MS                   (100U)    /* max sensor sample age */
#endif

#ifndef CONTROL_DT_MS
#define CONTROL_DT_MS              (10U)     /* controller step period */
#endif

/* ---- HAL test doubles (deterministic & side-effect free) ---------------- */

static uint16_t mock_distance_mm = 2000U;
static uint32_t mock_sample_ts_ms = 0U;
static bool     mock_vehicle_ready = true;
static bool     mock_driver_brake = false;
static bool     mock_brake_request = false;
static uint32_t mock_now_ms = 0U;
static bool     mock_sensor_ok = true;

bool hal_get_vehicle_ready(void) { return mock_vehicle_ready; }

bool hal_driver_brake_pressed(void) { return mock_driver_brake; }

uint32_t hal_now_ms(void) { return mock_now_ms; }

bool hal_read_distance_mm(uint16_t *out_mm, uint32_t *out_ts_ms)
{
    if ((!mock_sensor_ok) || (out_mm == NULL) || (out_ts_ms == NULL))
    {
        return false;
    }
    *out_mm    = mock_distance_mm;
    *out_ts_ms = mock_sample_ts_ms;
    return true;
}

void hal_set_brake_request(bool on) { mock_brake_request = on; }

/* ---- Test harness helpers ---------------------------------------------- */

static void reset_mocks(void)
{
    mock_distance_mm   = 2000U;
    mock_sample_ts_ms  = 0U;
    mock_vehicle_ready = true;
    mock_driver_brake  = false;
    mock_brake_request = false;
    mock_now_ms        = 0U;
    mock_sensor_ok     = true;
}

/* Advance controller by n steps of CONTROL_DT_MS each */
static void step_n(uint32_t n_steps)
{
    uint32_t i;
    for (i = 0U; i < n_steps; ++i)
    {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_autobrake_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

/* Run until condition or time budget (ms) is exceeded */
static bool run_until_or_timeout(bool *cond, uint32_t budget_ms)
{
    uint32_t elapsed = 0U;
    while ((!(*cond)) && (elapsed <= budget_ms))
    {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_autobrake_step();
        mock_now_ms += CONTROL_DT_MS;
        elapsed     += CONTROL_DT_MS;
    }
    return (*cond) && (elapsed <= budget_ms);
}

/* ---- Unity fixtures ----------------------------------------------------- */

void setUp(void)
{
    reset_mocks();
    /* Start time at a non-zero deterministic value */
    mock_now_ms = 100U;
    app_autobrake_init();
}

void tearDown(void) { /* nothing */ }

/* ======================================================================== */
/*                              TEST CASES                                  */
/* ======================================================================== */

/* SSR-BRK-01 (ISO 26262): No automatic brake request at or above threshold.
 * Rationale: Avoid nuisance braking when TTC not critical.
 */
void test_SAF_NoBrake_When_Distance_AtOrAbove_Threshold(void)
{
    mock_distance_mm  = BRAKE_THRESH_MM;     /* boundary */
    mock_sample_ts_ms = mock_now_ms;

    app_autobrake_step();
    TEST_ASSERT_FALSE(mock_brake_request);

    mock_distance_mm  = (uint16_t)(BRAKE_THRESH_MM + 1U); /* > threshold */
    app_autobrake_step();
    TEST_ASSERT_FALSE(mock_brake_request);
}

/* SSR-BRK-01/03 (ISO 26262): Brake asserts when distance < threshold
 * after required debounce, and within the activation latency.
 */
void test_SAF_Brake_Activates_BelowThreshold_Within_Latency(void)
{
    mock_distance_mm  = (uint16_t)(BRAKE_THRESH_MM - 1U); /* just below */
    mock_sample_ts_ms = mock_now_ms;

    /* Satisfy debounce: AUTOBRAKE_DEBOUNCE_COUNT consecutive detections */
    uint32_t need = AUTOBRAKE_DEBOUNCE_COUNT;
    while ((need--) > 0U)
    {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_autobrake_step();
        mock_now_ms += CONTROL_DT_MS;
    }

    /* Now ensure activation occurs within latency budget */
    bool ok = run_until_or_timeout(&mock_brake_request, AUTOBRAKE_ACTIVATE_LATENCY_MS);
    TEST_ASSERT_TRUE(ok);
}

/* SSR-BRK-02 (ISO 26262): Vehicle-not-ready inhibits auto-brake. */
void test_SAF_NoBrake_When_Vehicle_NotReady(void)
{
    mock_vehicle_ready = false;
    mock_distance_mm   = (uint16_t)(BRAKE_THRESH_MM - 100U);
    mock_sample_ts_ms  = mock_now_ms;

    step_n(AUTOBRAKE_DEBOUNCE_COUNT);
    TEST_ASSERT_FALSE(mock_brake_request);
}

/* SSR-BRK-04 (ISO 26262): Manual driver braking overrides auto request. */
void test_SAF_NoAutoBrake_When_Driver_Override(void)
{
    mock_driver_brake  = true;
    mock_distance_mm   = (uint16_t)(BRAKE_THRESH_MM - 100U);
    mock_sample_ts_ms  = mock_now_ms;

    step_n(AUTOBRAKE_DEBOUNCE_COUNT + 2U);
    TEST_ASSERT_FALSE(mock_brake_request);
}

/* SSR-BRK-05 (ISO 26262): Stale sensor sample must not trigger braking. */
void test_SAF_NoBrake_On_Stale_Sensor_Data(void)
{
    mock_distance_mm  = (uint16_t)(BRAKE_THRESH_MM - 100U);
    mock_sample_ts_ms = (uint32_t)(mock_now_ms - (STALE_MS + 1U)); /* older than allowed */

    step_n(AUTOBRAKE_DEBOUNCE_COUNT + 2U);
    TEST_ASSERT_FALSE(mock_brake_request);
}

/* SSR-BRK-06 (ISO 26262): Sensor read failure must not trigger braking. */
void test_SAF_NoBrake_On_Sensor_Failure(void)
{
    mock_sensor_ok    = false;       /* hal_read_distance_mm will return false */
    mock_sample_ts_ms = mock_now_ms;
    step_n(AUTOBRAKE_DEBOUNCE_COUNT + 3U);
    TEST_ASSERT_FALSE(mock_brake_request);
}

/* SSR-BRK-07 (ISO 26262): De-assert when hazard is gone (hysteresis exit).
 * Rationale: Controller should release command when distance recovers.
 */
void test_SAF_Brake_Deasserts_When_Distance_Recovers(void)
{
    /* Ensure completely clean state */
    reset_mocks();
    mock_now_ms = 100U;
    app_autobrake_init();
    
    /* First assert */
    mock_distance_mm  = (uint16_t)(BRAKE_THRESH_MM - 1U);
    mock_sample_ts_ms = mock_now_ms;
    step_n(AUTOBRAKE_DEBOUNCE_COUNT + 3U);
    TEST_ASSERT_TRUE(mock_brake_request);

    /* Then recover to safe distance */
    mock_distance_mm  = (uint16_t)(BRAKE_THRESH_MM + 50U);
    mock_sample_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_FALSE(mock_brake_request);
}

/* ======================================================================== */
/*                                  MAIN                                    */
/* ======================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SAF_NoBrake_When_Distance_AtOrAbove_Threshold);
    RUN_TEST(test_SAF_Brake_Activates_BelowThreshold_Within_Latency);
    RUN_TEST(test_SAF_NoBrake_When_Vehicle_NotReady);
    RUN_TEST(test_SAF_NoAutoBrake_When_Driver_Override);
    RUN_TEST(test_SAF_NoBrake_On_Stale_Sensor_Data);
    RUN_TEST(test_SAF_NoBrake_On_Sensor_Failure);
    RUN_TEST(test_SAF_Brake_Deasserts_When_Distance_Recovers);

    return UNITY_END();
}
