#include "hal.h"
#include "platform.h"

#if !HEADLESS_BUILD
#include <SDL.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ---- Platform glue ---- */
extern uint32_t platform_get_time_ms(void);
extern bool platform_sdl_pump_events(void);

/* ---- Window/renderer ---- */
static SDL_Window*   window   = NULL;
static SDL_Renderer* renderer = NULL;

/* ---- Simulation state (inputs) ---- */
static uint16_t sim_speed_kph     = 0U;     /* user-adjustable via ↑/↓ */
static uint16_t sim_rpm           = 900U;   /* idle; auto-changes with speed */
static uint8_t  sim_rain_pct      = 0U;     /* auto rain 0..100 */
static uint16_t sim_speed_limit   = 50U;
static bool     sim_gap_found     = false;
static uint16_t sim_gap_width     = 5500U;

/* temps (x10 fixed-point) */
static int16_t  sim_cabin_temp    = 220;    /* auto climate will drive this */
static int16_t  sim_ambient_temp  = 250;    /* user changes with H/L */
static uint8_t  sim_humidity      = 45U;
static int16_t  sim_setpoint      = 220;    /* your climate module target */

/* vehicle state inputs */
static bool     sim_vehicle_ready = true;
static bool     sim_driver_brake  = false;

/* voice */
static char     sim_voice_buffer[64] = {0};
static bool     sim_voice_available  = false;

/* ---- Actuator sinks (outputs from app) ---- */
static bool     last_brake_request  = false;
static uint8_t  last_wiper_mode     = 0U;    /* 0=OFF,1=INT,2=LOW,3=HIGH */
static bool     last_alarm          = false;
static uint16_t last_limit_request  = 0U;
static uint8_t  last_fan_stage      = 0U;
static bool     last_ac_on          = false;
static uint8_t  last_blend_pct      = 50U;
static uint8_t  last_park_step      = 0U;

/* ---- Auto objects & distance sim (no user control) ---- */
static uint16_t sim_distance_mm     = 4000U;  /* decreasing when an object spawns */
static uint8_t  obj_state           = 0U;     /* 0=idle,1=spawned,2=braking */
static uint32_t obj_t_next_spawn_ms = 0U;     /* when to spawn next object */

/* ---- Pedestrian rendering state ---- */
static bool   ped_active = false;
static float  ped_x_px   = 0.0f;   /* in windshield coords */
static int    ped_dir    = 1;      /* +1 right, -1 left */
static float  ped_speed  = 70.0f;  /* px/sec lateral */

/* ---- Rain simulation (no user control) ---- */
typedef enum { RAIN_IDLE=0, RAIN_RISE, RAIN_PEAK, RAIN_DECAY } rain_state_e;
static rain_state_e rain_state      = RAIN_IDLE;
static uint32_t     rain_t_next_ms  = 0U;     /* when to (re)trigger */
static uint32_t     rain_last_ms    = 0U;

/* ---- Windshield geometry (top half) ---- */
static int ws_x=0, ws_y=0, ws_w=0, ws_h=0;

/* ---- Buildings (outside world) ---- */
#define MAX_BUILDINGS 12
typedef struct { float x; int w; int h; SDL_Color col; uint32_t seed; } building_t;
static building_t buildings[MAX_BUILDINGS];

/* ---- Misc helpers ---- */
static uint32_t now_ms(void){ return platform_get_time_ms(); }
static float    deg2rad(float d){ return (float)M_PI*d/180.0f; }
static int      clampi(int v,int lo,int hi){ return (v<lo)?lo:((v>hi)?hi:v); }
static uint32_t lcg(uint32_t s){ return 1664525u*s + 1013904223u; }

/* ---- Theme ---- */
static const SDL_Color COL_BG     = {18,18,22,255};
static const SDL_Color COL_INT    = {32,34,40,255};   /* interior/dash */
static const SDL_Color COL_GLASS  = {24,24,28,255};   /* windshield base */
static const SDL_Color COL_TEXT   = {230,230,235,255};
static const SDL_Color COL_ACC    = { 90,145,250,255};
static const SDL_Color COL_OK     = { 80,200,120,255};
static const SDL_Color COL_WARN   = {255,200, 50,255};
static const SDL_Color COL_ERR    = {235, 64, 52,255};

/* ===========================================================
 *  INPUT HANDLING
 * =========================================================== */
