#include "platform.h"

#if !HEADLESS_BUILD
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

static bool sdl_initialized = false;

void platform_assert(bool cond) {
    if (!cond) {
        fprintf(stderr, "Assertion failed!\n");
        exit(1);
    }
}

bool platform_sdl_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    sdl_initialized = true;
    return true;
}

void platform_sdl_quit(void) {
    if (sdl_initialized) {
        SDL_Quit();
        sdl_initialized = false;
    }
}

uint32_t platform_get_time_ms(void) {
    return SDL_GetTicks();
}

void platform_sdl_sleep(uint32_t ms) {
    SDL_Delay(ms);
}

bool platform_sdl_pump_events(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            return false;
        }
    }
    return true;
}

#endif /* !HEADLESS_BUILD */