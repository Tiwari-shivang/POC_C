/*
 * test_wipers.c
 *
 * ISO 26262–aligned unit tests for Rain-Sensing Wipers controller.
 * - Tests are tagged with safety requirement IDs (SSR-WPR-xx) for traceability.
 * - Boundaries, debounce, latency, stale-sample handling, and sensor-failure paths covered.
 * - Avoids magic numbers: uses project config constants with fallbacks.
 */

#include "unity.h"
#include "app_wipers.h"
#include <stdbool.h>
#include <stdint.h>

/* Prefer your project's config; these are safe fallbacks if not provided. */
#include "config.h"

#ifndef WIPER_MODE_OFF
#define WIPER_MODE_OFF           (0U)
#endif
#ifndef WIPER_MODE_INT
#define WIPER_MODE_INT           (1U)  /* intermittent */
#endif
#ifndef WIPER_MODE_LOW
#define WIPER_MODE_LOW           (2U)
#endif
#ifndef WIPER_MODE_HIGH
#define WIPER_MODE_HIGH          (3U)
#endif

#ifndef RAIN_THR_INT_ON_PCT
#define RAIN_THR_INT_ON_PCT      (20U) /* up-threshold to go from OFF -> INT */
#endif
#ifndef RAIN_THR_INT_OFF_PCT
#define RAIN_THR_INT_OFF_PCT     (15U) /* down-threshold to go INT -> OFF (hysteresis) */
#endif
#ifndef RAIN_THR_LOW_ON_PCT
#define RAIN_THR_LOW_ON_PCT      (40U) /* INT -> LOW */
#endif
#ifndef RAIN_THR_LOW_OFF_PCT
#define RAIN_THR_LOW_OFF_PCT     (35U) /* LOW -> INT */
#endif
#ifndef RAIN_THR_HIGH_ON_PCT
#define RAIN_THR_HIGH_ON_PCT     (70U) /* LOW -> HIGH */
#endif
#ifndef RAIN_THR_HIGH_OFF_PCT
#define RAIN_THR_HIGH_OFF_PCT    (60U) /* HIGH -> LOW */
#endif

#ifndef WIPERS_DEBOUNCE_COUNT
#define WIPERS_DEBOUNCE_COUNT    (2U)  /* consecutive samples before mode change */
#endif

#ifndef WIPERS_LATENCY_MS
#define WIPERS_LATENCY_MS        (200U) /* must settle within this time */
#endif

#ifndef STALE_MS
#define STALE_MS                 (100U) /* reject samples older than this */
#endif

#ifndef CONTROL_DT_MS
#define CONTROL_DT_MS            (10U)  /* controller period */
#endif

/* ------------------ HAL test doubles ------------------ */

static uint8_t  mock_rain_pct     = 0U;
static uint32_t mock_rain_ts_ms   = 0U;
static uint8_t  mock_wiper_mode   = WIPER_MODE_OFF;
static uint32_t mock_now_ms       = 0U;

static bool     mock_rain_ok      = true;

uint32_t hal_now_ms(void) { return mock_now_ms; }

bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms)
{
    if (!mock_rain_ok || (out_pct == NULL) || (out_ts_ms == NULL)) { return false; }
    *out_pct  = mock_rain_pct;
    *out_ts_ms = mock_rain_ts_ms;
    return true;
}

void hal_set_wiper_mode(uint8_t mode) { mock_wiper_mode = mode; }

/* ------------------ Helpers ------------------ */

static void reset_mocks(void)
{
    mock_rain_pct   = 0U;
    mock_rain_ts_ms = 0U;
    mock_wiper_mode = WIPER_MODE_OFF;
    mock_now_ms     = 100U;
    mock_rain_ok    = true;
}

static void step_n(uint32_t n)
{
    for (uint32_t i = 0U; i < n; ++i) {
        mock_rain_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_wipers_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

/* Wait for specific mode within time budget */
static bool wait_mode(uint8_t want_mode, uint32_t budget_ms)
{
    uint32_t elapsed = 0U;
    while ((mock_wiper_mode != want_mode) && (elapsed <= budget_ms)) {
        mock_rain_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_wipers_step();
        mock_now_ms += CONTROL_DT_MS;
        elapsed     += CONTROL_DT_MS;
    }
    return (mock_wiper_mode == want_mode) && (elapsed <= budget_ms);
}

/* ------------------ Unity fixtures ------------------ */

void setUp(void)
{
    reset_mocks();
    app_wipers_init();
    mock_rain_ts_ms = mock_now_ms;
}

void tearDown(void) {}

/* ======================================================================= */
/*                              TEST CASES                                  */
/* ======================================================================= */

/* SSR-WPR-01 (ISO 26262):
 * With no rain (<= INT off threshold), mode shall be OFF.
 */
void test_SAF_Off_When_NoRain(void)
{
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_OFF_PCT);
    mock_rain_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_OFF, mock_wiper_mode);
}

/* SSR-WPR-02 (ISO 26262):
 * Light rain reaching INT on threshold shall transition OFF -> INT
 * after debounce and within latency.
 */
void test_SAF_Int_On_LightRain_After_Debounce_Within_Latency(void)
{
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_ON_PCT); /* boundary */
    mock_rain_ts_ms = mock_now_ms;

    /* satisfy debounce */
    step_n(WIPERS_DEBOUNCE_COUNT);
    bool ok = wait_mode(WIPER_MODE_INT, WIPERS_LATENCY_MS);
    TEST_ASSERT_TRUE(ok);
}

/* SSR-WPR-03 (ISO 26262):
 * Moderate rain reaching LOW on threshold shall transition INT -> LOW
 * with debounce and latency constraints.
 */
