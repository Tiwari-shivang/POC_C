/*
 * test_speedgov.c
 *
 * ISO 26262–aligned unit tests for Speed Governor.
 * - Tests tagged with safety requirement IDs (SSR-GOV-xx).
 * - Boundary, timing (latency/debounce), hysteresis, and fault-injection covered.
 * - Uses project config constants when available; safe fallbacks otherwise.
 */

#include "unity.h"
#include "app_speedgov.h"
#include <stdbool.h>
#include <stdint.h>

/* ---- Prefer your project config; fall back if not provided -------------- */
#include "config.h"

#ifndef SPEEDGOV_DEFAULT_LIMIT_KPH
#define SPEEDGOV_DEFAULT_LIMIT_KPH        (50U)
#endif

#ifndef SPEEDGOV_DEBOUNCE_COUNT
#define SPEEDGOV_DEBOUNCE_COUNT           (2U)     /* consecutive overspeed samples */
#endif

#ifndef SPEEDGOV_HYSTERESIS_KPH
#define SPEEDGOV_HYSTERESIS_KPH           (3U)     /* below limit to clear alarm */
#endif

#ifndef SPEEDGOV_LATENCY_MS
#define SPEEDGOV_LATENCY_MS               (200U)   /* max time to raise alarm */
#endif

#ifndef STALE_MS
#define STALE_MS                          (100U)
#endif

#ifndef CONTROL_DT_MS
#define CONTROL_DT_MS                     (10U)    /* controller step period */
#endif

/* ---- HAL test doubles --------------------------------------------------- */

static uint16_t mock_speed_kph          = SPEEDGOV_DEFAULT_LIMIT_KPH;
static uint32_t mock_speed_ts_ms        = 0U;

static uint16_t mock_speed_limit_kph    = SPEEDGOV_DEFAULT_LIMIT_KPH;
static bool     mock_limit_available    = false;

static bool     mock_alarm_state        = false;
static uint16_t mock_limit_request      = 0U;     /* what controller asks to set */

static uint32_t mock_now_ms             = 0U;

/* Failure toggles */
static bool     mock_speed_read_ok      = true;

/* HAL shims the app calls */
uint32_t hal_now_ms(void) { return mock_now_ms; }

bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms)
{
    if (!mock_speed_read_ok || (out_kph == NULL) || (out_ts_ms == NULL)) return false;
    *out_kph  = mock_speed_kph;
    *out_ts_ms = mock_speed_ts_ms;
    return true;
}

/* One-shot provider: delivers limit when available, then resets flag */
bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph)
{
    if (mock_limit_available && (out_limit_kph != NULL)) {
        *out_limit_kph = mock_speed_limit_kph;
        mock_limit_available = false;
        return true;
    }
    return false;
}

void hal_set_alarm(bool on) { mock_alarm_state = on; }

void hal_set_speed_limit_request(uint16_t kph) { mock_limit_request = kph; }

/* ---- Helpers ------------------------------------------------------------ */

static void reset_mocks(void)
{
    mock_speed_kph        = SPEEDGOV_DEFAULT_LIMIT_KPH;
    mock_speed_ts_ms      = 0U;

    mock_speed_limit_kph  = SPEEDGOV_DEFAULT_LIMIT_KPH;
    mock_limit_available  = false;

    mock_alarm_state      = false;
    mock_limit_request    = 0U;

    mock_now_ms           = 100U;

    mock_speed_read_ok    = true;
}

static void step_n(uint32_t n)
{
    for (uint32_t i = 0U; i < n; ++i) {
        mock_speed_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_speedgov_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

static bool wait_alarm_state(bool want_on, uint32_t budget_ms)
{
    uint32_t elapsed = 0U;
    while ((mock_alarm_state != want_on) && (elapsed <= budget_ms)) {
        mock_speed_ts_ms = mock_now_ms; /* Keep sensor data fresh */
        app_speedgov_step();
        mock_now_ms += CONTROL_DT_MS;
        elapsed     += CONTROL_DT_MS;
    }
    return (mock_alarm_state == want_on) && (elapsed <= budget_ms);
}

/* ---- Unity fixtures ----------------------------------------------------- */

void setUp(void)
{
    reset_mocks();
    app_speedgov_init();
    mock_speed_ts_ms = mock_now_ms;
}

void tearDown(void) {}

/* ======================================================================== */
/*                              TEST CASES                                  */
/* ======================================================================== */

/* SSR-GOV-01 (ISO 26262): No alarm when speed <= limit (at/below boundary). */
void test_SAF_NoAlarm_AtOrBelow_Limit(void)
{
    /* at limit */
    mock_speed_kph   = SPEEDGOV_DEFAULT_LIMIT_KPH;
    mock_speed_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_FALSE(mock_alarm_state);

    /* below limit */
    mock_speed_kph   = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH - 1U);
    mock_speed_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_FALSE(mock_alarm_state);
}

/* SSR-GOV-02 (ISO 26262): Alarm asserted when speed > limit after
 * required debounce and within latency budget.
 */
void test_SAF_Alarm_On_Overspeed_Debounce_Within_Latency(void)
{
    mock_speed_kph   = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH + 1U);  /* just over */
    mock_speed_ts_ms = mock_now_ms;

    /* satisfy debounce first */
    step_n(SPEEDGOV_DEBOUNCE_COUNT);

    /* then ensure we assert within latency */
    bool ok = wait_alarm_state(true, SPEEDGOV_LATENCY_MS);
    TEST_ASSERT_TRUE(ok);
}

