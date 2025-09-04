@echo off
REM ============================================================================
REM Mercedes POC - Comprehensive Test Suite Runner
REM ============================================================================
REM This script runs all unit tests, MISRA compliance checks, and generates
REM evaluation reports for the Mercedes POC automotive safety systems.
REM ============================================================================

echo.
echo ============================================================================
echo                    MERCEDES POC - COMPREHENSIVE TEST SUITE
echo ============================================================================
echo.
echo Starting test execution at %DATE% %TIME%
echo.

REM Set variables
set PROJECT_ROOT=%CD%
set CAR_POC_DIR=%PROJECT_ROOT%\car_poc
set BUILD_DIR=%CAR_POC_DIR%\build
set BUILD_EVAL_DIR=%CAR_POC_DIR%\build_eval
set REPORTS_DIR=%PROJECT_ROOT%\test_reports
set MISRA_REPORT_DIR=%REPORTS_DIR%\misra
set COVERAGE_REPORT_DIR=%REPORTS_DIR%\coverage
set EVAL_REPORT_DIR=%REPORTS_DIR%\evaluation

REM Create reports directory structure
echo [SETUP] Creating report directories...
if not exist "%REPORTS_DIR%" mkdir "%REPORTS_DIR%"
if not exist "%MISRA_REPORT_DIR%" mkdir "%MISRA_REPORT_DIR%"
if not exist "%COVERAGE_REPORT_DIR%" mkdir "%COVERAGE_REPORT_DIR%"
if not exist "%EVAL_REPORT_DIR%" mkdir "%EVAL_REPORT_DIR%"

REM ============================================================================
REM SECTION 1: BUILD THE PROJECT
REM ============================================================================
echo.
echo ============================================================================
echo SECTION 1: BUILDING PROJECT
echo ============================================================================
echo.

cd "%CAR_POC_DIR%"

REM Clean and rebuild
echo [BUILD] Cleaning previous build...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo [BUILD] Configuring CMake...
cmake .. -G "MinGW Makefiles" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    goto :error_exit
)

echo [BUILD] Compiling project...
mingw32-make -j4
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    goto :error_exit
)

echo [BUILD] Build completed successfully!

REM ============================================================================
REM SECTION 2: RUN UNIT TESTS
REM ============================================================================
echo.
echo ============================================================================
echo SECTION 2: RUNNING UNIT TESTS
echo ============================================================================
echo.

cd "%BUILD_DIR%"
set TESTS_PASSED=0
set TESTS_FAILED=0

echo [TEST] Running test_autobrake...
test_autobrake.exe
if %errorlevel% equ 0 (
    echo [PASS] test_autobrake passed
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL] test_autobrake failed
    set /a TESTS_FAILED+=1
)

echo.
echo [TEST] Running test_wipers...
test_wipers.exe
if %errorlevel% equ 0 (
    echo [PASS] test_wipers passed
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL] test_wipers failed
    set /a TESTS_FAILED+=1
)

echo.
echo [TEST] Running test_speedgov...
test_speedgov.exe
if %errorlevel% equ 0 (
    echo [PASS] test_speedgov passed
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL] test_speedgov failed
    set /a TESTS_FAILED+=1
)

echo.
echo [TEST] Running test_autopark...
test_autopark.exe
if %errorlevel% equ 0 (
    echo [PASS] test_autopark passed
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL] test_autopark failed
    set /a TESTS_FAILED+=1
)

echo.
echo [TEST] Running test_climate...
test_climate.exe
if %errorlevel% equ 0 (
    echo [PASS] test_climate passed
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL] test_climate failed
    set /a TESTS_FAILED+=1
)

echo.
echo [TEST SUMMARY] Tests Passed: %TESTS_PASSED%, Tests Failed: %TESTS_FAILED%

REM ============================================================================
REM SECTION 3: RUN EVALUATION TESTS
REM ============================================================================
echo.
echo ============================================================================
echo SECTION 3: RUNNING EVALUATION TESTS
echo ============================================================================
echo.

cd "%CAR_POC_DIR%"

REM Build evaluation tests
echo [EVAL] Building evaluation tests...
if exist "%BUILD_EVAL_DIR%" rmdir /s /q "%BUILD_EVAL_DIR%"
mkdir "%BUILD_EVAL_DIR%"
cd "%BUILD_EVAL_DIR%"

cmake .. -G "MinGW Makefiles" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [WARNING] Evaluation build configuration failed
) else (
    mingw32-make test_autobrake_eval
    if %errorlevel% equ 0 (
        echo [EVAL] Running autobrake evaluation...
        test_autobrake_eval.exe
        if %errorlevel% equ 0 (
            echo [PASS] Autobrake evaluation passed
        ) else (
            echo [FAIL] Autobrake evaluation failed
        )
    )
)

REM Copy evaluation reports if they exist
if exist "%CAR_POC_DIR%\eval\reports\*.json" (
    echo [EVAL] Copying evaluation reports...
    xcopy /Y "%CAR_POC_DIR%\eval\reports\*.json" "%EVAL_REPORT_DIR%\" >nul
)
if exist "%CAR_POC_DIR%\eval\reports\*.csv" (
    xcopy /Y "%CAR_POC_DIR%\eval\reports\*.csv" "%EVAL_REPORT_DIR%\" >nul
)

