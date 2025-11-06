@echo off
echo ============================================
echo Mercedes POC - Complete Rebuild with OpenCV
echo ============================================
echo.

REM Clean previous builds
echo Cleaning previous builds...
if exist build rmdir /s /q build
if exist build_opencv rmdir /s /q build_opencv
echo Previous builds cleaned.
echo.

REM Create new build directory
echo Creating new build directory...
mkdir build
cd build

REM Configure with CMake (with OpenCV if available)
echo Configuring CMake...
echo --------------------------

REM Try to build with OpenCV first
cmake -DUSE_OPENCV=ON -DHEADLESS=OFF .. >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Configuration successful with OpenCV support!
    set OPENCV_ENABLED=1
) else (
    echo OpenCV not found, building without computer vision support
    cmake -DUSE_OPENCV=OFF -DHEADLESS=OFF ..
    set OPENCV_ENABLED=0
)

echo.
echo Building project...
echo -------------------
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed! Trying Debug configuration...
    cmake --build . --config Debug
    
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo ERROR: Build failed in both Release and Debug modes
        echo Please check the error messages above
        pause
        exit /b 1
    )
    
    set BUILD_TYPE=Debug
) else (
    set BUILD_TYPE=Release
)

echo.
echo ========================================
echo Build completed successfully!
echo Build type: %BUILD_TYPE%
if %OPENCV_ENABLED%==1 (
    echo OpenCV: ENABLED - Speed sign detection active!
) else (
    echo OpenCV: DISABLED - Manual speed limit control only
)
echo ========================================
echo.

REM Copy SDL2.dll if needed
if exist %BUILD_TYPE%\SDL2.dll (
    echo SDL2.dll found in build directory
) else (
    if exist ..\SDL2.dll (
        echo Copying SDL2.dll to build directory...
        copy ..\SDL2.dll %BUILD_TYPE%\
    )
)

echo Starting Mercedes POC Simulator...
echo ==================================
echo.
echo CONTROLS:
echo   Arrow Keys: Control vehicle speed
echo   Q/P: Manual speed limit adjustment
echo   H/L: Temperature control
echo   R: Spawn pedestrian
echo   M: Voice command
echo   ESC: Exit
echo.
if %OPENCV_ENABLED%==1 (
    echo OPENCV FEATURES:
    echo   - Speed signs spawn automatically every 10-15 seconds
    echo   - Drive at ^>5 km/h to see signs appear
    echo   - Detected limits update automatically
    echo   - Check console for detection confidence
    echo.
)
echo Starting application...
echo -----------------------

cd %BUILD_TYPE%
start car_poc.exe

cd ..\..\

echo.
echo Application launched in new window!
echo.
echo To run tests: cd build ^&^& ctest -C %BUILD_TYPE%
echo.
pause