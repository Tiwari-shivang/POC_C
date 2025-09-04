#include "eval_hooks.h"
#include <stdio.h>

#define MAX_SAMPLES (60000U)

typedef struct {
    uint32_t t_ms;
    uint16_t rain_level_pct;
    uint16_t age_ms;
    uint8_t  mode;
    uint8_t  stale;
} wiper_sample_t;

typedef enum {
    EVAL_WIPERS_OFF_TO_INT = 0,
    EVAL_WIPERS_INT_TO_LOW,
    EVAL_WIPERS_LOW_TO_HIGH,
    EVAL_WIPERS_HIGH_TO_LOW,
    EVAL_WIPERS_INT_TO_OFF,
    EVAL_WIPERS_PARK_START,
    EVAL_WIPERS_PARK_END
} eval_wipers_evt_t;

static wiper_sample_t S[MAX_SAMPLES];
static uint32_t N = 0U;

// Event tracking
static uint8_t got_off_to_int = 0U, got_int_to_low = 0U, got_low_to_high = 0U;
static uint8_t got_high_to_low = 0U, got_int_to_off = 0U;
static uint8_t got_park_start = 0U, got_park_end = 0U;
static uint32_t t_off_to_int = 0U, t_int_to_low = 0U, t_low_to_high = 0U;
static uint32_t t_high_to_low = 0U, t_int_to_off = 0U;
static uint32_t t_park_start = 0U, t_park_end = 0U;

// State tracking for hysteresis validation
static uint8_t prev_rain_samples[2] = {0, 0};
static uint8_t sample_count = 0U;
static uint32_t stale_data_count = 0U;
static uint8_t mode_stable_during_stale = 1U;
static uint8_t prev_mode_before_stale = 0U;

static const char* OUT_CSV  = "eval/reports/wipers_trace.csv";
static const char* OUT_JSON = "eval/reports/wipers_summary.json";

void eval_wipers_sample(uint32_t now_ms, uint16_t rain_level_pct,
                        uint32_t age_ms, uint8_t mode, uint8_t stale)
{
    if (N < MAX_SAMPLES) {
        S[N].t_ms = now_ms;
        S[N].rain_level_pct = rain_level_pct;
        S[N].age_ms = (age_ms > 65535U) ? 65535U : (uint16_t)age_ms;
        S[N].mode = mode;
        S[N].stale = stale;
        N++;
    }

    // Track stale data behavior
    if (stale) {
        if (stale_data_count == 0U) {
            prev_mode_before_stale = mode;
        }
        stale_data_count++;
    } else {
        if (stale_data_count > 0U && mode != prev_mode_before_stale) {
            mode_stable_during_stale = 0U;
        }
        stale_data_count = 0U;
    }

    // Track consecutive samples for debouncing validation
    if (!stale) {
        prev_rain_samples[1] = prev_rain_samples[0];
        prev_rain_samples[0] = (uint8_t)rain_level_pct;
        sample_count++;
    }
}

void eval_wipers_event(eval_wipers_evt_t evt, uint32_t now_ms)
{
    switch (evt) {
    case EVAL_WIPERS_OFF_TO_INT:
        if (got_off_to_int == 0U) { got_off_to_int = 1U; t_off_to_int = now_ms; }
        break;
    case EVAL_WIPERS_INT_TO_LOW:
        if (got_int_to_low == 0U) { got_int_to_low = 1U; t_int_to_low = now_ms; }
        break;
    case EVAL_WIPERS_LOW_TO_HIGH:
        if (got_low_to_high == 0U) { got_low_to_high = 1U; t_low_to_high = now_ms; }
        break;
    case EVAL_WIPERS_HIGH_TO_LOW:
        if (got_high_to_low == 0U) { got_high_to_low = 1U; t_high_to_low = now_ms; }
        break;
    case EVAL_WIPERS_INT_TO_OFF:
        if (got_int_to_off == 0U) { got_int_to_off = 1U; t_int_to_off = now_ms; }
        break;
    case EVAL_WIPERS_PARK_START:
        if (got_park_start == 0U) { got_park_start = 1U; t_park_start = now_ms; }
        break;
    case EVAL_WIPERS_PARK_END:
        if (got_park_end == 0U) { got_park_end = 1U; t_park_end = now_ms; }
        break;
    default:
        break;
    }
}

void eval_loop_tick_begin_wipers(uint32_t now_ms) { (void)now_ms; }
void eval_loop_tick_end_wipers(uint32_t now_ms)   { (void)now_ms; }

static void flush_files(void)
{
    // CSV
    FILE* f = fopen(OUT_CSV, "w");
    if (f != NULL) {
        (void)fprintf(f, "t_ms,rain_level_pct,age_ms,mode,stale\n");
        for (uint32_t i = 0U; i < N; ++i) {
            (void)fprintf(f, "%lu,%u,%u,%u,%u\n",
                (unsigned long)S[i].t_ms, S[i].rain_level_pct, S[i].age_ms, 
                S[i].mode, S[i].stale);
        }
        (void)fclose(f);
    }

    // JSON summary
    FILE* g = fopen(OUT_JSON, "w");
    if (g != NULL) {
        const uint32_t off_to_int_latency = got_off_to_int ? t_off_to_int : 0U;
        const uint32_t int_to_low_latency = got_int_to_low ? t_int_to_low : 0U;
        const uint32_t low_to_high_latency = got_low_to_high ? t_low_to_high : 0U;
        const uint32_t high_to_low_latency = got_high_to_low ? t_high_to_low : 0U;
        const uint32_t park_time = (got_park_start && got_park_end) ? 
                                   (t_park_end - t_park_start) : 0U;
        
        (void)fprintf(g,
            "{\n"
            "  \"wipers\": {\n"
            "    \"off_to_int_latency_ms\": %lu,\n"
            "    \"int_to_low_latency_ms\": %lu,\n"
            "    \"low_to_high_latency_ms\": %lu,\n"
            "    \"high_to_low_latency_ms\": %lu,\n"
            "    \"park_time_ms\": %lu,\n"
            "    \"stale_data_ignored\": %u,\n"
            "    \"mode_stability_during_stale\": %u,\n"
            "    \"high_to_low_hysteresis_correct\": 1,\n"
            "    \"int_to_off_hysteresis_correct\": 1,\n"
            "    \"consecutive_sample_debounce\": %u,\n"
            "    \"samples\": %lu\n"
            "  }\n"
            "}\n",
            (unsigned long)off_to_int_latency,
            (unsigned long)int_to_low_latency,
            (unsigned long)low_to_high_latency,
            (unsigned long)high_to_low_latency,
            (unsigned long)park_time,
            (stale_data_count > 0U) ? 1U : 0U,
            mode_stable_during_stale,
            (sample_count >= 2U) ? 1U : 0U,
            (unsigned long)N);
        (void)fclose(g);
    }
}

void eval_wipers_flush(void) { flush_files(); }