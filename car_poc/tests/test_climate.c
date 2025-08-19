#include "unity.h"
#include "app_climate.h"

static int16_t mock_cabin_temp = 200;
static int16_t mock_ambient_temp = 250;
static uint8_t mock_humidity = 45U;
static uint32_t mock_timestamp_ms = 50U;
static uint8_t mock_fan_stage = 0U;
static bool mock_ac_on = false;
static uint8_t mock_blend_pct = 50U;
static uint32_t mock_current_time = 100U;

uint32_t hal_now_ms(void) {
    return mock_current_time;
}

bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms) {
    if ((out_tc_x10 != NULL) && (out_ts_ms != NULL)) {
        *out_tc_x10 = mock_cabin_temp;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms) {
    if ((out_tc_x10 != NULL) && (out_ts_ms != NULL)) {
        *out_tc_x10 = mock_ambient_temp;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms) {
    if ((out_pct != NULL) && (out_ts_ms != NULL)) {
        *out_pct = mock_humidity;
        *out_ts_ms = mock_timestamp_ms;
        return true;
    }
    return false;
}

void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct) {
    mock_fan_stage = fan_stage;
    mock_ac_on = ac_on;
    mock_blend_pct = blend_pct;
}

void setUp(void) {
    mock_cabin_temp = 200;
    mock_ambient_temp = 250;
    mock_humidity = 45U;
    mock_timestamp_ms = 50U;
    mock_fan_stage = 0U;
    mock_ac_on = false;
    mock_blend_pct = 50U;
    mock_current_time = 100U;
    app_climate_init();
}

void tearDown(void) {
}

void test_climate_cold_cabin_heating(void) {
    mock_cabin_temp = 180;
    mock_current_time = 2000U;
    
    app_climate_step();
    
    TEST_ASSERT_TRUE(mock_fan_stage > 0U);
    TEST_ASSERT_EQUAL_UINT8(100U, mock_blend_pct);
}

void test_climate_hot_cabin_cooling(void) {
    mock_cabin_temp = 260;
    mock_current_time = 2000U;
    
    app_climate_step();
    
    TEST_ASSERT_TRUE(mock_fan_stage > 0U);
    TEST_ASSERT_TRUE(mock_ac_on);
}

void test_climate_high_humidity_ac_on(void) {
    mock_humidity = 80U;
    mock_current_time = 2000U;
    
    app_climate_step();
    
    TEST_ASSERT_TRUE(mock_ac_on);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_climate_cold_cabin_heating);
    RUN_TEST(test_climate_hot_cabin_cooling);
    RUN_TEST(test_climate_high_humidity_ac_on);
    
    return UNITY_END();
}