/* SSR-GOV-03 (ISO 26262): Hysteresis—alarm clears only when speed
 * falls below (limit - hysteresis).
 */
void test_SAF_Alarm_Clears_With_Hysteresis(void)
{
    /* Raise alarm first */
    mock_speed_kph   = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH + 5U);
    mock_speed_ts_ms = mock_now_ms;
    step_n(SPEEDGOV_DEBOUNCE_COUNT + 2U);
    (void)wait_alarm_state(true, SPEEDGOV_LATENCY_MS);
    TEST_ASSERT_TRUE(mock_alarm_state);

    /* Drop speed but not enough to cross hysteresis */
    mock_speed_kph   = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH - (SPEEDGOV_HYSTERESIS_KPH - 1U));
    mock_speed_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_TRUE(mock_alarm_state); /* still latched */

    /* Now drop below hysteresis boundary */
    mock_speed_kph   = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH - SPEEDGOV_HYSTERESIS_KPH);
    mock_speed_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_FALSE(mock_alarm_state);
}

/* SSR-GOV-04 (ISO 26262): Stale speed sample must not drive alarm behavior. */
void test_SAF_NoAlarm_On_Stale_Speed_Sample(void)
{
    mock_speed_kph    = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH + 10U);

    /* Step with stale data to ensure no alarm activation */
    for (uint32_t i = 0U; i < (SPEEDGOV_DEBOUNCE_COUNT + 3U); ++i) {
        /* Make speed sample stale for each step */
        mock_speed_ts_ms = (uint32_t)(mock_now_ms - (STALE_MS + 1U));
        app_speedgov_step();
        mock_now_ms += CONTROL_DT_MS;
    }
    TEST_ASSERT_FALSE(mock_alarm_state);
}

/* SSR-GOV-05 (ISO 26262): Sensor read failure → do not change alarm spuriously. */
void test_SAF_NoSpurious_On_Sensor_Failure(void)
{
    /* First, ensure alarm is off in normal state */
    mock_speed_kph   = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH - 5U);
    mock_speed_ts_ms = mock_now_ms;
    step_n(2U);
    TEST_ASSERT_FALSE(mock_alarm_state);

    /* Now simulate read failure */
    mock_speed_read_ok = false;
    step_n(3U);
    TEST_ASSERT_FALSE(mock_alarm_state);
}

/* SSR-GOV-06 (ISO 26262): Limit update—when a new limit is provided,
 * system shall request the new limit value.
 */
void test_SAF_Limit_Update_Request_Propagates(void)
{
    /* Ensure completely clean state */
    reset_mocks();
    mock_now_ms = 100U;
    app_speedgov_init();
    mock_speed_ts_ms = mock_now_ms;
    
    mock_speed_limit_kph = (uint16_t)(SPEEDGOV_DEFAULT_LIMIT_KPH + 30U);
    mock_limit_available = true;

    /* step until poll is consumed */
    step_n(2U);

    TEST_ASSERT_EQUAL_UINT16(mock_speed_limit_kph, mock_limit_request);
}

/* SSR-GOV-07 (ISO 26262): Ignore nonsensical limits (e.g., 0 kph). */
void test_SAF_Ignore_Invalid_Limit_Zero(void)
{
    mock_speed_limit_kph = 0U;   /* invalid */
    mock_limit_available = true;

    step_n(2U);

    /* Controller should not request setting an invalid limit to 0;
       implementation may keep previous request or remain unchanged. */
    TEST_ASSERT_TRUE(mock_limit_request != 0U);
}

/* ======================================================================== */
/*                                   MAIN                                   */
/* ======================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_SAF_NoAlarm_AtOrBelow_Limit);
    RUN_TEST(test_SAF_Alarm_On_Overspeed_Debounce_Within_Latency);
    RUN_TEST(test_SAF_Alarm_Clears_With_Hysteresis);
    RUN_TEST(test_SAF_NoAlarm_On_Stale_Speed_Sample);
    RUN_TEST(test_SAF_NoSpurious_On_Sensor_Failure);
    RUN_TEST(test_SAF_Limit_Update_Request_Propagates);
    RUN_TEST(test_SAF_Ignore_Invalid_Limit_Zero);

    return UNITY_END();
}
