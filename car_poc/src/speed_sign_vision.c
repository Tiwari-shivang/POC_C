#include "speed_sign_vision.h"
#include "hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#ifdef USE_OPENCV
#include "opencv_speed_detection.h"
#endif

#define MAX_DETECTIONS 5
#define MIN_CONFIDENCE 0.6f
#define DETECTION_TIMEOUT_MS 500U

typedef struct {
    uint32_t timestamp_ms;
    uint16_t speed_limit_kph;
    float confidence;
    bool valid;
} sign_detection_cache_t;

static sign_detection_cache_t detection_cache = {0U, 0U, 0.0f, false};
static bool vision_initialized = false;

bool speed_sign_vision_init(void) {
    if (vision_initialized) {
        return true;
    }
    
#ifdef USE_OPENCV
    int result = opencv_speed_detection_init();
    if (result == 1) {
        vision_initialized = true;
        detection_cache.valid = false;
        return true;
    }
    return false;
#else
    /* OpenCV not available - return success but no actual functionality */
    vision_initialized = true;
    detection_cache.valid = false;
    return true;
#endif
}

void speed_sign_vision_cleanup(void) {
    if (vision_initialized) {
#ifdef USE_OPENCV
        opencv_speed_detection_cleanup();
#endif
        vision_initialized = false;
        detection_cache.valid = false;
    }
}

bool speed_sign_vision_process_frame(const uint8_t* frame_data, uint16_t width, 
                                   uint16_t height, uint8_t channels) {
    if (!vision_initialized || frame_data == NULL) {
        return false;
    }
    
#ifdef USE_OPENCV
    speed_sign_detection_t detections[MAX_DETECTIONS];
    int detection_count = opencv_detect_speed_signs(
        (unsigned char*)frame_data, 
        (int)width, 
        (int)height, 
        (int)channels,
        detections, 
        MAX_DETECTIONS
    );
    
    if (detection_count <= 0) {
        return false;
    }
    
    /* Find best detection (highest confidence) */
    float best_confidence = 0.0f;
    int best_detection_idx = -1;
    
    for (int i = 0; i < detection_count; i++) {
        if (detections[i].confidence > best_confidence && 
            detections[i].confidence >= MIN_CONFIDENCE) {
            best_confidence = detections[i].confidence;
            best_detection_idx = i;
        }
    }
    
    if (best_detection_idx >= 0) {
        /* Update detection cache */
        detection_cache.timestamp_ms = hal_now_ms();
        detection_cache.speed_limit_kph = (uint16_t)detections[best_detection_idx].speed_limit;
        detection_cache.confidence = detections[best_detection_idx].confidence;
        detection_cache.valid = true;
        return true;
    }
#else
    /* No OpenCV - no processing possible */
    (void)width; (void)height; (void)channels; /* suppress warnings */
#endif
    
    return false;
}

bool speed_sign_vision_get_latest_detection(speed_sign_result_t* result) {
    if (result == NULL || !vision_initialized) {
        return false;
    }
    
    uint32_t current_time = hal_now_ms();
    
    /* Check if cached detection is still valid (not too old) */
    if (detection_cache.valid && 
        (current_time - detection_cache.timestamp_ms) <= DETECTION_TIMEOUT_MS) {
        result->speed_limit_kph = detection_cache.speed_limit_kph;
        result->confidence = detection_cache.confidence;
        result->timestamp_ms = detection_cache.timestamp_ms;
        return true;
    }
    
    /* Mark cache as invalid if too old */
    if (detection_cache.valid && 
        (current_time - detection_cache.timestamp_ms) > DETECTION_TIMEOUT_MS) {
        detection_cache.valid = false;
    }
    
    return false;
}

bool speed_sign_vision_is_initialized(void) {
    return vision_initialized;
}