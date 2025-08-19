#include "unity.h"
#include "app_wipers.h"

static uint8_t mock_rain_pct = 0U;
static uint32_t mock_timestamp_ms = 0U;
static uint8_t mock_wiper_mode = 0U;
static uint32_t mock_current_time = 100U;

uint32_t hal_now_ms(void) {
    return mock_current_time;
}

bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms) {
    if ((out_pct != NULL) && (out_ts_ms != NULL)) {
        *out_pct = mock_rain_pct;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

void hal_set_wiper_mode(uint8_t mode) {
    mock_wiper_mode = mode;
}

void setUp(void) {
    mock_rain_pct = 0U;
    mock_timestamp_ms = 50U;
    mock_wiper_mode = 0U;
    mock_current_time = 100U;
    app_wipers_init();
}

void tearDown(void) {
}

void test_wipers_off_when_no_rain(void) {
    mock_rain_pct = 0U;
    
    app_wipers_step();
    
    TEST_ASSERT_EQUAL_UINT8(0U, mock_wiper_mode);
}

void test_wipers_intermittent_on_light_rain(void) {
    mock_rain_pct = 25U;
    
    app_wipers_step();
    
    TEST_ASSERT_EQUAL_UINT8(1U, mock_wiper_mode);
}

void test_wipers_low_on_moderate_rain(void) {
    mock_rain_pct = 50U;
    
    app_wipers_step();
    
    TEST_ASSERT_EQUAL_UINT8(2U, mock_wiper_mode);
}

void test_wipers_high_on_heavy_rain(void) {
    mock_rain_pct = 80U;
    
    app_wipers_step();
    
    TEST_ASSERT_EQUAL_UINT8(3U, mock_wiper_mode);
}

void test_wipers_hysteresis_behavior(void) {
    mock_rain_pct = 25U;
    app_wipers_step();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_wiper_mode);
    
    mock_rain_pct = 18U;
    app_wipers_step();
    TEST_ASSERT_EQUAL_UINT8(1U, mock_wiper_mode);
    
    mock_rain_pct = 10U;
    app_wipers_step();
    TEST_ASSERT_EQUAL_UINT8(0U, mock_wiper_mode);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_wipers_off_when_no_rain);
    RUN_TEST(test_wipers_intermittent_on_light_rain);
    RUN_TEST(test_wipers_low_on_moderate_rain);
    RUN_TEST(test_wipers_high_on_heavy_rain);
    RUN_TEST(test_wipers_hysteresis_behavior);
    
    return UNITY_END();
}