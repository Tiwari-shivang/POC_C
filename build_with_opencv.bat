@echo off
echo Building Mercedes POC with OpenCV Integration
echo ==============================================

REM Check if OpenCV is installed
where opencv_version > NUL 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Warning: OpenCV not found in PATH
    echo You may need to install OpenCV manually
)

REM Create build directory
if exist build_opencv rmdir /s /q build_opencv
mkdir build_opencv
cd build_opencv

echo.
echo Configuring with CMake...
echo --------------------------
cmake -DUSE_OPENCV=ON -DHEADLESS=OFF .. 

if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed
    echo Please ensure OpenCV is properly installed
    pause
    exit /b 1
)

echo.
echo Building project...
echo -------------------
cmake --build . --config Release

if %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed
    pause
    exit /b 1
)

echo.
echo Build completed successfully!
echo ============================
echo.
echo To run with OpenCV integration:
echo   cd build_opencv\Release
echo   car_poc.exe
echo.
echo To run tests:
echo   ctest -C Release
echo.
echo OpenCV speed sign detection is now integrated!

cd ..