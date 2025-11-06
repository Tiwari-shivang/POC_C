@echo off
echo ============================================
echo Mercedes POC - UPDATED with Dashboard Sync
echo ============================================
echo.

REM Check if updated application exists
if exist "car_poc\build_updated\Release\car_poc.exe" (
    echo Updated application found! Launching Mercedes POC...
    echo.
) else (
    echo ERROR: Updated application not found!
    echo Please check if the build completed successfully.
    pause
    exit /b 1
)

echo NEW FEATURES IN THIS UPDATE:
echo ============================
echo   ✓ Dashboard speed limit AUTO-SYNCS with road signs
echo   ✓ Signs are "detected" when they reach center of view
echo   ✓ Green "DETECTED" indicator appears on processed signs
echo   ✓ Console messages show: "Speed sign detected: XX km/h"
echo   ✓ No manual Q/P adjustment needed - fully automatic!
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
echo HOW TO TEST THE NEW FEATURE:
echo ============================
echo   1. Use UP ARROW to drive forward (speed ^>5 km/h)
echo   2. Wait for speed signs to appear (every 10-15 seconds)
echo   3. Watch the dashboard SPEED LIMIT update automatically
echo   4. Look for GREEN "DETECTED" indicators on signs
echo   5. Check console for detection messages
echo.
echo Starting updated application...
echo ==============================

cd car_poc\build_updated\Release
start car_poc.exe

echo.
echo Updated application launched with DASHBOARD SYNC!
echo Watch the speed limit in the bottom right update automatically
echo as you drive past the speed limit signs on the road.
echo.
pause