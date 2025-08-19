#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static uint32_t start_time_ms = 0U;

void platform_assert(bool cond) {
    if (!cond) {
        fprintf(stderr, "Assertion failed!\n");
        exit(1);
    }
}

uint32_t platform_get_time_ms(void) {
#ifdef _WIN32
    return (uint32_t)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((tv.tv_sec * 1000U) + (tv.tv_usec / 1000U));
#endif
}

void platform_init(void) {
    start_time_ms = platform_get_time_ms();
}

void platform_sleep_ms(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000U);
#endif
}