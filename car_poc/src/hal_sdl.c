#include "hal.h"
#include "platform.h"

#if !HEADLESS_BUILD
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

extern uint32_t platform_get_time_ms(void);
extern bool platform_sdl_pump_events(void);

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

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

bool hal_sdl_init(void) {
    window = SDL_CreateWindow("Car PoC Dashboard",
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             800, 600,
                             SDL_WINDOW_SHOWN);
    
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

void hal_sdl_cleanup(void) {
    if (renderer != NULL) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    
    if (window != NULL) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
}

static void handle_keyboard_input(void) {
    const uint8_t* keystate = SDL_GetKeyboardState(NULL);
    
    if (keystate[SDL_SCANCODE_UP] && sim_speed_kph < 200U) {
        sim_speed_kph++;
    }
    if (keystate[SDL_SCANCODE_DOWN] && sim_speed_kph > 0U) {
        sim_speed_kph--;
    }
    if (keystate[SDL_SCANCODE_LEFT] && sim_distance_mm > 50U) {
        sim_distance_mm -= 50U;
    }
    if (keystate[SDL_SCANCODE_RIGHT] && sim_distance_mm < 5000U) {
        sim_distance_mm += 50U;
    }
    if (keystate[SDL_SCANCODE_R] && sim_rain_pct < 100U) {
        sim_rain_pct++;
    }
    if (keystate[SDL_SCANCODE_F] && sim_rain_pct > 0U) {
        sim_rain_pct--;
    }
    if (keystate[SDL_SCANCODE_1]) {
        sim_speed_limit = 30U;
    }
    if (keystate[SDL_SCANCODE_2]) {
        sim_speed_limit = 50U;
    }
    if (keystate[SDL_SCANCODE_3]) {
        sim_speed_limit = 80U;
    }
    if (keystate[SDL_SCANCODE_4]) {
        sim_speed_limit = 100U;
    }
    if (keystate[SDL_SCANCODE_P]) {
        sim_gap_found = !sim_gap_found;
    }
    if (keystate[SDL_SCANCODE_B]) {
        sim_driver_brake = !sim_driver_brake;
    }
}

static void render_hud(void) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    SDL_Rect speed_rect = {50, 50, 100, 60};
    SDL_RenderDrawRect(renderer, &speed_rect);
    
    SDL_Rect distance_rect = {200, 50, 200, 20};
    if (sim_distance_mm < 1220U) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    }
    SDL_RenderFillRect(renderer, &distance_rect);
    
    if (last_brake_request) {
        SDL_Rect brake_rect = {450, 50, 80, 30};
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderFillRect(renderer, &brake_rect);
    }
    
    if (last_alarm) {
        SDL_Rect alarm_rect = {450, 100, 80, 30};
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &alarm_rect);
    }
    
    SDL_RenderPresent(renderer);
}

void hal_sdl_step(void) {
    if (!platform_sdl_pump_events()) {
        return;
    }
    
    handle_keyboard_input();
    render_hud();
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

#endif /* !HEADLESS_BUILD */