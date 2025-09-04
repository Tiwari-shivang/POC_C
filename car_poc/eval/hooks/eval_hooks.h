#ifndef EVAL_HOOKS_H
#define EVAL_HOOKS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EVAL_EVT_FIRST_BELOW_THRESH = 0,
    EVAL_EVT_HAZARD_FLAG        = 1,
    EVAL_EVT_BRAKE_ASSERT       = 2,
    EVAL_EVT_BRAKE_DEASSERT     = 3
} eval_evt_t;

void eval_autobrake_sample(uint32_t now_ms,
                           uint16_t distance_mm,
                           uint32_t sensor_age_ms,
                           uint8_t  hit_count,
                           bool     brake_active);

void eval_autobrake_event(eval_evt_t evt, uint32_t now_ms);

void eval_loop_tick_begin(uint32_t now_ms);
void eval_loop_tick_end(uint32_t now_ms);

#endif /* EVAL_HOOKS_H */