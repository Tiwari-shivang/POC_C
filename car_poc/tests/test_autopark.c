/*
 * test_autopark.c
 *
 * ISO 26262–aligned unit tests for Auto-Park controller.
 * - Tests are tagged with requirement IDs (SSR-APK-xx) and rationale.
 * - Boundaries, timing, and fault-injection are covered.
 * - No magic numbers: uses config constants with safe fallbacks.
 */

#include "unity.h"
#include "app_autopark.h"
#include "hal.h"
#include <stdbool.h>
#include <stdint.h>

/* --------- Project constants (use your config.h if present) ------------- */
#include "config.h"

#ifndef AUTOPARK_MAX_SPEED_KPH
#define AUTOPARK_MAX_SPEED_KPH         (10U)
#endif

#ifndef AUTOPARK_MIN_GAP_MM
#define AUTOPARK_MIN_GAP_MM            (5500U)
#endif

#ifndef AUTOPARK_DEBOUNCE_COUNT
#define AUTOPARK_DEBOUNCE_COUNT        (3U)
#endif

#ifndef AUTOPARK_ACTIVATE_LATENCY_MS
#define AUTOPARK_ACTIVATE_LATENCY_MS   (200U)
#endif

#ifndef STALE_MS
#define STALE_MS                       (100U)
#endif

#ifndef CONTROL_DT_MS
#define CONTROL_DT_MS                  (10U)
#endif

/* Prompt codes (fallbacks; keep equal to app’s values if they exist) */
#ifndef AUTOPARK_PROMPT_SCAN
#define AUTOPARK_PROMPT_SCAN           (1U)
#endif
#ifndef AUTOPARK_PROMPT_ALIGN
#define AUTOPARK_PROMPT_ALIGN          (2U)
#endif

/* ---------------- HAL test doubles (deterministic stubs) ---------------- */

static bool     mock_gap_found      = false;
static uint16_t mock_gap_width_mm   = 0U;
static uint32_t mock_sample_ts_ms   = 0U;

static uint16_t mock_speed_kph      = 0U;
static uint32_t mock_speed_ts_ms    = 0U;

static uint8_t  mock_prompt_code    = 0U;
static uint32_t mock_now_ms         = 0U;

static bool     mock_gap_read_ok    = true;
static bool     mock_speed_read_ok  = true;

uint32_t hal_now_ms(void) { return mock_now_ms; }

bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms)
{
    if (!mock_speed_read_ok || (out_kph == NULL) || (out_ts_ms == NULL))
        return false;
    *out_kph  = mock_speed_kph;
    *out_ts_ms = mock_speed_ts_ms;
    return true;
}

bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms)
{
    if (!mock_gap_read_ok || (out == NULL) || (out_ts_ms == NULL))
        return false;
    out->found   = mock_gap_found;
    out->width_mm = mock_gap_width_mm;
    *out_ts_ms    = mock_sample_ts_ms;
    return true;
}

void hal_actuate_parking_prompt(uint8_t step_code)
{
    mock_prompt_code = step_code;
}

/* ---------------------------- Helpers ----------------------------------- */

static void reset_mocks(void)
{
    mock_gap_found     = false;
    mock_gap_width_mm  = 0U;
    mock_sample_ts_ms  = 0U;

    mock_speed_kph     = 5U;
    mock_speed_ts_ms   = 0U;

    mock_prompt_code   = 0U;
    mock_now_ms        = 100U;

    mock_gap_read_ok   = true;
    mock_speed_read_ok = true;
}