static void handle_keyboard_input(void){
    const uint8_t* ks = SDL_GetKeyboardState(NULL);

    /* Desired speed */
    if (ks[SDL_SCANCODE_UP]   && sim_speed_kph < 200U) sim_speed_kph++;
    if (ks[SDL_SCANCODE_DOWN] && sim_speed_kph >   0U) sim_speed_kph--;

    /* Outside temperature (user control) */
    if (ks[SDL_SCANCODE_H] && sim_ambient_temp < 500)  sim_ambient_temp += 1;  /* +0.1 C */
    if (ks[SDL_SCANCODE_L] && sim_ambient_temp > -400) sim_ambient_temp -= 1;  /* -0.1 C */

    /* Voice assist trigger */
    if (ks[SDL_SCANCODE_M]) {
        (void)snprintf(sim_voice_buffer, sizeof(sim_voice_buffer), "hey car open sunroof");
        sim_voice_available = true;
    }

    /* Speed limit controls */
    if (ks[SDL_SCANCODE_Q] && sim_speed_limit < 200U) sim_speed_limit += 5U; /* increase limit */
    if (ks[SDL_SCANCODE_P] && sim_speed_limit > 30U)  sim_speed_limit -= 5U; /* decrease limit */

    /* Manual object spawn with R key */
    static bool r_pressed = false;
    if (ks[SDL_SCANCODE_R] && !r_pressed) {
        if (obj_state == 0U) { /* only spawn if no object active */
            obj_state = 1U;
            sim_distance_mm = 4000U + (uint16_t)((now_ms() % 2000u));
            /* spawn pedestrian at bottom RIGHT of windshield */
            ped_active = true;
            ped_x_px = (float)(ws_x + ws_w - 30); /* right edge */
            ped_dir = -1; /* moving left */
        }
        r_pressed = true;
    } else if (!ks[SDL_SCANCODE_R]) {
        r_pressed = false;
    }
}

/* ===========================================================
 *  SIMPLE 5x7 FONT (digits + a few letters + dot/space)
 * =========================================================== */
typedef struct { char ch; uint8_t col5[5]; } glyph_t;

static const glyph_t FONT[] = {
    /* digits */
    {'0',{0x3E,0x51,0x49,0x45,0x3E}}, {'1',{0x00,0x42,0x7F,0x40,0x00}},
    {'2',{0x42,0x61,0x51,0x49,0x46}}, {'3',{0x21,0x41,0x45,0x4B,0x31}},
    {'4',{0x18,0x14,0x12,0x7F,0x10}}, {'5',{0x27,0x45,0x45,0x45,0x39}},
    {'6',{0x3C,0x4A,0x49,0x49,0x30}}, {'7',{0x01,0x71,0x09,0x05,0x03}},
    {'8',{0x36,0x49,0x49,0x49,0x36}}, {'9',{0x06,0x49,0x49,0x29,0x1E}},
    /* letters used in labels */
    {'A',{0x7E,0x09,0x09,0x09,0x7E}}, {'B',{0x7F,0x49,0x49,0x49,0x36}},
    {'C',{0x3E,0x41,0x41,0x41,0x22}}, {'D',{0x7F,0x41,0x41,0x22,0x1C}},
    {'E',{0x7F,0x49,0x49,0x49,0x41}}, {'F',{0x7F,0x09,0x09,0x09,0x01}},
    {'G',{0x3E,0x41,0x49,0x49,0x7A}}, {'H',{0x7F,0x08,0x08,0x08,0x7F}},
    {'I',{0x00,0x41,0x7F,0x41,0x00}}, {'K',{0x7F,0x08,0x14,0x22,0x41}},
    {'L',{0x7F,0x40,0x40,0x40,0x40}}, {'M',{0x7F,0x02,0x04,0x02,0x7F}},
    {'N',{0x7F,0x04,0x08,0x10,0x7F}}, {'O',{0x3E,0x41,0x41,0x41,0x3E}},
    {'P',{0x7F,0x09,0x09,0x09,0x06}}, {'R',{0x7F,0x09,0x19,0x29,0x46}},
    {'S',{0x26,0x49,0x49,0x49,0x32}}, {'T',{0x01,0x01,0x7F,0x01,0x01}},
    {'U',{0x3F,0x40,0x40,0x40,0x3F}}, {'V',{0x1F,0x20,0x40,0x20,0x1F}},
    {'W',{0x7F,0x20,0x18,0x20,0x7F}}, {'Y',{0x07,0x08,0x70,0x08,0x07}},
    {' ',{0x00,0x00,0x00,0x00,0x00}}, {'.',{0x00,0x60,0x60,0x00,0x00}},
};

static const uint8_t* glyph_for(char c){
    size_t n = sizeof(FONT)/sizeof(FONT[0]);
    for(size_t i=0;i<n;i++){ if(FONT[i].ch==c) return FONT[i].col5; }
    return NULL;
}
static void draw_glyph(int x,int y,int s,const uint8_t col5[5], SDL_Color c){
    SDL_SetRenderDrawColor(renderer,c.r,c.g,c.b,255);
    for(int col=0; col<5; ++col){
        uint8_t bits = col5[col];
        for(int row=0; row<7; ++row){
            if(bits & (1u<<row)){ /* Fixed: removed (6-row) to fix upside-down text */
                SDL_Rect px = { x + col*s, y + row*s, s, s };
                SDL_RenderFillRect(renderer,&px);
            }
        }
    }
}
static void draw_text(int x,int y,int s,const char* txt, SDL_Color c){
    int pen=x;
    for(const char* p=txt; *p; ++p){
        const uint8_t* g = glyph_for((*p>='a'&&*p<='z')? (*p-32):*p);
        if(g){ draw_glyph(pen,y,s,g,c); pen += 6*s; }
        else  pen += 3*s;
    }
}
static void draw_number(int x,int y,int s,int val, SDL_Color c){
    char buf[16]; (void)snprintf(buf,sizeof(buf),"%d", val);
    draw_text(x,y,s,buf,c);
}

