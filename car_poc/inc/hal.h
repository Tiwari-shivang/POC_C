#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

bool     hal_get_vehicle_ready(void);
bool     hal_driver_brake_pressed(void);
uint32_t hal_now_ms(void);

bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms);
bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms);
bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms);
bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph);

typedef struct {
    bool found;
    uint16_t width_mm;
} park_gap_t;

bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms);
bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms);
bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms);
bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms);
bool hal_read_voice_line(char* buf, uint16_t len);

void hal_set_brake_request(bool on);
void hal_set_wiper_mode(uint8_t mode);
void hal_set_alarm(bool on);
void hal_set_speed_limit_request(uint16_t kph);
void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct);
void hal_actuate_parking_prompt(uint8_t step_code);

#endif /* HAL_H */