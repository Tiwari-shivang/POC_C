#ifdef USE_OPENCV

#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "opencv_speed_detection.h"
}

using namespace cv;

// Template matching data for digits
struct DigitTemplate {
    Mat image;
    int digit;
    float scale;
};

static std::vector<DigitTemplate> digit_templates;
static bool templates_initialized = false;

// Initialize digit templates (simple synthetic templates)
static void init_digit_templates() {
    if (templates_initialized) return;
    
    // Create simple synthetic digit templates
    for (int digit = 0; digit <= 9; digit++) {
        DigitTemplate tmpl;
        tmpl.digit = digit;
        tmpl.scale = 1.0f;
        
        // Create a 40x60 template for each digit (simple geometric shapes)
        tmpl.image = Mat::zeros(60, 40, CV_8UC1);
        
        // Draw simple digit patterns
        switch (digit) {
            case 0:
                ellipse(tmpl.image, Point(20, 30), Size(15, 25), 0, 0, 360, Scalar(255), 3);
                break;
            case 1:
                line(tmpl.image, Point(20, 5), Point(20, 55), Scalar(255), 3);
                line(tmpl.image, Point(15, 10), Point(20, 5), Scalar(255), 3);
                break;
            case 2:
                arc(tmpl.image, Point(20, 15), Size(12, 12), 0, -30, 210, Scalar(255), 3);
                line(tmpl.image, Point(8, 55), Point(32, 55), Scalar(255), 3);
                line(tmpl.image, Point(8, 30), Point(32, 55), Scalar(255), 3);
                break;
            case 3:
                arc(tmpl.image, Point(20, 15), Size(12, 12), 0, -90, 90, Scalar(255), 3);
                arc(tmpl.image, Point(20, 45), Size(12, 12), 0, -90, 90, Scalar(255), 3);
                break;
            case 4:
                line(tmpl.image, Point(12, 5), Point(12, 35), Scalar(255), 3);
                line(tmpl.image, Point(28, 5), Point(28, 55), Scalar(255), 3);
                line(tmpl.image, Point(12, 35), Point(28, 35), Scalar(255), 3);
                break;
            case 5:
                line(tmpl.image, Point(10, 5), Point(30, 5), Scalar(255), 3);
                line(tmpl.image, Point(10, 5), Point(10, 30), Scalar(255), 3);
                line(tmpl.image, Point(10, 30), Point(25, 30), Scalar(255), 3);
                arc(tmpl.image, Point(20, 45), Size(10, 12), 0, -90, 90, Scalar(255), 3);
                break;
            case 6:
                arc(tmpl.image, Point(20, 45), Size(12, 12), 0, 0, 360, Scalar(255), 3);
                line(tmpl.image, Point(20, 5), Point(20, 33), Scalar(255), 3);
                arc(tmpl.image, Point(20, 20), Size(8, 8), 0, 90, 270, Scalar(255), 3);
                break;
            case 7:
                line(tmpl.image, Point(8, 5), Point(32, 5), Scalar(255), 3);
                line(tmpl.image, Point(32, 5), Point(15, 55), Scalar(255), 3);
                break;
            case 8:
                ellipse(tmpl.image, Point(20, 18), Size(10, 10), 0, 0, 360, Scalar(255), 3);
                ellipse(tmpl.image, Point(20, 42), Size(10, 10), 0, 0, 360, Scalar(255), 3);
                break;
            case 9:
                ellipse(tmpl.image, Point(20, 18), Size(10, 10), 0, 0, 360, Scalar(255), 3);
                line(tmpl.image, Point(30, 18), Point(30, 55), Scalar(255), 3);
                break;
        }
        digit_templates.push_back(tmpl);
    }
    templates_initialized = true;
}