/* ===========================================================
 *  AUTO RAIN
 * =========================================================== */
static void rain_update(void){
    const uint32_t t = now_ms();
    if (t < rain_t_next_ms) return;

    switch (rain_state){
        case RAIN_IDLE:
            rain_state = RAIN_RISE; rain_last_ms = t; break;
        case RAIN_RISE:
            if (sim_rain_pct < 100U){
                if ((t - rain_last_ms) > 30U){ sim_rain_pct = (uint8_t)clampi(sim_rain_pct+5,0,100); rain_last_ms = t; }
            } else { rain_state = RAIN_PEAK; rain_t_next_ms = t + 1500U; }
            break;
        case RAIN_PEAK:
            if (t >= rain_t_next_ms){ rain_state = RAIN_DECAY; rain_last_ms = t; }
            break;
        case RAIN_DECAY:
            if (sim_rain_pct > 0U){
                if ((t - rain_last_ms) > 60U){ sim_rain_pct--; rain_last_ms = t; }
            } else {
                rain_state = RAIN_IDLE;
                static uint32_t seed=1234567u; seed = lcg(seed);
                rain_t_next_ms = t + 7000u + (seed%8000u);
            }
            break;
        default: rain_state = RAIN_IDLE; break;
    }
}
static void draw_rain(void){
    if (sim_rain_pct == 0U) return;
    int drops = 80 + (int)(sim_rain_pct * 2); /* 80..280 */
    int len   = 8  + (int)(sim_rain_pct / 6); /* 8..24  */
    SDL_SetRenderDrawColor(renderer, 235,235,235,255);
    uint32_t seed = (now_ms()*1103515245u) ^ 0xA5A5u;
    for(int i=0;i<drops;i++){
        seed = lcg(seed);
        int rx = ws_x + 6 + (int)(seed % (uint32_t)(ws_w-12));
        seed = lcg(seed);
        int ry = ws_y + 6 + (int)(seed % (uint32_t)(ws_h-12));
        SDL_RenderDrawLine(renderer, rx, ry, rx-3, ry+len);
    }
}

/* ===========================================================
 *  OBJECT / DISTANCE / PEDESTRIAN
 * =========================================================== */
#define AB_THRESHOLD_MM (1220U)  /* 4 feet auto brake threshold */
#define GAP_THRESHOLD_MM (1830U)  /* 6 feet gap alarm threshold */

static void draw_pedestrian(void){
    if (!ped_active) return;

    uint16_t d = sim_distance_mm;
    if (d < 1000U) d = 1000U; if (d > 6000U) d = 6000U;

    float t = (6000.0f - (float)d) / 5000.0f; /* 0..1 */
    if (t < 0.f) t = 0.f; if (t > 1.f) t = 1.f;

    int H = 40 + (int)(t * 80.0f);
    int W = (int)(H * 0.45f);
    int x = (int)ped_x_px;
    int y = ws_y + ws_h - H - 4; /* bottom of windshield for ground level */

    SDL_Color body = (SDL_Color){ 245, 245, 250, 255 }; /* white pedestrian */
    SDL_SetRenderDrawColor(renderer, body.r, body.g, body.b, 255);

    /* head */
    int head = H/5; SDL_Rect headR = { x - head/2, y, head, head }; SDL_RenderFillRect(renderer, &headR);
    /* torso */
    int torsoH = (int)(H*0.45f); SDL_Rect torso = { x - W/6, y + head, W/3, torsoH }; SDL_RenderFillRect(renderer,&torso);
    /* arms */
    SDL_Rect armL = { x - W/2, y + head + 8, W/2, 6 }; SDL_Rect armR = { x, y + head + 8, W/2, 6 };
    SDL_RenderFillRect(renderer,&armL); SDL_RenderFillRect(renderer,&armR);
    /* legs */
    int legY = y + head + torsoH;
    SDL_Rect legL = { x - W/4, legY, 6, H - (head + torsoH) };
    SDL_Rect legR = { x + W/6, legY, 6, H - (head + torsoH) };
    SDL_RenderFillRect(renderer,&legL); SDL_RenderFillRect(renderer,&legR);
}

static void pedestrian_update(float dt){
    if (!ped_active) return;
    ped_x_px += ped_dir * ped_speed * dt;
    if (ped_x_px < (float)(ws_x + 20)) { ped_x_px = (float)(ws_x + 20); ped_dir = +1; }
    if (ped_x_px > (float)(ws_x + ws_w - 20)) { ped_x_px = (float)(ws_x + ws_w - 20); ped_dir = -1; }
}

