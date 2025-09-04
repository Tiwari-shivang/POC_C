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

typedef enum {
    EVAL_WIPERS_OFF_TO_INT = 0,
    EVAL_WIPERS_INT_TO_LOW,
    EVAL_WIPERS_LOW_TO_HIGH,
    EVAL_WIPERS_HIGH_TO_LOW,
    EVAL_WIPERS_INT_TO_OFF,
    EVAL_WIPERS_PARK_START,
    EVAL_WIPERS_PARK_END
} eval_wipers_evt_t;

typedef enum {
    EVAL_SPEEDGOV_OVERSPEED_ALARM = 0,
    EVAL_SPEEDGOV_ALARM_CLEAR,
    EVAL_SPEEDGOV_LIMIT_UPDATE_REQUEST,
    EVAL_SPEEDGOV_DTC_RAISED,
    EVAL_SPEEDGOV_DTC_CLEARED
} eval_speedgov_evt_t;

/* Autobrake evaluation functions */
void eval_autobrake_sample(uint32_t now_ms,
                           uint16_t distance_mm,
                           uint32_t sensor_age_ms,
                           uint8_t  hit_count,
                           bool     brake_active);

void eval_autobrake_event(eval_evt_t evt, uint32_t now_ms);

/* Wipers evaluation functions */
void eval_wipers_sample(uint32_t now_ms, uint16_t rain_level_pct,
                        uint32_t age_ms, uint8_t mode, uint8_t stale);

void eval_wipers_event(eval_wipers_evt_t evt, uint32_t now_ms);

/* Speed governor evaluation functions */
void eval_speedgov_sample(uint32_t now_ms, uint16_t speed_kph, uint16_t speed_limit_kph,
                          uint32_t age_ms, uint8_t alarm_active, uint8_t stale, uint8_t sensor_failed);

void eval_speedgov_event(eval_speedgov_evt_t evt, uint32_t now_ms);

void eval_speedgov_limit_request(uint32_t now_ms, uint16_t limit_kph);

void eval_speedgov_opencv_detection_start(uint32_t now_ms);

void eval_speedgov_opencv_detection_result(uint32_t now_ms, uint16_t detected_limit, 
                                          float confidence, uint16_t expected_limit);

/* Common evaluation functions */
void eval_loop_tick_begin(uint32_t now_ms);
void eval_loop_tick_end(uint32_t now_ms);

/* Flush functions for generating reports */
void eval_autobrake_flush(void);
void eval_wipers_flush(void);
void eval_speedgov_flush(void);

#endif /* EVAL_HOOKS_H */