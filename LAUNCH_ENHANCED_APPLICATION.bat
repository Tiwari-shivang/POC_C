@echo off
echo ============================================
echo Mercedes POC - ENHANCED with STOP Signs
echo ============================================
echo.

REM Check if enhanced application exists
if exist "car_poc\build_final\Release\car_poc.exe" (
    echo Enhanced application found! Launching Mercedes POC...
    echo.
) else (
    echo ERROR: Enhanced application not found!
    echo Please check if the build completed successfully.
    pause
    exit /b 1
)

echo NEW FEATURES IN THIS ENHANCED VERSION:
echo ====================================
echo   ✓ VISIBLE STOP SIGNS on the road (red octagonal)
echo   ✓ Enhanced SPEED LIMIT SIGNS (larger, more visible)
echo   ✓ Dashboard speed limit AUTO-SYNCS with road signs
echo   ✓ Real-time DETECTION ZONE visualization
echo   ✓ Signs detected when entering camera view center
echo   ✓ Green "DETECTED" indicators on processed signs
echo   ✓ Console shows: "Speed sign detected: XX km/h"
echo   ✓ STOP signs show "STOP sign detected" messages
echo.
echo CONTROLS:
echo =========
echo   Arrow Keys    : Control vehicle speed
echo   Q / P         : Manual speed limit override (if needed)
echo   H / L         : Temperature control  
echo   R             : Spawn pedestrian manually
echo   M             : Voice command test
echo   ESC or Close  : Exit application
echo.
echo HOW TO TEST THE ENHANCED FEATURES:
echo =================================
echo   1. Use UP ARROW to drive forward (speed >5 km/h)
echo   2. Watch for BOTH speed signs AND stop signs
echo   3. Speed signs: Round, red border, white background
echo   4. Stop signs: Octagonal, red background, white text
echo   5. Dashboard speed limit updates automatically
echo   6. Look for detection zone overlay (camera view)
echo   7. Green indicators show when signs are detected
echo   8. Check console for detection messages
echo.
echo Starting enhanced application...
echo ===============================

cd car_poc\build_final\Release
start car_poc.exe

echo.
echo ENHANCED APPLICATION LAUNCHED!
echo =============================
echo Watch for STOP signs (red octagons) and enhanced speed limit signs!
echo Dashboard will update automatically as you drive past speed signs.
echo Detection zone shows where the "camera" is looking for signs.
echo.
pause