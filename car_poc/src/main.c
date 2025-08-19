#include "platform.h"
#include "hal.h"
#include "app_autobrake.h"
#include "app_wipers.h"
#include "app_speedgov.h"
#include "app_autopark.h"
#include "app_climate.h"
#include "app_voice.h"
#include "scenario.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HEADLESS_BUILD
extern void platform_init(void);
extern void platform_sleep_ms(uint32_t ms);
extern void hal_mock_cleanup(void);
#else
extern bool platform_sdl_init(void);
extern void platform_sdl_quit(void);
extern void platform_sdl_sleep(uint32_t ms);
extern bool hal_sdl_init(void);
extern void hal_sdl_cleanup(void);
extern void hal_sdl_step(void);
#endif

static bool running = true;
static const char* scenario_file = "cfg/scenario_default.csv";

static void init_all_modules(void) {
    app_autobrake_init();
    app_wipers_init();
    app_speedgov_init();
    app_autopark_init();
    app_climate_init();
    app_voice_init();
}

static void tick_10ms(void) {
    app_autobrake_step();
    app_wipers_step();
    app_speedgov_step();
    app_autopark_step();
    app_climate_step();
    app_voice_step();
}

static void parse_arguments(int argc, char* argv[]) {
    int i = 0;
    
    for (i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "--scenario") == 0) && ((i + 1) < argc)) {
            scenario_file = argv[i + 1];
            i++;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --scenario <file>  Specify scenario CSV file\n");
            printf("  --help             Show this help\n");
            exit(0);
        } else {
        }
    }
}

#if HEADLESS_BUILD
int main(int argc, char* argv[]) {
    uint32_t last_tick_time = 0U;
    uint32_t current_time = 0U;
    uint32_t elapsed_time = 0U;
    
    parse_arguments(argc, argv);
    
    printf("Starting Car PoC (Headless mode)\n");
    printf("Scenario file: %s\n", scenario_file);
    
    platform_init();
    
    if (!scenario_init(scenario_file)) {
        fprintf(stderr, "Failed to open scenario file: %s\n", scenario_file);
        return 1;
    }
    
    init_all_modules();
    
    last_tick_time = hal_now_ms();
    
    while (running) {
        current_time = hal_now_ms();
        elapsed_time = current_time - last_tick_time;
        
        if (elapsed_time >= TICK_MS) {
            tick_10ms();
            last_tick_time = current_time;
        }
        
        platform_sleep_ms(1U);
    }
    
    scenario_close();
    hal_mock_cleanup();
    
    printf("Car PoC simulation completed\n");
    return 0;
}

#else

int main(int argc, char* argv[]) {
    uint32_t last_tick_time = 0U;
    uint32_t current_time = 0U;
    uint32_t elapsed_time = 0U;
    
    parse_arguments(argc, argv);
    
    printf("Starting Car PoC (SDL2 Interactive mode)\n");
    
    if (!platform_sdl_init()) {
        fprintf(stderr, "Failed to initialize SDL2 platform\n");
        return 1;
    }
    
    if (!hal_sdl_init()) {
        fprintf(stderr, "Failed to initialize SDL2 HAL\n");
        platform_sdl_quit();
        return 1;
    }
    
    init_all_modules();
    
    last_tick_time = hal_now_ms();
    
    while (running) {
        hal_sdl_step();
        
        current_time = hal_now_ms();
        elapsed_time = current_time - last_tick_time;
        
        if (elapsed_time >= TICK_MS) {
            tick_10ms();
            last_tick_time = current_time;
        }
        
        platform_sdl_sleep(5U);
    }
    
    hal_sdl_cleanup();
    platform_sdl_quit();
    
    printf("Car PoC simulation completed\n");
    return 0;
}

#endif