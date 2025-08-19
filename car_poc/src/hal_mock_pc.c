#include "hal.h"
#include "platform.h"
#include "scenario.h"
#include <stdio.h>
#include <string.h>

extern uint32_t platform_get_time_ms(void);

static scenario_row_t current_row;
static bool row_valid = false;
static bool driver_brake = false;
static bool vehicle_ready = true;
static bool speed_limit_event_pending = false;
static uint16_t pending_speed_limit = 0U;

static bool outputs_file_open = false;
static FILE* outputs_file = NULL;

bool hal_get_vehicle_ready(void) {
    return vehicle_ready;
}

bool hal_driver_brake_pressed(void) {
    return driver_brake;
}

uint32_t hal_now_ms(void) {
    return platform_get_time_ms();
}

static bool update_current_row(void) {
    if (scenario_get_next_row(&current_row)) {
        row_valid = true;
        if (current_row.sign_event > 0U) {
            speed_limit_event_pending = true;
            pending_speed_limit = current_row.sign_event;
        }
        return true;
    }
    return false;
}

bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms) {
    if ((out_mm == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    *out_mm = current_row.distance_mm;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms) {
    if ((out_pct == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    *out_pct = current_row.rain_pct;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms) {
    if ((out_kph == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    *out_kph = current_row.speed_kph;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph) {
    if (out_limit_kph == NULL) {
        return false;
    }
    
    if (speed_limit_event_pending) {
        *out_limit_kph = pending_speed_limit;
        speed_limit_event_pending = false;
        return true;
    }
    
    return false;
}

bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms) {
    if ((out == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    out->found = current_row.gap_found;
    out->width_mm = current_row.gap_width_mm;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms) {
    if ((out_tc_x10 == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    *out_tc_x10 = current_row.cabin_tc_x10;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms) {
    if ((out_tc_x10 == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    *out_tc_x10 = current_row.ambient_tc_x10;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms) {
    if ((out_pct == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    *out_pct = current_row.humid_pct;
    *out_ts_ms = current_row.ms;
    return true;
}

bool hal_read_voice_line(char* buf, uint16_t len) {
    if ((buf == NULL) || (len == 0U)) {
        return false;
    }
    
    if (!row_valid && !update_current_row()) {
        return false;
    }
    
    if (strlen(current_row.voice_cmd) > 0U) {
        strncpy(buf, current_row.voice_cmd, len - 1U);
        buf[len - 1U] = '\0';
        current_row.voice_cmd[0] = '\0';
        return true;
    }
    
    return false;
}

static void ensure_outputs_file_open(void) {
    if (!outputs_file_open) {
        outputs_file = fopen("outputs.csv", "w");
        if (outputs_file != NULL) {
            fprintf(outputs_file, "ms,brake,wiper_mode,alarm,limit_req,fan_stage,ac_on,blend,park_step\n");
            outputs_file_open = true;
        }
    }
}

void hal_set_brake_request(bool on) {
    ensure_outputs_file_open();
    if (outputs_file != NULL) {
        fprintf(outputs_file, "%u,%d,", hal_now_ms(), on ? 1 : 0);
    }
}

void hal_set_wiper_mode(uint8_t mode) {
    ensure_outputs_file_open();
    if (outputs_file != NULL) {
        fprintf(outputs_file, "%u,", mode);
    }
}

void hal_set_alarm(bool on) {
    ensure_outputs_file_open();
    if (outputs_file != NULL) {
        fprintf(outputs_file, "%d,", on ? 1 : 0);
    }
}

void hal_set_speed_limit_request(uint16_t kph) {
    ensure_outputs_file_open();
    if (outputs_file != NULL) {
        fprintf(outputs_file, "%u,", kph);
    }
}

void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct) {
    ensure_outputs_file_open();
    if (outputs_file != NULL) {
        fprintf(outputs_file, "%u,%d,%u,", fan_stage, ac_on ? 1 : 0, blend_pct);
    }
}

void hal_actuate_parking_prompt(uint8_t step_code) {
    ensure_outputs_file_open();
    if (outputs_file != NULL) {
        fprintf(outputs_file, "%u\n", step_code);
        fflush(outputs_file);
    }
}

void hal_mock_cleanup(void) {
    if (outputs_file != NULL) {
        fclose(outputs_file);
        outputs_file = NULL;
        outputs_file_open = false;
    }
}