#include "hal.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <windows.h>

extern uint32_t platform_get_time_ms(void);

static uint16_t sim_distance_mm = 2000U;
static uint8_t sim_rain_pct = 0U;
static uint16_t sim_speed_kph = 50U;
static uint16_t sim_speed_limit = 50U;
static bool sim_gap_found = false;
static uint16_t sim_gap_width = 5500U;
static int16_t sim_cabin_temp = 220;
static int16_t sim_ambient_temp = 250;
static uint8_t sim_humidity = 45U;
static int16_t sim_setpoint = 220;
static bool sim_driver_brake = false;
static bool sim_vehicle_ready = true;
static char sim_voice_buffer[64] = {0};
static bool sim_voice_available = false;

static bool last_brake_request = false;
static uint8_t last_wiper_mode = 0U;
static bool last_alarm = false;
static uint16_t last_limit_request = 0U;
static uint8_t last_fan_stage = 0U;
static bool last_ac_on = false;
static uint8_t last_blend_pct = 50U;
static uint8_t last_park_step = 0U;

static bool ui_initialized = false;

static void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

static void clear_screen(void) {
    system("cls");
}

static void draw_dashboard(void) {
    if (!ui_initialized) {
        clear_screen();
        ui_initialized = true;
    }
    
    gotoxy(0, 0);
    printf("======================== CAR POC DASHBOARD ========================\n");
    printf("                                                                    \n");
    printf(" SENSORS:                        OUTPUTS:                          \n");
    printf(" Speed: %3d kph                  Brake: %s                        \n", 
           sim_speed_kph, last_brake_request ? "ON " : "OFF");
    printf(" Distance: %4d mm               Wipers: ", sim_distance_mm);
    
    switch(last_wiper_mode) {
        case 0: printf("OFF "); break;
        case 1: printf("INT "); break;
        case 2: printf("LOW "); break;
        case 3: printf("HIGH"); break;
        default: printf("??? "); break;
    }
    printf("                        \n");
    
    printf(" Rain: %3d%%                     Alarm: %s                         \n", 
           sim_rain_pct, last_alarm ? "ON " : "OFF");
    printf(" Speed Limit: %3d kph            Limit Req: %3d kph               \n", 
           sim_speed_limit, last_limit_request);
    printf(" Gap: %s %4dmm              Fan: Stage %d                     \n", 
           sim_gap_found ? "YES" : "NO ", sim_gap_width, last_fan_stage);
    printf(" Cabin: %2d.%dC                  AC: %s                           \n", 
           sim_cabin_temp/10, sim_cabin_temp%10, last_ac_on ? "ON " : "OFF");
    printf(" Ambient: %2d.%dC                Blend: %3d%%                     \n", 
           sim_ambient_temp/10, sim_ambient_temp%10, last_blend_pct);
    printf(" Humidity: %3d%%                 Park Step: %d                     \n", 
           sim_humidity, last_park_step);
    printf(" Setpoint: %2d.%dC               Driver Brake: %s                \n", 
           sim_setpoint/10, sim_setpoint%10, sim_driver_brake ? "ON " : "OFF");
    printf("                                                                    \n");
    printf("====================================================================\n");
    printf(" CONTROLS:                                                          \n");
    printf(" [↑/↓] Speed   [←/→] Distance   [R/F] Rain   [1-4] Speed Limits    \n");
    printf(" [P] Toggle Gap   [B] Brake   [T/G] Temp   [Q] Quit                \n");
    printf("====================================================================\n");
    
    if (strlen(sim_voice_buffer) > 0) {
        printf(" Voice: %s                                                      \n", sim_voice_buffer);
    } else {
        printf("                                                                    \n");
    }
}

