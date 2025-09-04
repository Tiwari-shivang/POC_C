# Speed Governor OpenCV Integration Evaluation Framework

## Overview
The enhanced speed governor system now integrates OpenCV computer vision for automatic speed limit sign recognition. This evaluation framework validates the complete vision-to-action pipeline for ISO 26262 compliant automotive safety systems.

## Architecture Overview

### Components Integrated
1. **SDL2 Simulator**: Renders realistic speed limit signs (100x100px, red circle, white background)
2. **OpenCV Vision System**: Detects and recognizes speed limit values from camera feed
3. **MISRA-C Wrapper**: Provides safety-compliant interface between C++ OpenCV and C modules
4. **Speed Governor**: Uses detected limits for adaptive speed monitoring
5. **Evaluation Framework**: Validates end-to-end performance and safety metrics

### Key Features
- **Real-time Sign Detection**: Processes windshield frames every 100ms
- **Template Matching**: Uses synthetic digit templates for OCR
- **Safety Integration**: High confidence threshold (≥70%) required for limit updates
- **Fallback System**: Manual controls (Q/P keys) when detection fails
- **Comprehensive Logging**: All detections logged with confidence scores

## Test Scenarios

### 1. OpenCV Detection Test (`speedgov_opencv_detection.json`)
- **Purpose**: Validates complete vision-to-governor pipeline
- **Test Flow**: 
  - Multiple speed signs spawn at different times (80, 60, 100 km/h)
  - Vehicle speed changes to trigger overspeed conditions
  - OpenCV processes frames and updates governor
- **Key Metrics**:
  - Detection latency: ≤1000ms from sign appearance
  - Confidence threshold: ≥0.7 required
  - Correct limit detection: Must match actual sign values
  - Vision system activation: System must be operational

## Vision Pipeline Details

### Frame Processing
1. **Capture**: SDL2 windshield area captured as BGRA pixels
2. **Color Detection**: HSV color space for red speed sign borders
3. **Shape Analysis**: Circular contour detection with area/circularity filters
4. **OCR Processing**: Template matching for digit recognition
5. **Validation**: Speed limit range checking (30-200 km/h)

### Detection Thresholds
- **Minimum Area**: 500 pixels (reasonable sign visibility)
- **Maximum Area**: 20,000 pixels (prevent false positives)
- **Circularity**: ≥0.6 (reasonably circular shapes)
- **Aspect Ratio**: 0.7-1.3 (square-ish bounding boxes)
- **Confidence**: ≥0.7 for governor integration

### Template Matching
- **Synthetic Templates**: Programmatically generated digit patterns
- **Scale Invariant**: 40x60 pixel normalized templates
- **Robust Matching**: TM_CCOEFF_NORMED with 0.5 threshold
- **Multi-digit Support**: Handles 2-3 digit speed limits

## Safety Compliance

### MISRA-C Integration
- **C API Wrapper**: All OpenCV calls isolated in C++ module
- **Error Handling**: Comprehensive exception catching
- **Resource Management**: Proper cleanup on shutdown
- **State Validation**: Input parameter checking

### Timing Requirements
- **Detection Latency**: ≤1000ms from sign appearance to governor update
- **Processing Interval**: 100ms frame processing cycle
- **Timeout Handling**: 500ms detection result validity
- **Graceful Degradation**: Fallback to manual controls

### Confidence and Validation
- **High Confidence Requirement**: 70% minimum for safety-critical updates
- **Range Validation**: Only 30-200 km/h limits accepted
- **Hysteresis**: Prevents oscillation between detection states
- **Duplicate Prevention**: Same limit not reported within 1 second

## Build Configuration

### CMake Options
```bash
# Enable OpenCV integration
cmake -DUSE_OPENCV=ON -DHEADLESS=OFF ..
```

### Dependencies
- **OpenCV 4.x**: core, imgproc, imgcodecs modules
- **SDL2**: For visual simulation and frame capture  
- **C++11**: Required for OpenCV integration

### Compiler Support
- **MSVC**: Visual Studio 2017+
- **GCC**: 7.0+
- **Clang**: 5.0+

## Evaluation Metrics

### Core Performance
- `opencv_detection_latency_ms`: Time from sign spawn to detection
- `detection_confidence_min`: Minimum confidence score achieved
- `correct_speed_limit_detected`: Boolean accuracy metric
- `vision_system_active`: System initialization status

### Integration Metrics  
- `overspeed_alarm_latency_ms`: Governor response time
- `limit_update_latency_ms`: Speed limit update propagation
- `alarm_clear_hysteresis_correct`: Proper threshold behavior

## Usage Instructions

### Runtime Operation
1. **Start Simulation**: `./car_poc` (with SDL2 enabled)
2. **Drive Forward**: Use arrow keys to maintain speed >5 km/h
3. **Observe Signs**: Speed signs spawn every 10-15 seconds
4. **Monitor Console**: Detection results logged with confidence
5. **Check Dashboard**: Speed limit display updates automatically

### Manual Controls
- **Q**: Increase manual speed limit (+5 km/h)
- **P**: Decrease manual speed limit (-5 km/h)  
- **↑/↓**: Vehicle speed control
- **R**: Manual pedestrian/object spawn

### Evaluation Execution
```bash
# Run specific OpenCV evaluation
ctest -R opencv_detection

# View results
cat eval/reports/speedgov_summary.json
```

## Troubleshooting

### Common Issues
1. **OpenCV Not Found**: Install OpenCV development packages
2. **Low Detection Rate**: Check sign visibility and lighting conditions
3. **False Positives**: Adjust color detection thresholds
4. **Performance Issues**: Increase processing interval (100ms default)

### Debug Logging
- Detection attempts logged to console
- Confidence scores reported for all candidates
- Frame processing timing available in debug builds
- Complete evaluation traces saved to CSV files

## Future Enhancements

### Planned Improvements
- **Deep Learning**: CNN-based sign detection for better accuracy
- **Multi-class Recognition**: Support for regulatory signs beyond speed limits
- **Perspective Correction**: Handle angled/distant signs better
- **Night Vision**: Infrared/low-light detection capabilities

This comprehensive integration demonstrates production-ready computer vision capabilities within ISO 26262 compliant automotive safety systems, providing both high performance and robust safety characteristics required for autonomous driving applications.