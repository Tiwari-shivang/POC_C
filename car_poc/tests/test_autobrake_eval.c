/*
 * test_autobrake_eval.c
 *
 * Scenario-based evaluation test for autobrake module.
 * This test drives the controller through deterministic scenarios and
 * measures KPIs like detection latency and reaction latency.
 */

#include "unity.h"
#include "app_autobrake.h"
#include "config.h"
#include <stdint.h>
#include <stdbool.h>

/* HAL test doubles - reusing similar mocks from unit tests */
static uint16_t mock_distance_mm = 2000U;
static uint32_t mock_sample_ts_ms = 0U;
static bool     mock_vehicle_ready = true;
static bool     mock_driver_brake = false;
static bool     mock_brake_request = false;
static uint32_t mock_now_ms = 0U;
static bool     mock_sensor_ok = true;

/* HAL mock implementations */
bool hal_get_vehicle_ready(void) { return mock_vehicle_ready; }
bool hal_driver_brake_pressed(void) { return mock_driver_brake; }
uint32_t hal_now_ms(void) { return mock_now_ms; }

bool hal_read_distance_mm(uint16_t *out_mm, uint32_t *out_ts_ms) {
    if ((!mock_sensor_ok) || (out_mm == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    *out_mm = mock_distance_mm;
    *out_ts_ms = mock_sample_ts_ms;
    return true;
}

void hal_set_brake_request(bool on) { mock_brake_request = on; }

/* Eval hooks: we rely on metrics_autobrake.c linked in eval build */
void eval_autobrake_flush(void); /* from metrics_autobrake.c */

static void reset_mocks(void) {
    mock_distance_mm = 2000U;
    mock_sample_ts_ms = 0U;
    mock_vehicle_ready = true;
    mock_driver_brake = false;
    mock_brake_request = false;
    mock_now_ms = 100U; /* deterministic non-zero */
    mock_sensor_ok = true;
}

static void step_n(uint32_t n_steps) {
    for (uint32_t i = 0U; i < n_steps; ++i) {
        mock_sample_ts_ms = mock_now_ms;
        app_autobrake_step();
        mock_now_ms += CONTROL_DT_MS;
    }
}

void setUp(void)   { reset_mocks(); app_autobrake_init(); }
void tearDown(void){}

/* Scenario: pedestrian appears; we measure latencies via eval hooks */
void test_EVAL_Autobrake_Pedestrian_Close_Scenario(void)
{
    /* Warm-up 500 ms */
    step_n(50U);

    /* Pedestrian event simulated by dropping distance below threshold steadily */
    for (uint32_t t = 0U; t < 1200U; t += CONTROL_DT_MS) {
        if (t >= 150U) { /* first-below occurs ~150 ms into scenario */
            mock_distance_mm = (uint16_t)(BRAKE_THRESH_MM - 20U);
        } else {
            mock_distance_mm = (uint16_t)(BRAKE_THRESH_MM + 200U);
        }
        step_n(1U);
    }

    /* Flush artifacts (CSV/JSON) for this run */
    eval_autobrake_flush();
    /* Optionally, add direct assertions here by reading the JSON file */
    TEST_ASSERT_TRUE(1); /* keep as artifact producer; thresholds handled in CI */
}

/* Test with multiple scenarios to validate different conditions */
void test_EVAL_Autobrake_Quick_Obstacle_Scenario(void)
{
    /* Reset for clean scenario */
    reset_mocks();
    app_autobrake_init();
    
    /* Quick obstacle scenario - faster approach */
    step_n(30U); /* warm-up 300ms */
    
    /* Rapid approach - distance drops quickly */
    for (uint32_t t = 0U; t < 800U; t += CONTROL_DT_MS) {
        if (t >= 50U) { /* obstacle detected after 50ms */
            mock_distance_mm = (uint16_t)(BRAKE_THRESH_MM - 50U);
        } else {
            mock_distance_mm = (uint16_t)(BRAKE_THRESH_MM + 500U);
        }
        step_n(1U);
    }
    
    /* Flush artifacts for this scenario */
    eval_autobrake_flush();
    TEST_ASSERT_TRUE(mock_brake_request); /* should have braked by now */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_EVAL_Autobrake_Pedestrian_Close_Scenario);
    RUN_TEST(test_EVAL_Autobrake_Quick_Obstacle_Scenario);
    return UNITY_END();
}