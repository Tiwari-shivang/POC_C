# OpenCV Integration for Speed Governance POC

## Overview
This document defines the integration of **OpenCV** into the project POC.  
OpenCV will be used to analyze speed sign board images and extract the integer values (speed limits) displayed on them.  

---

## Assumptions
- OpenCV is **not installed** on the local machine initially.  
- OpenCV must be installed via a package manager (e.g., vcpkg, pip, or prebuilt binaries).  
- The POC is developed with **SDL2 simulator** and follows the MISRA-C compliant structure for the core governor.  

---

## Feature Description
1. **Speed Sign Recognition**  
   - Use OpenCV to detect circular/red-bordered speed sign boards.  
   - Extract the digit inside the sign (50 – 200 kmph range).  
   - Provide output as `{limit_kmh, confidence, timestamp}` to the governor logic.

2. **SDL2 Simulator Integration**  
   - A **random speed sign board** should be placed on the road.  
   - Each sign board will be sized **100px x 100px**.  
   - Speed limit displayed will be a random value between **50 kmph and 200 kmph**.  
   - A new sign board should appear every **10 seconds**.  

---

## Implementation Steps
1. **Install OpenCV**  
   - If using pip (Python prototyping):  
     ```bash
     pip install opencv-python
     ```  
   - If using C++ with vcpkg:  
     ```bash
     vcpkg install opencv[core,imgproc,imgcodecs,dnn]
     ```  

2. **Add Speed Sign Generator in SDL2**  
   - Generate a random integer in the range `[50, 200]`.  
   - Render a **100x100 px sign board** with that number.  
   - Place it at a random point along the simulated road.  
   - Refresh/replace the sign every **10 seconds**.  

3. **OpenCV Speed Sign Analyzer**  
   - Capture the SDL2 rendered frame or feed a test image.  
   - Detect circular regions with red borders.  
   - Apply digit recognition (either template matching or small trained classifier).  
   - Return the detected integer speed limit.  

4. **Governor Integration**  
   - Feed detected limit into existing **speed governor logic**.  
   - Ensure stale-sample and debounce rules are applied.  
   - If no detection is made, fallback to "no limit update".  

---

## Next Steps
- Provide this document to **Claude** for code generation.  
- Code deliverables will include:  
  - SDL2 random sign board generator.  
  - OpenCV detection pipeline.  
  - C API wrapper for MISRA-C integration.  
