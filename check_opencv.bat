@echo off
echo ====================================
echo OpenCV Installation Check
echo ====================================
echo.

echo Checking for OpenCV in system PATH...
where opencv_world* >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo ✓ OpenCV DLLs found in PATH
) else (
    echo ✗ OpenCV DLLs not found in PATH
)

echo.
echo Checking for common OpenCV installation locations...

set OPENCV_FOUND=0

REM Check common installation paths
if exist "C:\opencv\build\include\opencv2" (
    echo ✓ Found OpenCV at C:\opencv\build\
    set OPENCV_FOUND=1
    set OPENCV_PATH=C:\opencv\build
)

if exist "C:\Program Files\opencv\build\include\opencv2" (
    echo ✓ Found OpenCV at C:\Program Files\opencv\build\
    set OPENCV_FOUND=1
    set OPENCV_PATH=C:\Program Files\opencv\build
)

if exist "%USERPROFILE%\opencv\build\include\opencv2" (
    echo ✓ Found OpenCV at %USERPROFILE%\opencv\build\
    set OPENCV_FOUND=1
    set OPENCV_PATH=%USERPROFILE%\opencv\build
)

REM Check for vcpkg installation
if exist "C:\vcpkg\installed\x64-windows\include\opencv2" (
    echo ✓ Found OpenCV installed via vcpkg
    set OPENCV_FOUND=1
    set OPENCV_PATH=C:\vcpkg\installed\x64-windows
)

echo.
if %OPENCV_FOUND%==1 (
    echo RESULT: OpenCV installation detected!
    echo Path: %OPENCV_PATH%
    echo.
    echo To build with OpenCV support, ensure:
    echo 1. OpenCV DLLs are in PATH or build directory
    echo 2. Run: cmake -DUSE_OPENCV=ON -DOPENCV_DIR="%OPENCV_PATH%" ..
) else (
    echo RESULT: No OpenCV installation found
    echo.
    echo To install OpenCV:
    echo 1. Download from: https://opencv.org/releases/
    echo 2. Extract to C:\opencv
    echo 3. Add C:\opencv\build\x64\vc15\bin to PATH
    echo.
    echo Alternative - Install via vcpkg:
    echo   vcpkg install opencv[core,imgproc,imgcodecs]:x64-windows
)

echo.
echo ====================================
pause