@echo off
echo ============================================
echo Mercedes POC - Automotive Safety Simulator
echo ============================================
echo.

REM Check if application exists
if exist "car_poc\build\Release\car_poc.exe" (
    echo Application found! Launching Mercedes POC...
    echo.
) else (
    echo ERROR: Application not found!
    echo Please run rebuild_and_run.bat first to build the project.
    pause
    exit /b 1
)

echo CONTROLS:
echo =========
echo   Arrow Keys    : Control vehicle speed
echo   Q / P         : Adjust speed limit manually (+5/-5 km/h)
echo   H / L         : Temperature control
echo   R             : Spawn pedestrian manually
echo   M             : Voice command test
echo   ESC or Close  : Exit application
echo.
echo FEATURES:
echo =========
echo   - Real-time speed and RPM gauges
echo   - Automatic rain simulation
echo   - Pedestrian collision detection
echo   - Auto-brake system (at 4 feet)
echo   - Gap detection alerts (at 6 feet) 
echo   - Windshield wipers (based on rain level)
echo   - Speed governor with alarm
echo   - Climate control system
echo   - Speed limit signs (spawn every 10-15 seconds)
if exist "car_poc\build\Release\car_poc.exe" (
    echo   - OpenCV integration: DISABLED (manual controls only)
)
echo.
echo Starting in 3 seconds...
timeout /t 3 >nul

cd car_poc\build\Release
start car_poc.exe

echo.
echo Application launched in new window!
echo Drive forward (speed ^>5 km/h) to see signs and features.
echo Close this window when done.
echo.
pause