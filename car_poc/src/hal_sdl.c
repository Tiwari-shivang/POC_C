#include "hal.h"
#include "platform.h"

#ifdef USE_OPENCV
#include "speed_sign_vision.h"
#endif

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

/* ---- Road Signs (Speed Limits and STOP signs) ---- */
#define MAX_ROAD_SIGNS 5
typedef enum {
    SIGN_TYPE_SPEED_LIMIT = 0,
    SIGN_TYPE_STOP = 1
} sign_type_t;

typedef struct { 
    float x; 
    sign_type_t type;
    int speed_limit;            /* for speed limit signs */
    uint32_t spawn_time_ms;
    bool active;
    uint32_t detected_time_ms;  /* when detected in center view */
    float confidence;           /* detection confidence */
    bool dashboard_updated;     /* has dashboard been updated for this sign */
} road_sign_t;
static road_sign_t road_signs[MAX_ROAD_SIGNS];
static uint32_t next_sign_spawn_ms = 0U;

#ifdef USE_OPENCV
/* ---- OpenCV Vision Integration ---- */
static uint32_t last_vision_process_ms = 0U;
static uint32_t vision_process_interval_ms = 100U; /* Process every 100ms */
static speed_sign_result_t detected_sign = {0U, 0.0f, 0U};
#endif

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
 *  SPEED SIGN BOARDS (100x100 px, red circle, white text)
 * =========================================================== */
static void road_signs_init(void){
    for(int i = 0; i < MAX_ROAD_SIGNS; i++){
        road_signs[i].active = false;
        road_signs[i].x = 0.0f;
        road_signs[i].type = SIGN_TYPE_SPEED_LIMIT;
        road_signs[i].speed_limit = 50;
        road_signs[i].spawn_time_ms = 0U;
        road_signs[i].detected_time_ms = 0U;
        road_signs[i].confidence = 0.0f;
        road_signs[i].dashboard_updated = false;
    }
    next_sign_spawn_ms = now_ms() + 3000U; /* first sign in 3 seconds */
}

static void spawn_road_sign(void){
    /* Find inactive sign slot */
    for(int i = 0; i < MAX_ROAD_SIGNS; i++){
        if(!road_signs[i].active){
            road_signs[i].active = true;
            road_signs[i].x = (float)(ws_x + ws_w + 150); /* spawn off right edge */
            
            uint32_t seed = now_ms() * 1103515245u + 12345u + (uint32_t)i;
            
            /* Decide sign type: 70% speed limit, 30% STOP sign */
            if((seed % 100) < 70){
                /* Speed limit sign */
                road_signs[i].type = SIGN_TYPE_SPEED_LIMIT;
                int speed_options[] = {30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130};
                int num_options = sizeof(speed_options) / sizeof(speed_options[0]);
                road_signs[i].speed_limit = speed_options[(seed / 100) % num_options];
            } else {
                /* STOP sign */
                road_signs[i].type = SIGN_TYPE_STOP;
                road_signs[i].speed_limit = 0; /* Not applicable for STOP signs */
            }
            
            road_signs[i].spawn_time_ms = now_ms();
            road_signs[i].detected_time_ms = 0U;
            road_signs[i].confidence = 0.0f;
            road_signs[i].dashboard_updated = false;
            
            /* Schedule next sign spawn in 8-12 seconds */
            next_sign_spawn_ms = now_ms() + 8000U + (seed % 4000U);
            
            if(road_signs[i].type == SIGN_TYPE_SPEED_LIMIT){
                printf("Spawned SPEED LIMIT sign: %d km/h\n", road_signs[i].speed_limit);
            } else {
                printf("Spawned STOP sign\n");
            }
            break;
        }
    }
}