/* advance controller by n steps of CONTROL_DT_MS each */
static void step_n(uint32_t n)
{
    for (uint32_t i = 0U; i < n; ++i) {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        mock_speed_ts_ms = mock_now_ms;
        app_autopark_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

/* Wait until prompt == code, or timeout (ms). Returns true if reached. */
static bool wait_prompt(uint8_t code, uint32_t budget_ms)
{
    uint32_t elapsed = 0U;
    while ((mock_prompt_code != code) && (elapsed <= budget_ms)) {
        mock_sample_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        mock_speed_ts_ms = mock_now_ms;
        app_autopark_step();
        mock_now_ms += CONTROL_DT_MS;
        elapsed     += CONTROL_DT_MS;
    }
    return (mock_prompt_code == code) && (elapsed <= budget_ms);
}

/* ---------------------------- Unity fixtures ---------------------------- */

void setUp(void)
{
    reset_mocks();
    app_autopark_init();
    mock_speed_ts_ms  = mock_now_ms;
    mock_sample_ts_ms = mock_now_ms;
}

void tearDown(void) {}

/* ======================================================================= */
/*                             SAFETY TESTS                                */
/* ======================================================================= */

/* SSR-APK-01 (ISO 26262): When no valid gap is present, system remains in
 * SCAN and prompts driver accordingly.
 */
void test_SAF_ScanPrompt_When_NoGap(void)
{
    mock_gap_found = false;
    step_n(1U);
    TEST_ASSERT_EQUAL_UINT8(AUTOPARK_PROMPT_SCAN, mock_prompt_code);
}

/* SSR-APK-02 (ISO 26262): Valid gap (width >= MIN) at allowed speed
 * shall transition to ALIGN prompt after AUTOPARK_DEBOUNCE_COUNT consistent
 * detections and within activation latency.
 */
void test_SAF_AlignPrompt_On_ValidGap_After_Debounce_Within_Latency(void)
{
    mock_gap_found     = true;
    mock_gap_width_mm  = AUTOPARK_MIN_GAP_MM;     /* boundary: exactly min */
    mock_speed_kph     = AUTOPARK_MAX_SPEED_KPH;  /* boundary: exactly max */
    mock_sample_ts_ms  = mock_now_ms;
    mock_speed_ts_ms   = mock_now_ms;

    /* satisfy debounce */
    step_n(AUTOPARK_DEBOUNCE_COUNT);

    /* now wait for ALIGN within latency */
    bool ok = wait_prompt(AUTOPARK_PROMPT_ALIGN, AUTOPARK_ACTIVATE_LATENCY_MS);
    TEST_ASSERT_TRUE(ok);
}

/* SSR-APK-03 (ISO 26262): System must NOT engage when vehicle speed
 * is above AUTOPARK_MAX_SPEED_KPH.
 */
void test_SAF_NoAlign_When_SpeedTooHigh(void)
{
    mock_gap_found     = true;
    mock_gap_width_mm  = (uint16_t)(AUTOPARK_MIN_GAP_MM + 500U);
    mock_speed_kph     = (uint16_t)(AUTOPARK_MAX_SPEED_KPH + 1U);
    mock_sample_ts_ms  = mock_now_ms;
    mock_speed_ts_ms   = mock_now_ms;

    step_n(AUTOPARK_DEBOUNCE_COUNT + 3U);
    TEST_ASSERT_TRUE(mock_prompt_code != AUTOPARK_PROMPT_ALIGN);
}

/* SSR-APK-04 (ISO 26262): Debounce enforcement—fewer than
 * AUTOPARK_DEBOUNCE_COUNT consistent detections must NOT transition.
 */
void test_SAF_NoAlign_If_Debounce_NotSatisfied(void)
{
    mock_gap_found     = true;
    mock_gap_width_mm  = (uint16_t)(AUTOPARK_MIN_GAP_MM + 50U);
    mock_speed_kph     = AUTOPARK_MAX_SPEED_KPH;

    step_n((AUTOPARK_DEBOUNCE_COUNT > 0U) ? (AUTOPARK_DEBOUNCE_COUNT - 1U) : 0U);
    TEST_ASSERT_TRUE(mock_prompt_code != AUTOPARK_PROMPT_ALIGN);
}

/* SSR-APK-05 (ISO 26262): Stale sensor data must not cause transition. */
void test_SAF_NoAlign_On_Stale_Sensor_Sample(void)
{
    mock_gap_found     = true;
    mock_gap_width_mm  = (uint16_t)(AUTOPARK_MIN_GAP_MM + 100U);
    mock_speed_kph     = AUTOPARK_MAX_SPEED_KPH;

    /* Step without stale data to ensure we don't get transitions */
    for (uint32_t i = 0U; i < (AUTOPARK_DEBOUNCE_COUNT + 2U); ++i) {
        /* make gap sample stale for each step */
        mock_sample_ts_ms  = (uint32_t)(mock_now_ms - (STALE_MS + 1U));
        mock_speed_ts_ms = mock_now_ms; /* Keep speed fresh */
        app_autopark_step();
        mock_now_ms += CONTROL_DT_MS;
    }
    TEST_ASSERT_TRUE(mock_prompt_code != AUTOPARK_PROMPT_ALIGN);
}

/* SSR-APK-06 (ISO 26262): Sensor read failures (gap or speed) must not
 * cause transitions.
 */
void test_SAF_NoAlign_On_Sensor_Read_Failure(void)
{
    /* Fail gap read */
    mock_gap_read_ok   = false;
    mock_gap_found     = true;
    mock_gap_width_mm  = (uint16_t)(AUTOPARK_MIN_GAP_MM + 200U);
    step_n(AUTOPARK_DEBOUNCE_COUNT + 2U);
    TEST_ASSERT_TRUE(mock_prompt_code != AUTOPARK_PROMPT_ALIGN);

    /* Recover gap; fail speed read */
    mock_gap_read_ok   = true;
    mock_speed_read_ok = false;
    step_n(2U);
    TEST_ASSERT_TRUE(mock_prompt_code != AUTOPARK_PROMPT_ALIGN);
}

/* SSR-APK-07 (ISO 26262): If ALIGN has been prompted and gap becomes
 * invalid (lost or too small), system must revert to SCAN.
 */
void test_SAF_Revert_To_Scan_When_Gap_Lost(void)
{
    /* Ensure completely clean state */
    reset_mocks();
    mock_now_ms = 100U;
    app_autopark_init();
    mock_speed_ts_ms  = mock_now_ms;
    mock_sample_ts_ms = mock_now_ms;
    
    /* First, reach ALIGN */
    mock_gap_found     = true;
    mock_gap_width_mm  = (uint16_t)(AUTOPARK_MIN_GAP_MM + 100U);
    mock_speed_kph     = AUTOPARK_MAX_SPEED_KPH;
    mock_sample_ts_ms  = mock_now_ms;
    step_n(AUTOPARK_DEBOUNCE_COUNT + 2U);
    (void)wait_prompt(AUTOPARK_PROMPT_ALIGN, AUTOPARK_ACTIVATE_LATENCY_MS);
    TEST_ASSERT_EQUAL_UINT8(AUTOPARK_PROMPT_ALIGN, mock_prompt_code);

    /* Then lose the gap */
    mock_gap_found     = false;
    mock_sample_ts_ms  = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_EQUAL_UINT8(AUTOPARK_PROMPT_SCAN, mock_prompt_code);
}

/* =============================== MAIN =================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SAF_ScanPrompt_When_NoGap);
    RUN_TEST(test_SAF_AlignPrompt_On_ValidGap_After_Debounce_Within_Latency);
    RUN_TEST(test_SAF_NoAlign_When_SpeedTooHigh);
    RUN_TEST(test_SAF_NoAlign_If_Debounce_NotSatisfied);
    RUN_TEST(test_SAF_NoAlign_On_Stale_Sensor_Sample);
    RUN_TEST(test_SAF_NoAlign_On_Sensor_Read_Failure);
    RUN_TEST(test_SAF_Revert_To_Scan_When_Gap_Lost);

    return UNITY_END();
}
