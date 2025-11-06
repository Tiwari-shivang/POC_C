@echo off
setlocal EnableDelayedExpansion

REM Windows batch script for running static analysis
REM Config
set "SRC_DIR=car_poc\src"
set "OUT_DIR=reports"
set "CPPCHECK_PATH=C:\Program Files\Cppcheck\cppcheck.exe"

REM Create output directory
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

REM Check if cppcheck exists
if not exist "%CPPCHECK_PATH%" (
    echo ERROR: Cppcheck not found at %CPPCHECK_PATH%
    echo Please install Cppcheck or update the path in this script
    exit /b 1
)

echo Running static analysis on safety-critical modules...

REM Run cppcheck on the safety-critical source files
"%CPPCHECK_PATH%" ^
    --enable=warning,style,performance,portability,information ^
    --inconclusive ^
    --language=c ^
    --std=c99 ^
    --platform=win64 ^
    --suppress=missingIncludeSystem ^
    --xml ^
    --xml-version=2 ^
    -I car_poc\inc ^
    -I car_poc\cfg ^
    "%SRC_DIR%\app_autobrake.c" ^
    "%SRC_DIR%\app_speedgov.c" ^
    "%SRC_DIR%\app_wipers.c" ^
    "%SRC_DIR%\app_autopark.c" ^
    "%SRC_DIR%\app_climate.c" ^
    2> "%OUT_DIR%\cppcheck.xml"

if %ERRORLEVEL% GEQ 1 (
    echo WARNING: Cppcheck reported some issues
)

REM Also generate a human-readable text report
"%CPPCHECK_PATH%" ^
    --enable=warning,style,performance,portability,information ^
    --inconclusive ^
    --language=c ^
    --std=c99 ^
    --platform=win64 ^
    --suppress=missingIncludeSystem ^
    -I car_poc\inc ^
    -I car_poc\cfg ^
    "%SRC_DIR%\app_autobrake.c" ^
    "%SRC_DIR%\app_speedgov.c" ^
    "%SRC_DIR%\app_wipers.c" ^
    "%SRC_DIR%\app_autopark.c" ^
    "%SRC_DIR%\app_climate.c" ^
    2> "%OUT_DIR%\cppcheck.txt"

echo.
echo Static analysis reports generated:
echo   - XML: %OUT_DIR%\cppcheck.xml
echo   - Text: %OUT_DIR%\cppcheck.txt
echo.

endlocal