static void handle_keyboard_input(void) {
    if (_kbhit()) {
        char key = _getch();
        switch(key) {
            case 'w': case 'W': 
                if (sim_speed_kph < 200U) sim_speed_kph += 5U;
                break;
            case 's': case 'S':
                if (sim_speed_kph > 0U) sim_speed_kph -= 5U;
                break;
            case 'a': case 'A':
                if (sim_distance_mm > 100U) sim_distance_mm -= 100U;
                break;
            case 'd': case 'D':
                if (sim_distance_mm < 5000U) sim_distance_mm += 100U;
                break;
            case 'r': case 'R':
                if (sim_rain_pct < 100U) sim_rain_pct += 5U;
                break;
            case 'f': case 'F':
                if (sim_rain_pct > 0U) sim_rain_pct -= 5U;
                break;
            case '1':
                sim_speed_limit = 30U;
                break;
            case '2':
                sim_speed_limit = 50U;
                break;
            case '3':
                sim_speed_limit = 80U;
                break;
            case '4':
                sim_speed_limit = 100U;
                break;
            case 'p': case 'P':
                sim_gap_found = !sim_gap_found;
                break;
            case 'b': case 'B':
                sim_driver_brake = !sim_driver_brake;
                break;
            case 't': case 'T':
                sim_setpoint += 5;
                break;
            case 'g': case 'G':
                sim_setpoint -= 5;
                break;
            case 'q': case 'Q':
                exit(0);
                break;
        }
    }
}

bool hal_interactive_init(void) {
    clear_screen();
    printf("Initializing Car PoC Interactive Dashboard...\n");
    printf("Please wait...\n");
    Sleep(1000);
    return true;
}

void hal_interactive_step(void) {
    handle_keyboard_input();
    draw_dashboard();
    Sleep(50);
}

void hal_interactive_cleanup(void) {
    printf("\nShutting down Car PoC Dashboard...\n");
}

bool hal_get_vehicle_ready(void) {
    return sim_vehicle_ready;
}

bool hal_driver_brake_pressed(void) {
    return sim_driver_brake;
}

uint32_t hal_now_ms(void) {
    return platform_get_time_ms();
}

bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms) {
    if ((out_mm == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    *out_mm = sim_distance_mm;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms) {
    if ((out_pct == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    *out_pct = sim_rain_pct;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms) {
    if ((out_kph == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    *out_kph = sim_speed_kph;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph) {
    static uint16_t last_reported_limit = 0U;
    
    if (out_limit_kph == NULL) {
        return false;
    }
    
    if (sim_speed_limit != last_reported_limit) {
        *out_limit_kph = sim_speed_limit;
        last_reported_limit = sim_speed_limit;
        return true;
    }
    
    return false;
}

bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms) {
    if ((out == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    out->found = sim_gap_found;
    out->width_mm = sim_gap_width;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms) {
    if ((out_tc_x10 == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    *out_tc_x10 = sim_cabin_temp;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms) {
    if ((out_tc_x10 == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    *out_tc_x10 = sim_ambient_temp;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms) {
    if ((out_pct == NULL) || (out_ts_ms == NULL)) {
        return false;
    }
    
    *out_pct = sim_humidity;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_voice_line(char* buf, uint16_t len) {
    if ((buf == NULL) || (len == 0U)) {
        return false;
    }
    
    if (sim_voice_available) {
        strncpy(buf, sim_voice_buffer, len - 1U);
        buf[len - 1U] = '\0';
        sim_voice_available = false;
        sim_voice_buffer[0] = '\0';
        return true;
    }
    
    return false;
}

void hal_set_brake_request(bool on) {
    last_brake_request = on;
}

void hal_set_wiper_mode(uint8_t mode) {
    last_wiper_mode = mode;
}

void hal_set_alarm(bool on) {
    last_alarm = on;
}

void hal_set_speed_limit_request(uint16_t kph) {
    last_limit_request = kph;
}

void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct) {
    last_fan_stage = fan_stage;
    last_ac_on = ac_on;
    last_blend_pct = blend_pct;
}

void hal_actuate_parking_prompt(uint8_t step_code) {
    last_park_step = step_code;
}