// Extract speed limit from detected circular sign region
static int extract_speed_from_sign(const Mat& sign_roi) {
    if (sign_roi.empty()) return -1;
    
    init_digit_templates();
    
    // Convert to grayscale if needed
    Mat gray;
    if (sign_roi.channels() == 3) {
        cvtColor(sign_roi, gray, COLOR_BGR2GRAY);
    } else {
        gray = sign_roi.clone();
    }
    
    // Threshold to get black text on white background
    Mat thresh;
    threshold(gray, thresh, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
    
    // Find contours for digit extraction
    std::vector<std::vector<Point>> contours;
    findContours(thresh, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    
    if (contours.empty()) return -1;
    
    // Sort contours by x-position (left to right)
    std::vector<Rect> digit_rects;
    for (const auto& contour : contours) {
        Rect bbox = boundingRect(contour);
        if (bbox.width > 8 && bbox.height > 15) { // minimum digit size
            digit_rects.push_back(bbox);
        }
    }
    
    if (digit_rects.empty()) return -1;
    
    std::sort(digit_rects.begin(), digit_rects.end(), 
              [](const Rect& a, const Rect& b) { return a.x < b.x; });
    
    // Extract and recognize digits
    std::vector<int> recognized_digits;
    for (const Rect& rect : digit_rects) {
        if (recognized_digits.size() >= 3) break; // max 3 digits
        
        Mat digit_roi = thresh(rect);
        resize(digit_roi, digit_roi, Size(40, 60));
        
        // Template matching
        float best_score = -1.0f;
        int best_digit = -1;
        
        for (const auto& tmpl : digit_templates) {
            Mat result;
            matchTemplate(digit_roi, tmpl.image, result, TM_CCOEFF_NORMED);
            
            double min_val, max_val;
            minMaxLoc(result, &min_val, &max_val);
            
            if (max_val > best_score && max_val > 0.5) { // threshold for match quality
                best_score = static_cast<float>(max_val);
                best_digit = tmpl.digit;
            }
        }
        
        if (best_digit >= 0) {
            recognized_digits.push_back(best_digit);
        }
    }
    
    // Construct speed limit number
    if (recognized_digits.empty()) return -1;
    
    int speed_limit = 0;
    for (int digit : recognized_digits) {
        speed_limit = speed_limit * 10 + digit;
    }
    
    // Validate reasonable speed limit range
    if (speed_limit >= 30 && speed_limit <= 200) {
        return speed_limit;
    }
    
    return -1;
}

// Main detection function
extern "C" int opencv_detect_speed_signs(unsigned char* image_data, int width, int height, 
                                        int channels, speed_sign_detection_t* detections, 
                                        int max_detections) {
    if (!image_data || !detections || max_detections <= 0) {
        return 0;
    }
    
    try {
        // Create OpenCV Mat from image data
        Mat image;
        if (channels == 3) {
            image = Mat(height, width, CV_8UC3, image_data);
        } else if (channels == 4) {
            image = Mat(height, width, CV_8UC4, image_data);
            cvtColor(image, image, COLOR_BGRA2BGR);
        } else {
            return 0; // Unsupported format
        }
        
        // Convert to HSV for better color detection
        Mat hsv;
        cvtColor(image, hsv, COLOR_BGR2HSV);
        
        // Define red color range for speed signs (two ranges for red hue wrap-around)
        Mat red_mask1, red_mask2, red_mask;
        inRange(hsv, Scalar(0, 50, 50), Scalar(10, 255, 255), red_mask1);
        inRange(hsv, Scalar(170, 50, 50), Scalar(180, 255, 255), red_mask2);
        bitwise_or(red_mask1, red_mask2, red_mask);
        
        // Morphological operations to clean up the mask
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        morphologyEx(red_mask, red_mask, MORPH_CLOSE, kernel);
        morphologyEx(red_mask, red_mask, MORPH_OPEN, kernel);
        
        // Find contours
        std::vector<std::vector<Point>> contours;
        findContours(red_mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        
        int detection_count = 0;
        
        for (const auto& contour : contours) {
            if (detection_count >= max_detections) break;
            
            // Filter by area and circularity
            double area = contourArea(contour);
            if (area < 500 || area > 20000) continue; // reasonable sign size
            
            // Check circularity
            double perimeter = arcLength(contour, true);
            double circularity = 4 * M_PI * area / (perimeter * perimeter);
            if (circularity < 0.6) continue; // reasonably circular
            
            // Get bounding rectangle
            Rect bbox = boundingRect(contour);
            if (bbox.width < 50 || bbox.height < 50) continue; // minimum size
            
            // Aspect ratio check (should be roughly square)
            double aspect_ratio = static_cast<double>(bbox.width) / bbox.height;
            if (aspect_ratio < 0.7 || aspect_ratio > 1.3) continue;
            
            // Extract the sign region
            Mat sign_roi = image(bbox);
            
            // Attempt to extract speed limit
            int speed_limit = extract_speed_from_sign(sign_roi);
            if (speed_limit > 0) {
                // Fill detection structure
                detections[detection_count].x = bbox.x;
                detections[detection_count].y = bbox.y;
                detections[detection_count].width = bbox.width;
                detections[detection_count].height = bbox.height;
                detections[detection_count].speed_limit = speed_limit;
                detections[detection_count].confidence = 0.75f + (circularity * 0.25f); // base confidence + circularity bonus
                
                detection_count++;
            }
        }
        
        return detection_count;
        
    } catch (const cv::Exception& e) {
        printf("OpenCV exception in speed sign detection: %s\n", e.what());
        return 0;
    } catch (const std::exception& e) {
        printf("Standard exception in speed sign detection: %s\n", e.what());
        return 0;
    }
}

// Initialize OpenCV (call once at startup)
extern "C" int opencv_speed_detection_init(void) {
    try {
        init_digit_templates();
        return 1; // success
    } catch (const std::exception& e) {
        printf("Failed to initialize OpenCV speed detection: %s\n", e.what());
        return 0; // failure
    }
}

// Cleanup OpenCV (call at shutdown)
extern "C" void opencv_speed_detection_cleanup(void) {
    digit_templates.clear();
    templates_initialized = false;
}

#else

// Dummy implementations when OpenCV is not available
extern "C" {
    int opencv_detect_speed_signs(unsigned char* image_data, int width, int height, 
                                 int channels, speed_sign_detection_t* detections, 
                                 int max_detections) {
        (void)image_data; (void)width; (void)height; (void)channels;
        (void)detections; (void)max_detections;
        return 0; // No detections
    }
    
    int opencv_speed_detection_init(void) {
        return 1; // Always succeed
    }
    
    void opencv_speed_detection_cleanup(void) {
        // Nothing to do
    }
}

#endif // USE_OPENCV