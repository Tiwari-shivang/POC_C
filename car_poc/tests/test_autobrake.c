#include "unity.h"
#include "app_autobrake.h"
#include <stdbool.h>

static uint16_t mock_distance_mm = 2000U;
static uint32_t mock_timestamp_ms = 0U;
static bool mock_vehicle_ready = true;
static bool mock_driver_brake = false;
static bool mock_brake_request = false;
static uint32_t mock_current_time = 0U;

bool hal_get_vehicle_ready(void) {
    return mock_vehicle_ready;
}

bool hal_driver_brake_pressed(void) {
    return mock_driver_brake;
}

uint32_t hal_now_ms(void) {
    return mock_current_time;
}

bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms) {
    if ((out_mm != NULL) && (out_ts_ms != NULL)) {
        *out_mm = mock_distance_mm;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

void hal_set_brake_request(bool on) {
    mock_brake_request = on;
}

void setUp(void) {
    mock_distance_mm = 2000U;
    mock_timestamp_ms = 0U;
    mock_vehicle_ready = true;
    mock_driver_brake = false;
    mock_brake_request = false;
    mock_current_time = 100U;
    app_autobrake_init();
}

void tearDown(void) {
}

void test_autobrake_no_brake_when_distance_safe(void) {
    mock_distance_mm = 2000U;
    mock_timestamp_ms = 50U;
    mock_current_time = 100U;
    
    app_autobrake_step();
    
    TEST_ASSERT_FALSE(mock_brake_request);
}

void test_autobrake_no_brake_when_vehicle_not_ready(void) {
    mock_vehicle_ready = false;
    mock_distance_mm = 1000U;
    mock_timestamp_ms = 50U;
    mock_current_time = 100U;
    
    app_autobrake_step();
    
    TEST_ASSERT_FALSE(mock_brake_request);
}

void test_autobrake_no_brake_when_driver_override(void) {
    mock_driver_brake = true;
    mock_distance_mm = 1000U;
    mock_timestamp_ms = 50U;
    mock_current_time = 100U;
    
    app_autobrake_step();
    
    TEST_ASSERT_FALSE(mock_brake_request);
}

void test_autobrake_debounce_before_activation(void) {
    /* Reset all state completely */
    app_autobrake_init();
    mock_distance_mm = 800U; /* Well below threshold of 1220U */
    mock_timestamp_ms = 50U;
    mock_current_time = 100U;
    mock_vehicle_ready = true;
    mock_driver_brake = false;
    mock_brake_request = false;
    
    /* First step - should not brake */
    app_autobrake_step();
    TEST_ASSERT_FALSE(mock_brake_request);
    
    /* Second step - should not brake */
    app_autobrake_step();
    TEST_ASSERT_FALSE(mock_brake_request);
    
    /* Third step - should brake */
    app_autobrake_step();
    TEST_ASSERT_TRUE(mock_brake_request);
}

void test_autobrake_stale_sensor_data(void) {
    mock_distance_mm = 1000U;
    mock_timestamp_ms = 0U;
    mock_current_time = 200U;
    
    app_autobrake_step();
    
    TEST_ASSERT_FALSE(mock_brake_request);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_autobrake_no_brake_when_distance_safe);
    RUN_TEST(test_autobrake_no_brake_when_vehicle_not_ready);
    RUN_TEST(test_autobrake_no_brake_when_driver_override);
    RUN_TEST(test_autobrake_debounce_before_activation);
    RUN_TEST(test_autobrake_stale_sensor_data);
    
    return UNITY_END();
}