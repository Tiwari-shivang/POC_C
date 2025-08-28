/*
 * test_climate.c
 *
 * ISO 26262–aligned unit tests for the climate controller.
 * - Each test is tagged with a safety requirement ID (SSR-CLM-xx).
 * - Boundaries, timing and fault injection are included.
 * - No magic numbers: uses project config constants when available,
 *   with safe fallbacks here to keep tests deterministic.
 */

#include "unity.h"
#include "app_climate.h"
#include <stdbool.h>
#include <stdint.h>

/* ---- Prefer project constants; provide fallbacks if not defined --------- */
#include "config.h"          /* If you have it; otherwise the #ifndefs apply */

#ifndef CLIMATE_TARGET_C_X10
#define CLIMATE_TARGET_C_X10           (220)     /* 22.0°C target */
#endif

#ifndef CLIMATE_HUMIDITY_HIGH_PCT
#define CLIMATE_HUMIDITY_HIGH_PCT      (70U)     /* high humidity threshold */
#endif

#ifndef CLIMATE_LATENCY_MS
#define CLIMATE_LATENCY_MS             (200U)    /* must react within this time */
#endif

#ifndef CONTROL_DT_MS
#define CONTROL_DT_MS                  (10U)     /* controller period */
#endif

#ifndef FAN_STAGE_MAX
#define FAN_STAGE_MAX                  (5U)      /* fallback max fan stage */
#endif

/* ---- HAL test doubles --------------------------------------------------- */

static int16_t  mock_cabin_temp_x10   = 200;     /* 20.0°C */
static int16_t  mock_ambient_temp_x10 = 250;     /* 25.0°C */
static uint8_t  mock_humidity_pct     = 45U;
static uint32_t mock_sample_ts_ms     = 50U;

static uint8_t  mock_fan_stage        = 0U;
static bool     mock_ac_on            = false;
static uint8_t  mock_blend_pct        = 50U;     /* 0=cold, 100=hot mix */

static uint32_t mock_now_ms           = 100U;

static bool     mock_cabin_ok         = true;
static bool     mock_amb_ok           = true;
static bool     mock_hum_ok           = true;

uint32_t hal_now_ms(void) { return mock_now_ms; }

bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms)
{
    if (!mock_cabin_ok || (out_tc_x10 == NULL) || (out_ts_ms == NULL)) return false;
    *out_tc_x10 = mock_cabin_temp_x10;
    *out_ts_ms  = mock_sample_ts_ms;
    return true;
}

bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms)
{
    if (!mock_amb_ok || (out_tc_x10 == NULL) || (out_ts_ms == NULL)) return false;
    *out_tc_x10 = mock_ambient_temp_x10;
    *out_ts_ms  = mock_sample_ts_ms;
    return true;
}

bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms)
{
    if (!mock_hum_ok || (out_pct == NULL) || (out_ts_ms == NULL)) return false;
    *out_pct   = mock_humidity_pct;
    *out_ts_ms = mock_sample_ts_ms;
    return true;
}

void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct)
{
    mock_fan_stage = fan_stage;
    mock_ac_on     = ac_on;
    mock_blend_pct = blend_pct;
}

/* ---- Helpers ------------------------------------------------------------ */

static void reset_mocks(void)
{
    mock_cabin_temp_x10   = 200;
    mock_ambient_temp_x10 = 250;
    mock_humidity_pct     = 45U;
    mock_sample_ts_ms     = 50U;

    mock_fan_stage        = 0U;
    mock_ac_on            = false;
    mock_blend_pct        = 50U;

    mock_now_ms           = 100U;

    mock_cabin_ok         = true;
    mock_amb_ok           = true;
    mock_hum_ok           = true;
}

