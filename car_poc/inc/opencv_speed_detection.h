#ifndef OPENCV_SPEED_DETECTION_H
#define OPENCV_SPEED_DETECTION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Structure to hold speed sign detection results */
typedef struct {
    int x;              /* Bounding box X coordinate */
    int y;              /* Bounding box Y coordinate */
    int width;          /* Bounding box width */
    int height;         /* Bounding box height */
    int speed_limit;    /* Detected speed limit (kmph) */
    float confidence;   /* Detection confidence (0.0 to 1.0) */
} speed_sign_detection_t;

/* Initialize OpenCV speed detection system
 * Returns: 1 on success, 0 on failure
 */
int opencv_speed_detection_init(void);

/* Detect speed signs in image data
 * Parameters:
 *   image_data: Raw image data (BGR or BGRA format)
 *   width: Image width in pixels
 *   height: Image height in pixels  
 *   channels: Number of channels (3 for BGR, 4 for BGRA)
 *   detections: Array to store detection results
 *   max_detections: Maximum number of detections to return
 * Returns: Number of speed signs detected
 */
int opencv_detect_speed_signs(unsigned char* image_data, int width, int height, 
                             int channels, speed_sign_detection_t* detections, 
                             int max_detections);

/* Clean up OpenCV resources */
void opencv_speed_detection_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* OPENCV_SPEED_DETECTION_H */