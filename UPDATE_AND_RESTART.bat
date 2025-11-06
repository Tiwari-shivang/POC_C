@echo off
echo ============================================
echo Mercedes POC - Update and Restart
echo ============================================
echo.

echo Stopping any running instances...
taskkill /f /im car_poc.exe >nul 2>&1

echo Waiting for process cleanup...
timeout /t 2 >nul

cd car_poc\build

echo Rebuilding with latest changes...
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed! Check error messages above.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful! Changes applied:
echo ========================================
echo   ✓ Dashboard speed limit syncs with road signs
echo   ✓ Signs detected when entering center of view  
echo   ✓ Green "DETECTED" indicator appears on signs
echo   ✓ Console messages show detection events
echo ========================================
echo.
echo Starting updated application...

cd Release
start car_poc.exe

echo.
echo UPDATED FEATURES:
echo =================
echo   • Drive forward (^>5 km/h) to spawn speed signs
echo   • Watch dashboard speed limit update automatically
echo   • Green indicators show when signs are "detected"
echo   • Console shows: "Speed sign detected: XX km/h"
echo.
echo Application restarted with dashboard sync!
pause