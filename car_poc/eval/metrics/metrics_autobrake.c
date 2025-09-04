#include "eval_hooks.h"
#include <stdio.h>

#define MAX_SAMPLES (60000U)

typedef struct {
    uint32_t t_ms;
    uint16_t dist_mm;
    uint16_t age_ms;
    uint8_t  hit;
    uint8_t  brake;
} sample_t;

static sample_t S[MAX_SAMPLES];
static uint32_t N = 0U;
static uint8_t  got_first = 0U, got_flag = 0U, got_assert = 0U;
static uint32_t t_first = 0U, t_flag = 0U, t_assert = 0U;

static const char* OUT_CSV  = "eval/reports/autobrake_trace.csv";
static const char* OUT_JSON = "eval/reports/autobrake_summary.json";

void eval_autobrake_sample(uint32_t now_ms, uint16_t dist_mm,
                           uint32_t age_ms, uint8_t hit, bool brake)
{
    if (N < MAX_SAMPLES) {
        S[N].t_ms   = now_ms;
        S[N].dist_mm= dist_mm;
        S[N].age_ms = (age_ms > 65535U) ? 65535U : (uint16_t)age_ms;
        S[N].hit    = hit;
        S[N].brake  = (brake ? 1U : 0U);
        N++;
    }
    (void)now_ms;
}

void eval_autobrake_event(eval_evt_t evt, uint32_t now_ms)
{
    switch (evt) {
    case EVAL_EVT_FIRST_BELOW_THRESH: if (got_first == 0U) { got_first = 1U; t_first = now_ms; } break;
    case EVAL_EVT_HAZARD_FLAG:        if (got_flag  == 0U) { got_flag  = 1U; t_flag  = now_ms; } break;
    case EVAL_EVT_BRAKE_ASSERT:       if (got_assert== 0U) { got_assert= 1U; t_assert= now_ms; } break;
    case EVAL_EVT_BRAKE_DEASSERT:
    default: break;
    }
}

void eval_loop_tick_begin(uint32_t now_ms) { (void)now_ms; }
void eval_loop_tick_end(uint32_t now_ms)   { (void)now_ms; }

/* Flush artifacts at the end of an eval */
static void flush_files(void)
{
    /* CSV */
    FILE* f = fopen(OUT_CSV, "w");
    if (f != NULL) {
        (void)fprintf(f, "t_ms,dist_mm,age_ms,hit,brake\n");
        for (uint32_t i = 0U; i < N; ++i) {
            (void)fprintf(f, "%lu,%u,%u,%u,%u\n",
                (unsigned long)S[i].t_ms, S[i].dist_mm, S[i].age_ms, S[i].hit, S[i].brake);
        }
        (void)fclose(f);
    }

    /* JSON summary */
    FILE* g = fopen(OUT_JSON, "w");
    if (g != NULL) {
        const uint32_t detect_latency = (got_first && got_flag)  ? (t_flag  - t_first) : 0U;
        const uint32_t react_latency  = (got_flag  && got_assert)? (t_assert- t_flag ) : 0U;
        (void)fprintf(g,
            "{\n"
            "  \"autobrake\": {\n"
            "    \"detect_latency_ms\": %lu,\n"
            "    \"react_latency_ms\": %lu,\n"
            "    \"events\": {\"first\": %lu, \"flag\": %lu, \"assert\": %lu},\n"
            "    \"samples\": %lu\n"
            "  }\n"
            "}\n",
            (unsigned long)detect_latency,
            (unsigned long)react_latency,
            (unsigned long)t_first, (unsigned long)t_flag, (unsigned long)t_assert,
            (unsigned long)N);
        (void)fclose(g);
    }
}

/* Provide a symbol the eval test can call */
void eval_autobrake_flush(void) { flush_files(); }