static void distance_update_and_auto_brake(void){
    const uint32_t t = now_ms();
    static uint32_t seed = 987654u;

    if (obj_state == 0U){
        if (t >= obj_t_next_spawn_ms && sim_speed_kph > 10U){
            obj_state = 1U;
            sim_distance_mm = 4000U + (uint16_t)((lcg(seed+=33u)%2000u)); /* 4–6 m */

            /* spawn pedestrian from RIGHT side only */
            ped_active = true;
            ped_x_px = (float)(ws_x + ws_w - 30); /* always from right */
            ped_dir = -1; /* always moving left */
        }
    }

    if (obj_state >= 1U){
        float approach = (float)sim_speed_kph * 2.8f; if (approach < 1.f) approach = 1.f;
        if (sim_distance_mm > (uint16_t)approach) sim_distance_mm -= (uint16_t)approach;
        else sim_distance_mm = 0U;

        /* Check thresholds: 6 feet for gap alarm, 4 feet for auto brake */
        if (sim_distance_mm <= GAP_THRESHOLD_MM && sim_distance_mm > AB_THRESHOLD_MM){
            sim_gap_found = true; /* gap alarm at 6 feet */
        }
        if (sim_distance_mm <= AB_THRESHOLD_MM){
            obj_state = 2U;
            last_brake_request = true; /* auto brake at 4 feet */
            sim_gap_found = true;
        }
    }

    if (obj_state == 2U){
        if (sim_speed_kph > 0U){
            sim_speed_kph = (sim_speed_kph > 5U) ? (sim_speed_kph - 5U) : 0U;
        } else {
            static uint32_t stop_ms = 0U;
            if (stop_ms == 0U) stop_ms = t + 1200U;
            if (t >= stop_ms){
                obj_state = 0U;
                ped_active = false;
                last_brake_request = false;
                sim_gap_found = false; /* reset gap alarm */
                sim_distance_mm = 5000U;
                stop_ms = 0U;
                seed = lcg(seed);
                obj_t_next_spawn_ms = t + 4000u + (seed%5000u);
            }
        }
    }

    /* RPM follows speed */
    uint16_t target_rpm = (uint16_t)(800U + (sim_speed_kph * 35U));
    if (sim_rpm < target_rpm) sim_rpm += 50U;
    else if (sim_rpm > target_rpm && sim_rpm > 900U) sim_rpm -= 50U;

    /* Speed limit alarm */
    if (sim_speed_kph > sim_speed_limit) {
        last_alarm = true;
    } else {
        last_alarm = false;
    }
}

/* ===========================================================
 *  BUILDINGS (colors + windows)
 * =========================================================== */
static SDL_Color palette[8] = {
    {230,  90, 100,255}, /* light red */
    {135, 206, 235,255}, /* sky blue */
    {255, 182, 193,255}, /* pink */
    {176, 196, 222,255}, /* light steel blue */
    {152, 251, 152,255}, /* pale green */
    {238, 221, 130,255}, /* light khaki */
    {221, 160, 221,255}, /* plum */
    {240, 128, 128,255}, /* light coral */
};

static void buildings_init(int x,int y,int w,int h){
    ws_x=x; ws_y=y; ws_w=w; ws_h=h;
    int gx = ws_x + 16;
    uint32_t seed = 321321u;
    int last_idx = -1;
    for(int i=0;i<MAX_BUILDINGS;i++){
        int bw = 50 + (int)(lcg(seed+=77u)%70u);
        int bh = 80 + (int)(lcg(seed+=91u)% (ws_h-100));
        int idx = (int)(lcg(seed+=13u)%8u);
        if (idx == last_idx) idx = (idx+1) % 8;
        last_idx = idx;
        buildings[i].x = (float)gx;
        buildings[i].w = bw;
        buildings[i].h = bh;
        buildings[i].col = palette[idx];
        buildings[i].seed = lcg(seed);
        gx += bw + 40;
        if (gx > ws_x + ws_w - 80) gx = ws_x + 16;
    }
}

static void draw_building_windows(SDL_Rect r, uint32_t* seed){
    SDL_Color win_on  = {250, 245, 180, 255};
    SDL_Color win_off = { 70,  70,  80, 255};
    int cols = r.w / 16; if (cols < 2) cols = 2;
    int rows = r.h / 20; if (rows < 2) rows = 2;

    for(int cy=0; cy<rows; ++cy){
        for(int cx=0; cx<cols; ++cx){
            *seed = lcg(*seed);
            bool on = ((*seed) & 3u) != 0u; /* most windows lit */
            SDL_Color c = on ? win_on : win_off;
            SDL_SetRenderDrawColor(renderer, c.r,c.g,c.b,255);
            SDL_Rect w = {
                r.x + 6 + cx*16,
                r.y + 6 + cy*20,
                10, 12
            };
            if (w.x + w.w <= r.x + r.w - 4 && w.y + w.h <= r.y + r.h - 4)
                SDL_RenderFillRect(renderer,&w);
        }
    }
}