static void draw_speed_limit_sign(road_sign_t* sign){
    if(!sign->active) return;
    
    int sign_size = 120; /* Larger for better visibility */
    int sign_x = (int)sign->x;
    int sign_y = ws_y + ws_h - sign_size - 30; /* bottom of windshield */
    
    /* White background circle */
    SDL_Color bg_white = {255, 255, 255, 255};
    SDL_SetRenderDrawColor(renderer, bg_white.r, bg_white.g, bg_white.b, 255);
    
    /* Draw filled circle */
    int radius = sign_size / 2;
    int center_x = sign_x + radius;
    int center_y = sign_y + radius;
    
    for(int w = 0; w < sign_size; w++){
        for(int h = 0; h < sign_size; h++){
            int dx = w - radius;
            int dy = h - radius;
            if((dx*dx + dy*dy) <= (radius*radius)){
                SDL_RenderDrawPoint(renderer, sign_x + w, sign_y + h);
            }
        }
    }
    
    /* Red border circle */
    SDL_Color red_border = {220, 20, 20, 255};
    SDL_SetRenderDrawColor(renderer, red_border.r, red_border.g, red_border.b, 255);
    
    /* Draw thick circle border */
    for(int thickness = 0; thickness < 10; thickness++){
        int border_radius = radius - 8 - thickness;
        if(border_radius > 0){
            for(int angle = 0; angle < 360; angle += 1){
                int x = center_x + (int)(border_radius * cosf(deg2rad((float)angle)));
                int y = center_y + (int)(border_radius * sinf(deg2rad((float)angle)));
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    
    /* Speed limit number (black text) */
    SDL_Color black_text = {0, 0, 0, 255};
    char speed_str[8];
    snprintf(speed_str, sizeof(speed_str), "%d", sign->speed_limit);
    
    /* Calculate text position to center it */
    int digit_count = (sign->speed_limit >= 100) ? 3 : ((sign->speed_limit >= 10) ? 2 : 1);
    int text_scale = (digit_count >= 3) ? 5 : 6; /* bigger text */
    int text_width = digit_count * 6 * text_scale;
    int text_x = center_x - text_width / 2;
    int text_y = center_y - (7 * text_scale) / 2;
    
    draw_text(text_x, text_y, text_scale, speed_str, black_text);
    
    /* Post/pole (gray rectangle below sign) */
    SDL_Color pole_gray = {120, 120, 130, 255};
    SDL_SetRenderDrawColor(renderer, pole_gray.r, pole_gray.g, pole_gray.b, 255);
    SDL_Rect pole = {center_x - 4, sign_y + sign_size, 8, 50};
    SDL_RenderFillRect(renderer, &pole);
}

static void draw_stop_sign(road_sign_t* sign){
    if(!sign->active) return;
    
    int sign_size = 120;
    int sign_x = (int)sign->x;
    int sign_y = ws_y + ws_h - sign_size - 30;
    
    /* Red octagonal background */
    SDL_Color red_bg = {220, 20, 20, 255};
    SDL_SetRenderDrawColor(renderer, red_bg.r, red_bg.g, red_bg.b, 255);
    
    /* Draw octagon (simplified as filled rectangle with cut corners) */
    int center_x = sign_x + sign_size/2;
    int center_y = sign_y + sign_size/2;
    int oct_size = sign_size - 10;
    
    /* Main rectangle */
    SDL_Rect main_rect = {center_x - oct_size/3, center_y - oct_size/2, (oct_size*2)/3, oct_size};
    SDL_RenderFillRect(renderer, &main_rect);
    
    /* Top and bottom rectangles */
    SDL_Rect top_rect = {center_x - oct_size/2, center_y - oct_size/3, oct_size, (oct_size*2)/3};
    SDL_RenderFillRect(renderer, &top_rect);
    
    /* White border */
    SDL_Color white_border = {255, 255, 255, 255};
    SDL_SetRenderDrawColor(renderer, white_border.r, white_border.g, white_border.b, 255);
    for(int i = 0; i < 4; i++){
        SDL_Rect border1 = {main_rect.x - i, main_rect.y - i, main_rect.w + 2*i, main_rect.h + 2*i};
        SDL_Rect border2 = {top_rect.x - i, top_rect.y - i, top_rect.w + 2*i, top_rect.h + 2*i};
        SDL_RenderDrawRect(renderer, &border1);
        SDL_RenderDrawRect(renderer, &border2);
    }
    
    /* "STOP" text (white) */
    SDL_Color white_text = {255, 255, 255, 255};
    int text_x = center_x - 30;
    int text_y = center_y - 20;
    draw_text(text_x, text_y, 4, "STOP", white_text);
    
    /* Post/pole */
    SDL_Color pole_gray = {120, 120, 130, 255};
    SDL_SetRenderDrawColor(renderer, pole_gray.r, pole_gray.g, pole_gray.b, 255);
    SDL_Rect pole = {center_x - 4, sign_y + sign_size, 8, 50};
    SDL_RenderFillRect(renderer, &pole);
}

static void draw_road_sign(road_sign_t* sign){
    if(!sign->active) return;
    
    if(sign->type == SIGN_TYPE_SPEED_LIMIT){
        draw_speed_limit_sign(sign);
    } else if(sign->type == SIGN_TYPE_STOP){
        draw_stop_sign(sign);
    }
    
    /* Show detection indicator if sign has been detected */
    if (sign->detected_time_ms > 0U) {
        /* Bright green detection indicator */
        SDL_Color detected_green = {0, 255, 0, 255};
        SDL_SetRenderDrawColor(renderer, detected_green.r, detected_green.g, detected_green.b, 255);
        
        int sign_x = (int)sign->x;
        int sign_y = ws_y + ws_h - 120 - 30;
        int indicator_x = sign_x + 130;
        int indicator_y = sign_y + 10;
        int indicator_radius = 12;
        
        /* Larger detection indicator circle */
        for(int w = 0; w < indicator_radius * 2; w++){
            for(int h = 0; h < indicator_radius * 2; h++){
                int dx = w - indicator_radius;
                int dy = h - indicator_radius;
                if((dx*dx + dy*dy) <= (indicator_radius*indicator_radius)){
                    SDL_RenderDrawPoint(renderer, indicator_x + w, indicator_y + h);
                }
            }
        }
        
        /* "DETECTED" text */
        draw_text(indicator_x - 20, indicator_y - 25, 2, "DETECTED", detected_green);
        
        /* Show what was detected */
        if(sign->type == SIGN_TYPE_SPEED_LIMIT){
            char detected_str[32];
            snprintf(detected_str, sizeof(detected_str), "%d KM/H", sign->speed_limit);
            draw_text(indicator_x - 25, indicator_y + 30, 1, detected_str, detected_green);
        } else {
            draw_text(indicator_x - 15, indicator_y + 30, 1, "STOP", detected_green);
        }
    }
}

static void road_signs_update_and_draw(float px_per_tick){
    uint32_t current_time = now_ms();
    
    /* Spawn new signs every 8-12 seconds */
    if(current_time >= next_sign_spawn_ms && sim_speed_kph > 5U){
        spawn_road_sign();
    }
    
    /* Update and draw existing signs */
    for(int i = 0; i < MAX_ROAD_SIGNS; i++){
        if(road_signs[i].active){
            /* Check if sign is in "detection zone" (center portion of windshield) */
            int sign_center_x = (int)road_signs[i].x + 60; /* center of 120px sign */
            int detection_zone_start = ws_x + ws_w/3;       /* left third of windshield */
            int detection_zone_end = ws_x + (2*ws_w)/3;     /* right third of windshield */
            
            /* Detect and process sign when in center view */
            if(sign_center_x >= detection_zone_start && sign_center_x <= detection_zone_end &&
               road_signs[i].detected_time_ms == 0U) {
                
                /* Mark as detected */
                road_signs[i].detected_time_ms = current_time;
                road_signs[i].confidence = 0.95f; /* High confidence for simulation */
                
                if(road_signs[i].type == SIGN_TYPE_SPEED_LIMIT && !road_signs[i].dashboard_updated){
                    /* Update dashboard for speed limit signs */
                    sim_speed_limit = (uint16_t)road_signs[i].speed_limit;
                    road_signs[i].dashboard_updated = true;
                    
                    printf("🚗 SPEED LIMIT DETECTED: %d km/h → Dashboard Updated!\n", 
                           road_signs[i].speed_limit);
                } else if(road_signs[i].type == SIGN_TYPE_STOP){
                    printf("🛑 STOP SIGN DETECTED: Vehicle should prepare to stop!\n");
                    
                    /* Optional: Trigger emergency brake or warning */
                    /* This could integrate with the auto-brake system */
                }
            }
            
            /* Move sign backward (car moving forward effect) */
            if(sim_speed_kph > 0U){
                road_signs[i].x -= px_per_tick;
            }
            
            /* Deactivate sign when it goes off screen */
            if(road_signs[i].x + 120 < ws_x){
                road_signs[i].active = false;
            } else {
                draw_road_sign(&road_signs[i]);
            }
        }
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
    if(!once){ 
        buildings_init(ws_x,ws_y,ws_w,ws_h); 
        road_signs_init();
        once=true; 
    }

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
    road_signs_update_and_draw(px_per_tick); /* road signs move like buildings */
    draw_pedestrian();          /* obstacle visible */
    draw_rain();
    draw_wipers_and_wiped_area();
    
    /* Draw detection zone indicator (shows where camera is looking) */
    if(sim_speed_kph > 0U){ /* only show when moving */
        SDL_Color zone_color = {100, 255, 100, 100}; /* translucent green */
        SDL_SetRenderDrawColor(renderer, zone_color.r, zone_color.g, zone_color.b, 100);
        
        int detection_zone_start = ws_x + ws_w/3;
        int detection_zone_end = ws_x + (2*ws_w)/3;
        int zone_width = detection_zone_end - detection_zone_start;
        
        /* Top and bottom borders of detection zone */
        for(int i = 0; i < 3; i++){
            SDL_Rect top_border = {detection_zone_start, ws_y + 20 + i, zone_width, 1};
            SDL_Rect bottom_border = {detection_zone_start, ws_y + ws_h - 100 + i, zone_width, 1};
            SDL_RenderFillRect(renderer, &top_border);
            SDL_RenderFillRect(renderer, &bottom_border);
        }
        
        /* Side borders */
        for(int i = 0; i < 3; i++){
            SDL_Rect left_border = {detection_zone_start + i, ws_y + 20, 1, ws_h - 120};
            SDL_Rect right_border = {detection_zone_end - i, ws_y + 20, 1, ws_h - 120};
            SDL_RenderFillRect(renderer, &left_border);
            SDL_RenderFillRect(renderer, &right_border);
        }
        
        /* "DETECTION ZONE" label */
        draw_text(detection_zone_start + 10, ws_y + 30, 1, "CAMERA VIEW", zone_color);
    }
    
    /* Draw user's car at bottom left (driver perspective) */
    draw_user_car();

    /* Draw dashboard */
    draw_dashboard(dash.x,dash.y,dash.w,dash.h);

    SDL_RenderPresent(renderer);
}

#ifdef USE_OPENCV
/* ===========================================================
 *  OPENCV FRAME CAPTURE & PROCESSING
 * =========================================================== */
static void process_frame_with_opencv(void) {
    uint32_t current_time = now_ms();
    
    /* Only process frames at specified intervals to avoid performance issues */
    if (current_time - last_vision_process_ms < vision_process_interval_ms) {
        return;
    }
    
    last_vision_process_ms = current_time;
    
    /* Skip if vision system not initialized */
    if (!speed_sign_vision_is_initialized()) {
        return;
    }
    
    /* Capture current windshield area as frame */
    if (renderer == NULL) return;
    
    /* Create a surface to capture the windshield area */
    SDL_Surface* windshield_surface = SDL_CreateRGBSurface(0, ws_w, ws_h, 32,
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    
    if (windshield_surface == NULL) return;
    
    /* Read pixels from the windshield area */
    SDL_Rect windshield_rect = {ws_x, ws_y, ws_w, ws_h};
    if (SDL_RenderReadPixels(renderer, &windshield_rect, 
                            windshield_surface->format->format,
                            windshield_surface->pixels, 
                            windshield_surface->pitch) == 0) {
        
        /* Process with OpenCV vision system */
        bool detection_result = speed_sign_vision_process_frame(
            (const uint8_t*)windshield_surface->pixels,
            (uint16_t)ws_w,
            (uint16_t)ws_h,
            4  /* BGRA format */
        );
        
        if (detection_result) {
            /* Get the latest detection result */
            speed_sign_vision_get_latest_detection(&detected_sign);
        }
    }
    
    SDL_FreeSurface(windshield_surface);
}
#endif /* USE_OPENCV */

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

#ifdef USE_OPENCV
    /* Initialize speed sign vision system */
    if (!speed_sign_vision_init()) {
        printf("Warning: OpenCV speed sign vision system failed to initialize\n");
    } else {
        printf("OpenCV speed sign vision system initialized successfully\n");
    }
#else
    printf("OpenCV integration disabled - using manual speed limit controls only\n");
#endif

    /* schedule initial rain and object spawn */
    uint32_t t = now_ms();
    rain_state = RAIN_IDLE;
    rain_t_next_ms = t + 4000u;
    obj_state = 0U;
    obj_t_next_spawn_ms = t + 3000u;
    return true;
}

void hal_sdl_cleanup(void){
#ifdef USE_OPENCV
    /* Cleanup speed sign vision system */
    speed_sign_vision_cleanup();
#endif
    
    if (renderer){ SDL_DestroyRenderer(renderer); renderer=NULL; }
    if (window){ SDL_DestroyWindow(window); window=NULL; }
}

void hal_sdl_step(void){
    if (!platform_sdl_pump_events()) return;
    handle_keyboard_input();
    render_frame();
    
#ifdef USE_OPENCV
    /* Process frame with OpenCV for speed sign detection */
    process_frame_with_opencv();
#endif
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
    
#ifdef USE_OPENCV
    /* Check for OpenCV-detected speed limit first */
    speed_sign_result_t vision_result;
    if (speed_sign_vision_get_latest_detection(&vision_result)) {
        /* Use detected speed limit if confidence is high enough */
        if (vision_result.confidence >= 0.7f && vision_result.speed_limit_kph != last_reported) {
            *out_limit_kph = vision_result.speed_limit_kph;
            last_reported = vision_result.speed_limit_kph;
            
            /* Update simulator display limit to match detected limit */
            sim_speed_limit = vision_result.speed_limit_kph;
            
            printf("OpenCV detected speed limit: %d km/h (confidence: %.2f)\n", 
                   vision_result.speed_limit_kph, vision_result.confidence);
            return true;
        }
    }
#endif
    
    /* Check if speed limit changed (from road signs or manual controls) */
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
