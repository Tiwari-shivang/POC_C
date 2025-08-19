#include "unity.h"
#include "app_speedgov.h"
#include <stdbool.h>

static uint16_t mock_speed_kph = 50U;
static uint32_t mock_timestamp_ms = 50U;
static uint16_t mock_speed_limit = 50U;
static bool mock_speed_limit_available = false;
static bool mock_alarm_state = false;
static uint16_t mock_limit_request = 0U;
static uint32_t mock_current_time = 100U;

uint32_t hal_now_ms(void) {
    return mock_current_time;
}

bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms) {
    if ((out_kph != NULL) && (out_ts_ms != NULL)) {
        *out_kph = mock_speed_kph;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph) {
    if (mock_speed_limit_available && (out_limit_kph != NULL)) {
        *out_limit_kph = mock_speed_limit;
        mock_speed_limit_available = false;
        return true;
    }
    return false;
}

void hal_set_alarm(bool on) {
    mock_alarm_state = on;
}

void hal_set_speed_limit_request(uint16_t kph) {
    mock_limit_request = kph;
}

void setUp(void) {
    mock_speed_kph = 50U;
    mock_timestamp_ms = 50U;
    mock_speed_limit = 50U;
    mock_speed_limit_available = false;
    mock_alarm_state = false;
    mock_limit_request = 0U;
    mock_current_time = 100U;
    app_speedgov_init();
}

void tearDown(void) {
}

void test_speedgov_no_alarm_under_limit(void) {
    mock_speed_kph = 45U;
    
    app_speedgov_step();
    
    TEST_ASSERT_FALSE(mock_alarm_state);
}

void test_speedgov_alarm_over_limit_with_debounce(void) {
    mock_speed_kph = 55U;
    
    app_speedgov_step();
    TEST_ASSERT_FALSE(mock_alarm_state);
    
    app_speedgov_step();
    TEST_ASSERT_TRUE(mock_alarm_state);
}

void test_speedgov_alarm_clear_with_hysteresis(void) {
    mock_speed_kph = 55U;
    app_speedgov_step();
    app_speedgov_step();
    TEST_ASSERT_TRUE(mock_alarm_state);
    
    mock_speed_kph = 48U;
    app_speedgov_step();
    TEST_ASSERT_TRUE(mock_alarm_state);
    
    mock_speed_kph = 44U;
    app_speedgov_step();
    TEST_ASSERT_FALSE(mock_alarm_state);
}

void test_speedgov_limit_update(void) {
    mock_speed_limit = 80U;
    mock_speed_limit_available = true;
    
    app_speedgov_step();
    
    TEST_ASSERT_EQUAL_UINT16(80U, mock_limit_request);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_speedgov_no_alarm_under_limit);
    RUN_TEST(test_speedgov_alarm_over_limit_with_debounce);
    RUN_TEST(test_speedgov_alarm_clear_with_hysteresis);
    RUN_TEST(test_speedgov_limit_update);
    
    return UNITY_END();
}