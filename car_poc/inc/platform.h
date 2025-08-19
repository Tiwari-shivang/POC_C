#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

#define TICK_MS (10U)

void platform_assert(bool cond);

#endif /* PLATFORM_H */