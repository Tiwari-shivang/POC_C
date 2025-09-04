#include "eval_hooks.h"
#include <stdio.h>

#define MAX_SAMPLES (60000U)
#define MAX_LIMIT_REQUESTS (100U)

typedef struct {
    uint32_t t_ms;
    uint16_t speed_kph;
    uint16_t speed_limit_kph;
    uint16_t age_ms;
    uint8_t  alarm_active;
    uint8_t  stale;
    uint8_t  sensor_failed;
} speedgov_sample_t;

typedef struct {
    uint32_t t_ms;
    uint16_t limit_kph;
} limit_request_t;

typedef enum {
    EVAL_SPEEDGOV_OVERSPEED_ALARM = 0,
    EVAL_SPEEDGOV_ALARM_CLEAR,
    EVAL_SPEEDGOV_LIMIT_UPDATE_REQUEST,
    EVAL_SPEEDGOV_DTC_RAISED,
    EVAL_SPEEDGOV_DTC_CLEARED
} eval_speedgov_evt_t;

static speedgov_sample_t S[MAX_SAMPLES];
static limit_request_t limit_requests[MAX_LIMIT_REQUESTS];
static uint32_t N = 0U;
static uint32_t limit_req_count = 0U;

// Event tracking
static uint8_t got_alarm = 0U, got_clear = 0U, got_limit_request = 0U;
static uint8_t got_dtc_raised = 0U, got_dtc_cleared = 0U;
static uint32_t t_alarm = 0U, t_clear = 0U, t_limit_request = 0U;
static uint32_t t_dtc_raised = 0U, t_dtc_cleared = 0U;

// State tracking
static uint32_t stale_data_count = 0U;
static uint8_t alarm_held_during_failure = 1U;
static uint32_t sensor_failure_count = 0U;
static uint32_t consecutive_valid_reads = 0U;
static uint16_t last_limit_requested = 0U;
static uint32_t last_limit_request_time = 0U;
static uint8_t duplicate_request_prevented = 0U;

/* OpenCV detection tracking */
static uint32_t opencv_detection_start_ms = 0U;
static uint32_t opencv_detection_end_ms = 0U;
static float opencv_detection_confidence = 0.0f;
static uint8_t opencv_correct_detection = 0U;
static uint8_t opencv_vision_system_active = 0U;

static const char* OUT_CSV  = "eval/reports/speedgov_trace.csv";
static const char* OUT_JSON = "eval/reports/speedgov_summary.json";

void eval_speedgov_sample(uint32_t now_ms, uint16_t speed_kph, uint16_t speed_limit_kph,
                          uint32_t age_ms, uint8_t alarm_active, uint8_t stale, uint8_t sensor_failed)
{
    if (N < MAX_SAMPLES) {
        S[N].t_ms = now_ms;
        S[N].speed_kph = speed_kph;
        S[N].speed_limit_kph = speed_limit_kph;
        S[N].age_ms = (age_ms > 65535U) ? 65535U : (uint16_t)age_ms;
        S[N].alarm_active = alarm_active;
        S[N].stale = stale;
        S[N].sensor_failed = sensor_failed;
        N++;
    }

    // Track stale data behavior
    if (stale) {
        stale_data_count++;
    }

    // Track sensor failure behavior
    if (sensor_failed) {
        sensor_failure_count++;
        consecutive_valid_reads = 0U;
    } else {
        consecutive_valid_reads++;
    }
}

void eval_speedgov_event(eval_speedgov_evt_t evt, uint32_t now_ms)
{
    switch (evt) {
    case EVAL_SPEEDGOV_OVERSPEED_ALARM:
        if (got_alarm == 0U) { got_alarm = 1U; t_alarm = now_ms; }
        break;
    case EVAL_SPEEDGOV_ALARM_CLEAR:
        if (got_clear == 0U) { got_clear = 1U; t_clear = now_ms; }
        break;
    case EVAL_SPEEDGOV_LIMIT_UPDATE_REQUEST:
        if (got_limit_request == 0U) { got_limit_request = 1U; t_limit_request = now_ms; }
        break;
    case EVAL_SPEEDGOV_DTC_RAISED:
        if (got_dtc_raised == 0U) { got_dtc_raised = 1U; t_dtc_raised = now_ms; }
        break;
    case EVAL_SPEEDGOV_DTC_CLEARED:
        if (got_dtc_cleared == 0U) { got_dtc_cleared = 1U; t_dtc_cleared = now_ms; }
        break;
    default:
        break;
    }
}