static void buildings_update_and_draw(float px_per_tick){
    SDL_Rect ws = { ws_x, ws_y, ws_w, ws_h };
    SDL_SetRenderDrawColor(renderer, COL_GLASS.r,COL_GLASS.g,COL_GLASS.b,255);
    SDL_RenderFillRect(renderer,&ws);

    for(int i=0;i<MAX_BUILDINGS;i++){
        if (sim_speed_kph > 0U) buildings[i].x -= px_per_tick;
        if (buildings[i].x + buildings[i].w < ws_x){
            /* wrap to right with a color different from previous */
            SDL_Color prev = buildings[(i-1+MAX_BUILDINGS)%MAX_BUILDINGS].col;
            int pick = (int)((now_ms()/7)%8);
            for(int tries=0; tries<8; ++tries){
                if (palette[pick].r!=prev.r || palette[pick].g!=prev.g || palette[pick].b!=prev.b) break;
                pick = (pick+1)%8;
            }
            buildings[i].col = palette[pick];
            buildings[i].x = (float)(ws_x + ws_w + 40);
            buildings[i].w = 50 + (int)((now_ms()/3)%70);
            buildings[i].h = 80 + (int)((now_ms()/4)% (ws_h-100));
            buildings[i].seed = lcg(buildings[i].seed);
        }
        /* draw */
        SDL_Rect r = { (int)buildings[i].x, ws_y + ws_h - buildings[i].h, buildings[i].w, buildings[i].h };
        SDL_SetRenderDrawColor(renderer, buildings[i].col.r,buildings[i].col.g,buildings[i].col.b,255);
        SDL_RenderFillRect(renderer,&r);
        draw_building_windows(r, &buildings[i].seed);
    }
}

/* ===========================================================
 *  WIPERS (center-mounted, 20° ↔ 120°)
 * =========================================================== */
static void draw_wipers_and_wiped_area(void){
    /* base in middle-top of windshield (flipped upside down) */
    int base_x = ws_x + ws_w/2;
    int base_y = ws_y + 4; /* moved to top */

    /* sweep frequency by mode (off uses parked position) */
    float hz = 0.0f;
    if (last_wiper_mode==1U) hz = 0.6f;
    else if (last_wiper_mode==2U) hz = 1.2f;
    else if (last_wiper_mode==3U) hz = 2.0f;

    float angleDeg;
    if (hz <= 0.0f){
        angleDeg = 160.0f; /* parked position (flipped) */
    } else {
        /* map sine to [60, 160] (flipped range) */
        float s = sinf(2.0f*(float)M_PI*hz*((float)now_ms()/1000.0f)); /* -1..1 */
        angleDeg = 110.0f + 50.0f * s; /* 60..160 */
    }

    int arm = (ws_w*2)/3; if (arm > 320) arm = 320; /* increased wiper length */

    /* Left blade (leaning left from base), Right blade mirrored - flipped orientation */
    float aL = deg2rad(angleDeg); /* flipped */
    float aR = deg2rad(180.0f - angleDeg); /* flipped */

    /* wipe “clean” glass under the blade tip by overdrawing glass color wide */
    SDL_SetRenderDrawColor(renderer, COL_GLASS.r,COL_GLASS.g,COL_GLASS.b,255);
    for(int t=0;t<15;t++){ /* increased wipe coverage */
        int lx = base_x - 30 + (int)(cosf(aL)*(arm-40-t));
        int ly = base_y + (int)(sinf(aL)*(arm-40-t));
        int rx = base_x + 30 + (int)(cosf(aR)*(arm-40-t));
        int ry = base_y + (int)(sinf(aR)*(arm-40-t));
        SDL_RenderDrawLine(renderer, base_x-30, base_y, lx, ly);
        SDL_RenderDrawLine(renderer, base_x+30, base_y, rx, ry);
    }

    /* draw blades on top (thicker) */
    SDL_SetRenderDrawColor(renderer, 40,40,45,255);
    for(int w=0; w<5; ++w){ /* thicker blades */
        SDL_RenderDrawLine(renderer, base_x-30, base_y+w, base_x-30 + (int)(cosf(aL)*(arm)), base_y+w + (int)(sinf(aL)*(arm)));
        SDL_RenderDrawLine(renderer, base_x+30, base_y+w, base_x+30 + (int)(cosf(aR)*(arm)), base_y+w + (int)(sinf(aR)*(arm)));
    }
}

/* ===========================================================
 *  USER CAR (bottom left of windshield)
 * =========================================================== */