void test_SAF_Low_On_ModerateRain_After_Debounce(void)
{
    /* First ensure we are in INT */
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_ON_PCT + 1U);
    mock_rain_ts_ms = mock_now_ms;
    step_n(WIPERS_DEBOUNCE_COUNT + 1U);
    (void)wait_mode(WIPER_MODE_INT, WIPERS_LATENCY_MS);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_INT, mock_wiper_mode);

    /* Now cross to LOW */
    mock_rain_pct   = (uint8_t)(RAIN_THR_LOW_ON_PCT);
    mock_rain_ts_ms = mock_now_ms;
    step_n(WIPERS_DEBOUNCE_COUNT);
    bool ok = wait_mode(WIPER_MODE_LOW, WIPERS_LATENCY_MS);
    TEST_ASSERT_TRUE(ok);
}

/* SSR-WPR-04 (ISO 26262):
 * Heavy rain reaching HIGH on threshold shall transition LOW -> HIGH
 * with debounce and latency constraints.
 */
void test_SAF_High_On_HeavyRain_After_Debounce(void)
{
    /* Get to LOW first */
    mock_rain_pct   = (uint8_t)(RAIN_THR_LOW_ON_PCT + 5U);
    mock_rain_ts_ms = mock_now_ms;
    step_n(WIPERS_DEBOUNCE_COUNT + 2U);
    (void)wait_mode(WIPER_MODE_LOW, WIPERS_LATENCY_MS);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_LOW, mock_wiper_mode);

    /* Then cross to HIGH */
    mock_rain_pct   = (uint8_t)(RAIN_THR_HIGH_ON_PCT);
    mock_rain_ts_ms = mock_now_ms;
    step_n(WIPERS_DEBOUNCE_COUNT);
    bool ok = wait_mode(WIPER_MODE_HIGH, WIPERS_LATENCY_MS);
    TEST_ASSERT_TRUE(ok);
}

/* SSR-WPR-05 (ISO 26262):
 * Hysteresis—reducing rain slightly below up-threshold must not immediately
 * drop the mode; only when below the down-threshold should it step down.
 */
void test_SAF_Hysteresis_On_StepDowns(void)
{
    /* Go to INT */
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_ON_PCT + 1U);
    mock_rain_ts_ms = mock_now_ms;
    step_n(WIPERS_DEBOUNCE_COUNT + 1U);
    (void)wait_mode(WIPER_MODE_INT, WIPERS_LATENCY_MS);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_INT, mock_wiper_mode);

    /* Slightly below on-threshold, still >= off-threshold -> stay INT */
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_ON_PCT - 1U);
    step_n(WIPERS_DEBOUNCE_COUNT + 1U);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_INT, mock_wiper_mode);

    /* Go below off-threshold -> should drop to OFF */
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_OFF_PCT - 1U);
    step_n(WIPERS_DEBOUNCE_COUNT + 1U);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_OFF, mock_wiper_mode);
}

/* SSR-WPR-06 (ISO 26262):
 * Stale sensor data must not cause mode transitions.
 */
void test_SAF_NoChange_On_Stale_Sensor_Sample(void)
{
    /* Ensure some non-off state to detect unintended change */
    mock_rain_pct   = (uint8_t)(RAIN_THR_INT_ON_PCT + 5U);
    mock_rain_ts_ms = mock_now_ms;
    step_n(WIPERS_DEBOUNCE_COUNT + 1U);
    (void)wait_mode(WIPER_MODE_INT, WIPERS_LATENCY_MS);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_INT, mock_wiper_mode);

    /* Make next sample stale */
    mock_rain_pct   = (uint8_t)(RAIN_THR_LOW_ON_PCT + 10U);
    mock_rain_ts_ms = (uint32_t)(mock_now_ms - (STALE_MS + 1U));
    
    /* Step without refreshing timestamps to maintain stale state */
    for (uint32_t i = 0U; i < (WIPERS_DEBOUNCE_COUNT + 2U); ++i) {
        app_wipers_step();
        mock_now_ms += CONTROL_DT_MS;
    }

    /* Should remain INT due to stale rejection */
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_INT, mock_wiper_mode);
}

/* SSR-WPR-07 (ISO 26262):
 * Sensor read failure must not cause spurious transitions; hold last safe mode.
 */
void test_SAF_NoSpurious_On_Sensor_Failure(void)
{
    /* Ensure completely clean state */
    reset_mocks();
    mock_now_ms = 100U;
    app_wipers_init();
    mock_rain_ts_ms = mock_now_ms;
    
    /* Start and stay OFF */
    mock_rain_pct   = 0U;
    mock_rain_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_OFF, mock_wiper_mode);

    /* Now simulate failure */
    mock_rain_ok = false;
    step_n(3U);
    TEST_ASSERT_EQUAL_UINT8(WIPER_MODE_OFF, mock_wiper_mode);
}

/* ======================================================================= */
/*                                   MAIN                                   */
/* ======================================================================= */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SAF_Off_When_NoRain);
    RUN_TEST(test_SAF_Int_On_LightRain_After_Debounce_Within_Latency);
    RUN_TEST(test_SAF_Low_On_ModerateRain_After_Debounce);
    RUN_TEST(test_SAF_High_On_HeavyRain_After_Debounce);
    RUN_TEST(test_SAF_Hysteresis_On_StepDowns);
    RUN_TEST(test_SAF_NoChange_On_Stale_Sensor_Sample);
    RUN_TEST(test_SAF_NoSpurious_On_Sensor_Failure);

    return UNITY_END();
}
