#ifndef SPEED_SIGN_VISION_H
#define SPEED_SIGN_VISION_H

#include <stdint.h>
#include <stdbool.h>

/* Result structure for speed sign detection */
typedef struct {
    uint16_t speed_limit_kph;   /* Detected speed limit in km/h */
    float confidence;           /* Detection confidence (0.0 to 1.0) */
    uint32_t timestamp_ms;      /* When this detection was made */
} speed_sign_result_t;

/* Initialize the speed sign vision system
 * Returns: true on success, false on failure
 */
bool speed_sign_vision_init(void);

/* Process a camera frame for speed sign detection
 * Parameters:
 *   frame_data: Raw image data (BGR/BGRA format)
 *   width: Image width in pixels
 *   height: Image height in pixels
 *   channels: Number of color channels (3 or 4)
 * Returns: true if any signs detected, false otherwise
 */
bool speed_sign_vision_process_frame(const uint8_t* frame_data, uint16_t width, 
                                   uint16_t height, uint8_t channels);

/* Get the latest valid speed sign detection result
 * Parameters:
 *   result: Pointer to structure to fill with detection data
 * Returns: true if valid detection available, false otherwise
 */
bool speed_sign_vision_get_latest_detection(speed_sign_result_t* result);

/* Check if vision system is properly initialized
 * Returns: true if initialized, false otherwise
 */
bool speed_sign_vision_is_initialized(void);

/* Clean up vision system resources */
void speed_sign_vision_cleanup(void);

#endif /* SPEED_SIGN_VISION_H */