static void draw_user_car(void){
    if (ws_w == 0 || ws_h == 0) return; /* windshield not initialized */
    
    /* Position: bottom-left of windshield with slight offset */
    int car_x = ws_x + 30;
    int car_y = ws_y + ws_h - 60;
    int car_w = 80;  /* car width */
    int car_h = 35;  /* car height */
    
    /* Car body (main rectangle) - silver/gray color */
    SDL_Color car_body = {180, 180, 185, 255};
    SDL_SetRenderDrawColor(renderer, car_body.r, car_body.g, car_body.b, 255);
    SDL_Rect body = {car_x, car_y, car_w, car_h};
    SDL_RenderFillRect(renderer, &body);
    
    /* Car outline */
    SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
    SDL_RenderDrawRect(renderer, &body);
    
    /* Windshield (front window) */
    SDL_Color windshield = {120, 150, 200, 200};
    SDL_SetRenderDrawColor(renderer, windshield.r, windshield.g, windshield.b, 255);
    SDL_Rect car_window = {car_x + car_w - 25, car_y + 5, 20, car_h - 10};
    SDL_RenderFillRect(renderer, &car_window);
    
    /* Front bumper (pointing right to show forward motion) */
    SDL_SetRenderDrawColor(renderer, car_body.r - 20, car_body.g - 20, car_body.b - 20, 255);
    SDL_Rect bumper = {car_x + car_w, car_y + 8, 8, car_h - 16};
    SDL_RenderFillRect(renderer, &bumper);
    
    /* Wheels (4 circles) */
    SDL_Color wheel = {50, 50, 60, 255};
    SDL_SetRenderDrawColor(renderer, wheel.r, wheel.g, wheel.b, 255);
    /* Front wheels */
    SDL_Rect fw1 = {car_x + car_w - 15, car_y - 3, 8, 8};
    SDL_Rect fw2 = {car_x + car_w - 15, car_y + car_h - 5, 8, 8};
    /* Rear wheels */
    SDL_Rect rw1 = {car_x + 10, car_y - 3, 8, 8};
    SDL_Rect rw2 = {car_x + 10, car_y + car_h - 5, 8, 8};
    
    SDL_RenderFillRect(renderer, &fw1);
    SDL_RenderFillRect(renderer, &fw2);
    SDL_RenderFillRect(renderer, &rw1);
    SDL_RenderFillRect(renderer, &rw2);
    
    /* Motion lines when moving (speed-based animation) */
    if (sim_speed_kph > 5U) {
        SDL_SetRenderDrawColor(renderer, 200, 200, 220, 180);
        int motion_lines = (int)(sim_speed_kph / 10); /* more lines = higher speed */
        if (motion_lines > 8) motion_lines = 8;
        
        for(int i = 0; i < motion_lines; i++){
            int line_x = car_x - 15 - (i * 8);
            if (line_x > ws_x + 5) {
                int line_y1 = car_y + 8 + (i * 2);
                int line_y2 = car_y + car_h - 8 - (i * 2);
                SDL_RenderDrawLine(renderer, line_x, line_y1, line_x - 6, line_y1);
                SDL_RenderDrawLine(renderer, line_x, line_y2, line_x - 6, line_y2);
            }
        }
    }
    
    /* Headlights (small white rectangles at front) */
    SDL_SetRenderDrawColor(renderer, 255, 255, 220, 255);
    SDL_Rect light1 = {car_x + car_w + 2, car_y + 6, 3, 6};
    SDL_Rect light2 = {car_x + car_w + 2, car_y + car_h - 12, 3, 6};
    SDL_RenderFillRect(renderer, &light1);
    SDL_RenderFillRect(renderer, &light2);
}

/* ===========================================================
 *  GAUGES (small, upright numbers)
 * =========================================================== */
static void draw_gauge_int(int x,int y,int r,int minV,int maxV,int major,int minor,int value,
                           SDL_Color tick, SDL_Color needle, bool rpmMode)
{
    /* arc line */
    SDL_SetRenderDrawColor(renderer, tick.r,tick.g,tick.b,255);
    for(int i=-r;i<=r;i+=2) SDL_RenderDrawPoint(renderer, x+i, y);

    /* ticks + labels */
    int labelScale = 1; /* smaller digits */
    for(int v=minV; v<=maxV; v+=minor){
        float t = (float)(v-minV)/(float)(maxV-minV);
        float ang = -120.0f + t*240.0f;
        float rad = deg2rad(ang);
        int len = (v%major==0)? 12 : 6;
        int x1 = x + (int)(cosf(rad)*(r-4));
        int y1 = y + (int)(sinf(rad)*(r-4));
        int x2 = x + (int)(cosf(rad)*(r-4-len));
        int y2 = y + (int)(sinf(rad)*(r-4-len));
        SDL_SetRenderDrawColor(renderer, tick.r,tick.g,tick.b,255);
        SDL_RenderDrawLine(renderer,x1,y1,x2,y2);

        if (v%major==0){
            int nx = x + (int)(cosf(rad)*(r-26)) - 8;
            int ny = y + (int)(sinf(rad)*(r-26)) - 6;
            char buf[8];
            if (rpmMode){
                /* show 1.0 .. 8.0 (v is 1000..8000) */
                (void)snprintf(buf,sizeof(buf),"%d.%d", v/1000, 0);
            } else {
                (void)snprintf(buf,sizeof(buf),"%d", v);
            }
            draw_text(nx,ny,labelScale,buf,COL_TEXT);
        }
    }

    /* needle */
    value = clampi(value,minV,maxV);
    float tv = (float)(value-minV)/(float)(maxV-minV);
    float an = -120.0f + tv*240.0f;
    int nx = x + (int)(cosf(deg2rad(an))*(r-18));
    int ny = y + (int)(sinf(deg2rad(an))*(r-18));
    SDL_SetRenderDrawColor(renderer, needle.r,needle.g,needle.b,255);
    SDL_RenderDrawLine(renderer, x, y, nx, ny);
    SDL_Rect hub = { x-3,y-3,6,6 };
    SDL_SetRenderDrawColor(renderer, 230,230,230,255);
    SDL_RenderFillRect(renderer,&hub);
}

/* ===========================================================
 *  DASHBOARD (bottom half)
 * =========================================================== */