static void step_n(uint32_t n)
{
    for (uint32_t i = 0U; i < n; ++i) {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_climate_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

/* Wait until predicate (written as lambda via boolean variable update) or timeout */
static bool wait_until_change(uint32_t budget_ms, uint8_t min_fan, bool want_ac)
{
    uint32_t elapsed = 0U;
    while (elapsed <= budget_ms) {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_climate_step();
        if ((mock_fan_stage >= min_fan) && (mock_ac_on == want_ac)) {
            return true;
        }
        mock_now_ms += CONTROL_DT_MS;
        elapsed     += CONTROL_DT_MS;
    }
    return false;
}

/* ---- Unity fixtures ----------------------------------------------------- */

void setUp(void)
{
    reset_mocks();
    app_climate_init();
}

void tearDown(void) {}

/* ======================================================================== */
/*                               TEST CASES                                 */
/* ======================================================================== */

/* SSR-CLM-01 (ISO 26262): If cabin temp < target, system shall heat:
 * increase fan above 0 and bias blend toward HOT within CLIMATE_LATENCY_MS.
 */
void test_SAF_Heating_When_Cabin_Below_Target(void)
{
    mock_cabin_temp_x10 = (int16_t)(CLIMATE_TARGET_C_X10 - 30); /* 3.0°C below target */
    mock_sample_ts_ms   = mock_now_ms;

    bool ok = wait_until_change(CLIMATE_LATENCY_MS, /*min_fan*/1U, /*want_ac*/false);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_TRUE(mock_blend_pct >= 50U); /* toward HOT */
}

/* SSR-CLM-02 (ISO 26262): If cabin temp > target, system shall cool:
 * turn AC on and increase fan within CLIMATE_LATENCY_MS.
 */
void test_SAF_Cooling_When_Cabin_Above_Target(void)
{
    mock_cabin_temp_x10 = (int16_t)(CLIMATE_TARGET_C_X10 + 40); /* 4.0°C above */
    mock_sample_ts_ms   = mock_now_ms;

    bool ok = wait_until_change(CLIMATE_LATENCY_MS, /*min_fan*/1U, /*want_ac*/true);
    TEST_ASSERT_TRUE(ok);
    /* Blend likely toward cold; ensure in valid range */
    TEST_ASSERT_TRUE(mock_blend_pct <= 100U);
}

/* SSR-CLM-03 (ISO 26262): Exactly at target → no aggressive action:
 * fan may be low/idle; AC off by default.
 */
void test_SAF_At_Target_No_Aggressive_Action(void)
{
    mock_cabin_temp_x10 = CLIMATE_TARGET_C_X10;   /* boundary */
    mock_sample_ts_ms   = mock_now_ms;

    step_n(5U);
    TEST_ASSERT_TRUE(mock_fan_stage <= 1U);
    TEST_ASSERT_FALSE(mock_ac_on);
}

/* SSR-CLM-04 (ISO 26262): High humidity shall enable AC for dehumidification. */
void test_SAF_HighHumidity_Enables_AC(void)
{
    mock_humidity_pct   = (uint8_t)(CLIMATE_HUMIDITY_HIGH_PCT + 1U);
    mock_sample_ts_ms   = mock_now_ms;

    step_n(3U);
    TEST_ASSERT_TRUE(mock_ac_on);
}

/* SSR-CLM-05 (ISO 26262): Stale samples must not drive actuation changes. */
void test_SAF_No_Action_On_Stale_Sensor_Data(void)
{
    /* Push controller with stale data; outputs should remain safe/unchanged (fan 0, AC off) */
    for (uint32_t i = 0U; i < 5U; ++i) {
        /* Make inputs stale for each step */
        mock_sample_ts_ms = (uint32_t)(mock_now_ms - (CLIMATE_LATENCY_MS + 1U));
        app_climate_step();
        mock_now_ms += CONTROL_DT_MS;
    }
    TEST_ASSERT_EQUAL_UINT8(0U, mock_fan_stage);
    TEST_ASSERT_FALSE(mock_ac_on);
}

/* SSR-CLM-06 (ISO 26262): Sensor read failures must result in safe output. */
void test_SAF_Safe_Defaults_On_Sensor_Failure(void)
{
    mock_cabin_ok = false;  /* cabin temp read fails */
    mock_amb_ok   = true;
    mock_hum_ok   = true;

    step_n(3U);
    TEST_ASSERT_EQUAL_UINT8(0U, mock_fan_stage);
    TEST_ASSERT_FALSE(mock_ac_on);
    TEST_ASSERT_TRUE(mock_blend_pct <= 100U); /* still within range */
}

/* SSR-CLM-07 (ISO 26262): Output limits are always respected. */
void test_SAF_Output_Clamps_Within_Range(void)
{
    /* Drive controller through a few steps with extreme inputs */
    mock_cabin_temp_x10 = (int16_t)(CLIMATE_TARGET_C_X10 + 200);  /* +20°C */
    mock_humidity_pct   = (uint8_t)(CLIMATE_HUMIDITY_HIGH_PCT + 20U);
    step_n(10U);

    TEST_ASSERT_TRUE(mock_fan_stage <= FAN_STAGE_MAX);
    TEST_ASSERT_TRUE(mock_blend_pct <= 100U);
    TEST_ASSERT_TRUE(mock_blend_pct >= 0U);
}

/* ======================================================================== */
/*                                   MAIN                                   */
/* ======================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SAF_Heating_When_Cabin_Below_Target);
    RUN_TEST(test_SAF_Cooling_When_Cabin_Above_Target);
    RUN_TEST(test_SAF_At_Target_No_Aggressive_Action);
    RUN_TEST(test_SAF_HighHumidity_Enables_AC);
    RUN_TEST(test_SAF_No_Action_On_Stale_Sensor_Data);
    RUN_TEST(test_SAF_Safe_Defaults_On_Sensor_Failure);
    RUN_TEST(test_SAF_Output_Clamps_Within_Range);

    return UNITY_END();
}