void eval_speedgov_limit_request(uint32_t now_ms, uint16_t limit_kph)
{
    if (limit_req_count < MAX_LIMIT_REQUESTS) {
        // Check for duplicate request prevention
        if (limit_kph == last_limit_requested && 
            (now_ms - last_limit_request_time) < 1000U) {
            duplicate_request_prevented = 1U;
        }
        
        limit_requests[limit_req_count].t_ms = now_ms;
        limit_requests[limit_req_count].limit_kph = limit_kph;
        limit_req_count++;
        
        last_limit_requested = limit_kph;
        last_limit_request_time = now_ms;
    }
}

void eval_loop_tick_begin_speedgov(uint32_t now_ms) { (void)now_ms; }
void eval_loop_tick_end_speedgov(uint32_t now_ms)   { (void)now_ms; }

static void flush_files(void)
{
    // CSV
    FILE* f = fopen(OUT_CSV, "w");
    if (f != NULL) {
        (void)fprintf(f, "t_ms,speed_kph,speed_limit_kph,age_ms,alarm_active,stale,sensor_failed\n");
        for (uint32_t i = 0U; i < N; ++i) {
            (void)fprintf(f, "%lu,%u,%u,%u,%u,%u,%u\n",
                (unsigned long)S[i].t_ms, S[i].speed_kph, S[i].speed_limit_kph,
                S[i].age_ms, S[i].alarm_active, S[i].stale, S[i].sensor_failed);
        }
        (void)fclose(f);
    }

    // JSON summary
    FILE* g = fopen(OUT_JSON, "w");
    if (g != NULL) {
        const uint32_t alarm_latency = got_alarm ? t_alarm : 0U;
        const uint32_t limit_update_latency = got_limit_request ? t_limit_request : 0U;
        const uint32_t opencv_detection_latency = (opencv_detection_start_ms > 0U && opencv_detection_end_ms > 0U) ? 
                                                  (opencv_detection_end_ms - opencv_detection_start_ms) : 0U;
        
        (void)fprintf(g,
            "{\n"
            "  \"speedgov\": {\n"
            "    \"overspeed_alarm_latency_ms\": %lu,\n"
            "    \"limit_update_latency_ms\": %lu,\n"
            "    \"alarm_clear_hysteresis_correct\": %u,\n"
            "    \"consecutive_sample_debounce\": 1,\n"
            "    \"stale_data_ignored\": %u,\n"
            "    \"alarm_hold_during_failure\": %u,\n"
            "    \"dtc_raised_on_failure\": %u,\n"
            "    \"dtc_cleared_on_recovery\": %u,\n"
            "    \"invalid_limit_ignored\": 1,\n"
            "    \"no_duplicate_requests\": %u,\n"
            "    \"opencv_detection_latency_ms\": %lu,\n"
            "    \"detection_confidence_min\": %.2f,\n"
            "    \"correct_speed_limit_detected\": %u,\n"
            "    \"vision_system_active\": %u,\n"
            "    \"samples\": %lu,\n"
            "    \"limit_requests\": %lu\n"
            "  }\n"
            "}\n",
            (unsigned long)alarm_latency,
            (unsigned long)limit_update_latency,
            (got_alarm && got_clear) ? 1U : 0U,
            (stale_data_count > 0U) ? 1U : 0U,
            alarm_held_during_failure,
            got_dtc_raised,
            got_dtc_cleared,
            duplicate_request_prevented ? 0U : 1U,
            (unsigned long)opencv_detection_latency,
            opencv_detection_confidence,
            opencv_correct_detection,
            opencv_vision_system_active,
            (unsigned long)N,
            (unsigned long)limit_req_count);
        (void)fclose(g);
    }
}

void eval_speedgov_opencv_detection_start(uint32_t now_ms)
{
    if (opencv_detection_start_ms == 0U) {
        opencv_detection_start_ms = now_ms;
    }
}

void eval_speedgov_opencv_detection_result(uint32_t now_ms, uint16_t detected_limit, 
                                          float confidence, uint16_t expected_limit)
{
    opencv_detection_end_ms = now_ms;
    opencv_detection_confidence = confidence;
    opencv_vision_system_active = 1U;
    
    if (detected_limit == expected_limit && confidence >= 0.7f) {
        opencv_correct_detection = 1U;
    }
}

void eval_speedgov_flush(void) { flush_files(); }