static void draw_dashboard(int x,int y,int w,int h){
    SDL_Rect dash = {x,y,w,h};
    SDL_SetRenderDrawColor(renderer, COL_INT.r,COL_INT.g,COL_INT.b,255);
    SDL_RenderFillRect(renderer,&dash);

    /* ---- Gauges (left, side-by-side) ---- */
    int leftPad = 180; /* increased padding */
    int centerY = y + h/2 + 10;
    int r = 90; /* slightly smaller radius */
    int gxSpeed = x + leftPad - 90; /* adjusted position to fit in dashboard */
    int gxRPM   = x + leftPad + 90; /* adjusted position */

    draw_gauge_int(gxSpeed, centerY, r, 0,200,20,10, (int)sim_speed_kph, (SDL_Color){210,210,215,255}, COL_ACC, false);
    draw_gauge_int(gxRPM,   centerY, r, 0,8000,1000,500, (int)sim_rpm, (SDL_Color){210,210,215,255}, COL_WARN, true);

    draw_text(gxSpeed-18, centerY+r-12, 1, "KPH", COL_TEXT);
    draw_text(gxRPM-10,   centerY+r-12, 1, "RPM", COL_TEXT);

    /* ---- Center sensors with labels ---- */
    int cx = x + w/2 - 120;
    int cy = y + 70;
    struct Lamp { const char* name; bool on; SDL_Color col; } lamps[4] = {
        {"ALARM",  last_alarm,        COL_WARN},
        {"BRAKE",  last_brake_request,COL_ERR },
        {"WIPERS", last_wiper_mode!=0U, COL_OK},
        {"GAP",    sim_gap_found,     COL_OK },
    };
    for(int i=0;i<4;i++){
        SDL_Color c = lamps[i].on ? lamps[i].col : (SDL_Color){60,60,60,255};
        SDL_Rect L = { cx, cy + i*50, 18, 18 };
        SDL_SetRenderDrawColor(renderer, c.r,c.g,c.b,255);
        SDL_RenderFillRect(renderer,&L);
        SDL_SetRenderDrawColor(renderer, 210,210,210,255);
        SDL_RenderDrawRect(renderer,&L);
        draw_text(L.x + 28, L.y + 4, 2, lamps[i].name, COL_TEXT);  /* label */
    }

    /* ---- Right temperatures (Inside / Outside °C) ---- */
    int rx = x + w - 210;
    int ry = y + 60;
    SDL_SetRenderDrawColor(renderer, 210,210,210,255);
    SDL_RenderDrawRect(renderer, &(SDL_Rect){rx-10, ry-20, 200, 120});

    draw_text(rx, ry-16, 2, "INSIDE", COL_TEXT);
    int insideC = (int)(sim_cabin_temp/10);
    draw_number(rx, ry+6, 3, insideC, COL_TEXT);
    draw_text(rx + 3*6*3 + 6, ry+12, 1, "C", COL_TEXT);

    int ry2 = ry + 58;
    draw_text(rx, ry2-16, 2, "OUTSIDE", COL_TEXT);
    int outsideC = (int)(sim_ambient_temp/10);
    draw_number(rx, ry2+6, 3, outsideC, COL_TEXT);
    draw_text(rx + 3*6*3 + 6, ry2+12, 1, "C", COL_TEXT);

    /* ---- Speed Limit Indicator (below temperature) ---- */
    int sl_y = ry2 + 40;
    SDL_SetRenderDrawColor(renderer, 210,210,210,255);
    SDL_RenderDrawRect(renderer, &(SDL_Rect){rx-10, sl_y-20, 200, 50});
    
    draw_text(rx, sl_y-16, 2, "SPEED LIMIT", COL_TEXT);
    
    /* Speed limit value with alarm color if exceeded */
    SDL_Color limit_color = (sim_speed_kph > sim_speed_limit) ? COL_ERR : COL_TEXT;
    draw_number(rx + 20, sl_y+6, 3, (int)sim_speed_limit, limit_color);
    draw_text(rx + 20 + 3*6*3 + 6, sl_y+12, 1, "KPH", limit_color);

    /* small help bar */
    SDL_SetRenderDrawColor(renderer, 60,60,70,255);
    SDL_RenderFillRect(renderer,&(SDL_Rect){x+10, y+h-30, w-20, 20});
    draw_text(x+20, y+h-28, 1, "Arrows: speed   H/L: temp   M: voice   Q/P: limit   R: spawn", COL_TEXT);
}

/* ===========================================================
 *  RENDER FRAME
 * =========================================================== */
static void render_frame(void){
    int winW, winH; SDL_GetRendererOutputSize(renderer, &winW, &winH);
    SDL_SetRenderDrawColor(renderer, COL_BG.r,COL_BG.g,COL_BG.b,255);
    SDL_RenderClear(renderer);

    ws_x = 0; ws_y = 0; ws_w = winW; ws_h = winH/2;       /* Top: windshield full width */
    SDL_Rect dash = { 0, winH/2, winW, winH - winH/2 };   /* Bottom: dashboard full width */

    static bool once=false;
    if(!once){ buildings_init(ws_x,ws_y,ws_w,ws_h); once=true; }

    /* physics/auto systems */
    rain_update();
    distance_update_and_auto_brake();

    /* dt for pedestrian */
    static uint32_t prev_ms = 0U;
    uint32_t tnow = now_ms();
    float dt = (prev_ms==0U) ? 0.016f : (float)(tnow - prev_ms) / 1000.0f;
    prev_ms = tnow;
    pedestrian_update(dt);

    /* Draw windshield layers */
    float px_per_tick = (float)sim_speed_kph * 0.15f; /* building parallax */
    buildings_update_and_draw(px_per_tick);
    draw_pedestrian();          /* obstacle visible */
    draw_rain();
    draw_wipers_and_wiped_area();
    
    /* Draw user's car at bottom left (driver perspective) */
    draw_user_car();

    /* Draw dashboard */
    draw_dashboard(dash.x,dash.y,dash.w,dash.h);

    SDL_RenderPresent(renderer);
}