REM ============================================================================
REM SECTION 4: MISRA COMPLIANCE CHECK
REM ============================================================================
echo.
echo ============================================================================
echo SECTION 4: RUNNING MISRA COMPLIANCE CHECKS
echo ============================================================================
echo.

cd "%CAR_POC_DIR%"

REM Check if Cppcheck is installed
where cppcheck >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Cppcheck not found in PATH. Checking default location...
    if exist "C:\Program Files\Cppcheck\cppcheck.exe" (
        set CPPCHECK="C:\Program Files\Cppcheck\cppcheck.exe"
    ) else (
        echo [SKIP] Cppcheck not installed. Skipping MISRA analysis.
        goto :skip_misra
    )
) else (
    set CPPCHECK=cppcheck
)

echo [MISRA] Running MISRA C:2012 compliance check...
%CPPCHECK% --enable=all --language=c --std=c99 --check-level=exhaustive ^
    -I "%CAR_POC_DIR%\inc" -I "%CAR_POC_DIR%\cfg" -I "%CAR_POC_DIR%\sim" ^
    "%CAR_POC_DIR%\src" ^
    --suppress=missingIncludeSystem --inline-suppr ^
    --output-file="%MISRA_REPORT_DIR%\misra_report.txt" 2>&1

if exist "%MISRA_REPORT_DIR%\misra_report.txt" (
    echo [MISRA] MISRA report generated: %MISRA_REPORT_DIR%\misra_report.txt
    
    REM Generate XML report for better formatting
    %CPPCHECK% --enable=all --language=c --std=c99 --check-level=exhaustive ^
        -I "%CAR_POC_DIR%\inc" -I "%CAR_POC_DIR%\cfg" -I "%CAR_POC_DIR%\sim" ^
        "%CAR_POC_DIR%\src" ^
        --suppress=missingIncludeSystem --inline-suppr ^
        --xml --output-file="%MISRA_REPORT_DIR%\misra_report.xml" 2>&1
)

:skip_misra

REM ============================================================================
REM SECTION 5: CODE COVERAGE REPORT
REM ============================================================================
echo.
echo ============================================================================
echo SECTION 5: GENERATING CODE COVERAGE REPORT
echo ============================================================================
echo.

cd "%CAR_POC_DIR%"

REM Check if Python is available for HTML report generation
where python >nul 2>&1
if %errorlevel% equ 0 (
    if exist "%CAR_POC_DIR%\generate_html_coverage_report.py" (
        echo [COVERAGE] Generating HTML coverage report...
        python "%CAR_POC_DIR%\generate_html_coverage_report.py"
        if exist "%CAR_POC_DIR%\test_coverage_report.html" (
            move /Y "%CAR_POC_DIR%\test_coverage_report.html" "%COVERAGE_REPORT_DIR%\coverage_report.html" >nul
            echo [COVERAGE] HTML report saved to: %COVERAGE_REPORT_DIR%\coverage_report.html
        )
    )
) else (
    echo [WARNING] Python not found. Skipping HTML coverage report generation.
)

REM ============================================================================
REM SECTION 6: GENERATE SUMMARY REPORT
REM ============================================================================
echo.
echo ============================================================================
echo SECTION 6: GENERATING SUMMARY REPORT
echo ============================================================================
echo.

set SUMMARY_FILE=%REPORTS_DIR%\test_summary_%DATE:~-4%%DATE:~4,2%%DATE:~7,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.txt
set SUMMARY_FILE=%SUMMARY_FILE: =0%

echo Mercedes POC - Test Execution Summary > "%SUMMARY_FILE%"
echo ======================================== >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"
echo Execution Date: %DATE% >> "%SUMMARY_FILE%"
echo Execution Time: %TIME% >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"
echo Unit Test Results: >> "%SUMMARY_FILE%"
echo   - Tests Passed: %TESTS_PASSED% >> "%SUMMARY_FILE%"
echo   - Tests Failed: %TESTS_FAILED% >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"
echo Reports Generated: >> "%SUMMARY_FILE%"
if exist "%MISRA_REPORT_DIR%\misra_report.txt" echo   - MISRA Compliance Report >> "%SUMMARY_FILE%"
if exist "%COVERAGE_REPORT_DIR%\coverage_report.html" echo   - Code Coverage Report >> "%SUMMARY_FILE%"
if exist "%EVAL_REPORT_DIR%\*.json" echo   - Evaluation Reports >> "%SUMMARY_FILE%"
echo. >> "%SUMMARY_FILE%"
echo All reports saved in: %REPORTS_DIR% >> "%SUMMARY_FILE%"

echo [SUMMARY] Test summary saved to: %SUMMARY_FILE%

REM ============================================================================
REM COMPLETION
REM ============================================================================
echo.
echo ============================================================================
echo                          TEST EXECUTION COMPLETED
echo ============================================================================
echo.
echo All reports have been generated in: %REPORTS_DIR%
echo.
echo Report Structure:
echo   %REPORTS_DIR%
echo   ├── misra\          (MISRA compliance reports)
echo   ├── coverage\       (Code coverage reports)
echo   ├── evaluation\     (Evaluation test reports)
echo   └── test_summary_*.txt (Execution summary)
echo.
echo Execution completed at %DATE% %TIME%
echo.

cd "%PROJECT_ROOT%"
exit /b 0

:error_exit
echo.
echo [ERROR] Test execution failed. Please check the error messages above.
cd "%PROJECT_ROOT%"
exit /b 1