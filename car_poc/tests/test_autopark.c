#include "unity.h"
#include "app_autopark.h"

static bool mock_gap_found = false;
static uint16_t mock_gap_width = 0U;
static uint32_t mock_timestamp_ms = 50U;
static uint16_t mock_speed_kph = 5U;
static uint8_t mock_prompt_code = 0U;
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

bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms) {
    if ((out != NULL) && (out_ts_ms != NULL)) {
        out->found = mock_gap_found;
        out->width_mm = mock_gap_width;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

void hal_actuate_parking_prompt(uint8_t step_code) {
    mock_prompt_code = step_code;
}

void setUp(void) {
    mock_gap_found = false;
    mock_gap_width = 0U;
    mock_timestamp_ms = 50U;
    mock_speed_kph = 5U;
    mock_prompt_code = 0U;
    mock_current_time = 100U;
    app_autopark_init();
}

void tearDown(void) {
}

void test_autopark_scanning_state(void) {
    mock_gap_found = false;
    
    app_autopark_step();
    
    TEST_ASSERT_EQUAL_UINT8(1U, mock_prompt_code);
}

void test_autopark_gap_detection(void) {
    mock_gap_found = true;
    mock_gap_width = 6000U;
    
    app_autopark_step();
    app_autopark_step();
    app_autopark_step();
    
    TEST_ASSERT_EQUAL_UINT8(2U, mock_prompt_code);
}

void test_autopark_speed_too_high(void) {
    mock_speed_kph = 15U;
    mock_gap_found = true;
    mock_gap_width = 6000U;
    
    app_autopark_step();
    
    TEST_ASSERT_EQUAL_UINT8(0U, mock_prompt_code);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_autopark_scanning_state);
    RUN_TEST(test_autopark_gap_detection);
    RUN_TEST(test_autopark_speed_too_high);
    
    return UNITY_END();
}