/* ===========================================================
 *  PUBLIC HAL (SDL)
 * =========================================================== */
bool hal_sdl_init(void) {
    window = SDL_CreateWindow("Car Simulator — Driver View",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              1280, 720,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) { printf("SDL Window error: %s\n", SDL_GetError()); return false; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer){ printf("SDL Renderer error: %s\n", SDL_GetError()); return false; }
    SDL_RenderSetLogicalSize(renderer, 1280, 720);

    /* schedule initial rain and object spawn */
    uint32_t t = now_ms();
    rain_state = RAIN_IDLE;
    rain_t_next_ms = t + 4000u;
    obj_state = 0U;
    obj_t_next_spawn_ms = t + 3000u;
    return true;
}

void hal_sdl_cleanup(void){
    if (renderer){ SDL_DestroyRenderer(renderer); renderer=NULL; }
    if (window){ SDL_DestroyWindow(window); window=NULL; }
}

void hal_sdl_step(void){
    if (!platform_sdl_pump_events()) return;
    handle_keyboard_input();
    render_frame();
}

/* ===========================================================
 *  HAL SENSOR FEEDS
 * =========================================================== */
bool hal_get_vehicle_ready(void){ return sim_vehicle_ready; }
bool hal_driver_brake_pressed(void){ return sim_driver_brake; }
uint32_t hal_now_ms(void){ return platform_get_time_ms(); }

bool hal_read_distance_mm(uint16_t* out_mm, uint32_t* out_ts_ms){
    if((out_mm==NULL)||(out_ts_ms==NULL)) return false;
    *out_mm = sim_distance_mm;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_rain_level_pct(uint8_t* out_pct, uint32_t* out_ts_ms){
    if((out_pct==NULL)||(out_ts_ms==NULL)) return false;
    *out_pct  = sim_rain_pct;
    *out_ts_ms= hal_now_ms();
    return true;
}

bool hal_read_vehicle_speed_kph(uint16_t* out_kph, uint32_t* out_ts_ms){
    if((out_kph==NULL)||(out_ts_ms==NULL)) return false;
    *out_kph = sim_speed_kph;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_poll_speed_limit_kph(uint16_t* out_limit_kph){
    static uint16_t last_reported = 0U;
    if(out_limit_kph==NULL) return false;
    if(sim_speed_limit != last_reported){
        *out_limit_kph = sim_speed_limit;
        last_reported = sim_speed_limit;
        return true;
    }
    return false;
}

bool hal_parking_gap_read(park_gap_t* out, uint32_t* out_ts_ms){
    if ((out==NULL)||(out_ts_ms==NULL)) return false;
    out->found = sim_gap_found;
    out->width_mm = sim_gap_width;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_cabin_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms){
    if((out_tc_x10==NULL)||(out_ts_ms==NULL)) return false;
    *out_tc_x10 = sim_cabin_temp;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_ambient_temp_c(int16_t* out_tc_x10, uint32_t* out_ts_ms){
    if((out_tc_x10==NULL)||(out_ts_ms==NULL)) return false;
    *out_tc_x10 = sim_ambient_temp;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_humidity_pct(uint8_t* out_pct, uint32_t* out_ts_ms){
    if((out_pct==NULL)||(out_ts_ms==NULL)) return false;
    *out_pct = sim_humidity;
    *out_ts_ms = hal_now_ms();
    return true;
}

bool hal_read_voice_line(char* buf, uint16_t len){
    if((buf==NULL)||(len==0U)) return false;
    if(sim_voice_available){
        (void)strncpy(buf, sim_voice_buffer, (size_t)len-1U);
        buf[len-1U] = '\0';
        sim_voice_available = false;
        sim_voice_buffer[0]='\0';
        return true;
    }
    return false;
}

/* ===========================================================
 *  ACTUATORS (from app)
 * =========================================================== */
void hal_set_brake_request(bool on){ last_brake_request = on; }
void hal_set_wiper_mode(uint8_t mode){ last_wiper_mode = mode; }
void hal_set_alarm(bool on){ last_alarm = on; }
void hal_set_speed_limit_request(uint16_t kph){ last_limit_request = kph; (void)last_limit_request; }
void hal_set_climate(uint8_t fan_stage, bool ac_on, uint8_t blend_pct){
    last_fan_stage = fan_stage; last_ac_on = ac_on; last_blend_pct = blend_pct;
    (void)last_fan_stage; (void)last_blend_pct;
}
void hal_actuate_parking_prompt(uint8_t step_code){ last_park_step = step_code; (void)last_park_step; }

#endif /* !HEADLESS_BUILD */
