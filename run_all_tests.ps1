# ============================================================================
# Mercedes POC - Comprehensive Test Suite Runner (PowerShell Version)
# ============================================================================
# This script runs all unit tests, MISRA compliance checks, and generates
# evaluation reports for the Mercedes POC automotive safety systems.
# ============================================================================

param(
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$SkipMISRA,
    [switch]$SkipCoverage,
    [switch]$SkipEval,
    [switch]$Verbose
)

# Set colors for output
$Host.UI.RawUI.ForegroundColor = "White"

function Write-Header {
    param([string]$Text)
    Write-Host "`n============================================================================" -ForegroundColor Cyan
    Write-Host "  $Text" -ForegroundColor Cyan
    Write-Host "============================================================================" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Text)
    Write-Host "[PASS] $Text" -ForegroundColor Green
}

function Write-Failure {
    param([string]$Text)
    Write-Host "[FAIL] $Text" -ForegroundColor Red
}

function Write-Warning {
    param([string]$Text)
    Write-Host "[WARNING] $Text" -ForegroundColor Yellow
}

function Write-Info {
    param([string]$Text)
    Write-Host "[INFO] $Text" -ForegroundColor White
}

# Start execution
Clear-Host
Write-Header "MERCEDES POC - COMPREHENSIVE TEST SUITE"
Write-Info "Starting test execution at $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"

# Set variables
$ProjectRoot = Get-Location
$CarPocDir = Join-Path $ProjectRoot "car_poc"
$BuildDir = Join-Path $CarPocDir "build"
$BuildEvalDir = Join-Path $CarPocDir "build_eval"
$ReportsDir = Join-Path $ProjectRoot "test_reports"
$MisraReportDir = Join-Path $ReportsDir "misra"
$CoverageReportDir = Join-Path $ReportsDir "coverage"
$EvalReportDir = Join-Path $ReportsDir "evaluation"

# Test results tracking
$TestResults = @{
    Passed = 0
    Failed = 0
    Skipped = 0
    Details = @()
}

# Create report directories
Write-Info "Creating report directories..."
@($ReportsDir, $MisraReportDir, $CoverageReportDir, $EvalReportDir) | ForEach-Object {
    if (!(Test-Path $_)) {
        New-Item -ItemType Directory -Path $_ -Force | Out-Null
    }
}

# ============================================================================
# SECTION 1: BUILD THE PROJECT
# ============================================================================
if (!$SkipBuild) {
    Write-Header "SECTION 1: BUILDING PROJECT"
    
    Set-Location $CarPocDir
    
    # Clean and rebuild
    Write-Info "Cleaning previous build..."
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
    Set-Location $BuildDir
    
    Write-Info "Configuring CMake..."
    $cmakeResult = & cmake .. -G "MinGW Makefiles" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Failure "CMake configuration failed!"
        if ($Verbose) { Write-Host $cmakeResult }
        exit 1
    }
    
    Write-Info "Compiling project..."
    $makeResult = & mingw32-make -j4 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Failure "Build failed!"
        if ($Verbose) { Write-Host $makeResult }
        exit 1
    }
    
    Write-Success "Build completed successfully!"
} else {
    Write-Warning "Skipping build phase"
}

# ============================================================================
# SECTION 2: RUN UNIT TESTS
# ============================================================================
if (!$SkipTests) {
    Write-Header "SECTION 2: RUNNING UNIT TESTS"
    
    Set-Location $BuildDir
    
    $tests = @(
        @{Name = "test_autobrake"; Description = "Auto Brake System"},
        @{Name = "test_wipers"; Description = "Windshield Wipers"},
        @{Name = "test_speedgov"; Description = "Speed Governor"},
        @{Name = "test_autopark"; Description = "Auto Parking"},
        @{Name = "test_climate"; Description = "Climate Control"}
    )
    
    foreach ($test in $tests) {
        $testExe = "$($test.Name).exe"
        if (Test-Path $testExe) {
            Write-Info "Running $($test.Description) test..."
            $testOutput = & ".\$testExe" 2>&1
            
            if ($LASTEXITCODE -eq 0) {
                Write-Success "$($test.Name) passed"
                $TestResults.Passed++
                $TestResults.Details += @{
                    Name = $test.Name
                    Description = $test.Description
                    Status = "PASSED"
                    Time = Get-Date -Format "HH:mm:ss"
                }
            } else {
                Write-Failure "$($test.Name) failed"
                $TestResults.Failed++
                $TestResults.Details += @{
                    Name = $test.Name
                    Description = $test.Description
                    Status = "FAILED"
                    Time = Get-Date -Format "HH:mm:ss"
                }
                if ($Verbose) { Write-Host $testOutput }
            }
        } else {
            Write-Warning "$testExe not found"
            $TestResults.Skipped++
        }
    }
    
    Write-Info "Test Summary: Passed: $($TestResults.Passed), Failed: $($TestResults.Failed), Skipped: $($TestResults.Skipped)"
} else {
    Write-Warning "Skipping unit tests"
}

# ============================================================================
# SECTION 3: RUN EVALUATION TESTS
# ============================================================================
if (!$SkipEval) {
    Write-Header "SECTION 3: RUNNING EVALUATION TESTS"
    
    Set-Location $CarPocDir
    
    Write-Info "Building evaluation tests..."
    if (Test-Path $BuildEvalDir) {
        Remove-Item -Recurse -Force $BuildEvalDir
    }
    New-Item -ItemType Directory -Path $BuildEvalDir -Force | Out-Null
    Set-Location $BuildEvalDir
    
    $cmakeResult = & cmake .. -G "MinGW Makefiles" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Release 2>&1
    if ($LASTEXITCODE -eq 0) {
        $makeResult = & mingw32-make test_autobrake_eval 2>&1
        if ($LASTEXITCODE -eq 0) {
            Write-Info "Running autobrake evaluation..."
            $evalResult = & .\test_autobrake_eval.exe 2>&1
            if ($LASTEXITCODE -eq 0) {
                Write-Success "Autobrake evaluation passed"
            } else {
                Write-Failure "Autobrake evaluation failed"
                if ($Verbose) { Write-Host $evalResult }
            }
        }
    } else {
        Write-Warning "Evaluation build failed"
    }
    
    # Copy evaluation reports
    $evalReports = Get-ChildItem -Path "$CarPocDir\eval\reports" -Include "*.json", "*.csv" -Recurse -ErrorAction SilentlyContinue
    if ($evalReports) {
        Write-Info "Copying evaluation reports..."
        $evalReports | Copy-Item -Destination $EvalReportDir -Force
    }
} else {
    Write-Warning "Skipping evaluation tests"
}

# ============================================================================
# SECTION 4: MISRA COMPLIANCE CHECK
# ============================================================================
if (!$SkipMISRA) {
    Write-Header "SECTION 4: RUNNING MISRA COMPLIANCE CHECKS"
    
    Set-Location $CarPocDir
    
    $cppcheckPath = $null
    if (Get-Command cppcheck -ErrorAction SilentlyContinue) {
        $cppcheckPath = "cppcheck"
    } elseif (Test-Path "C:\Program Files\Cppcheck\cppcheck.exe") {
        $cppcheckPath = "C:\Program Files\Cppcheck\cppcheck.exe"
    }
    
    if ($cppcheckPath) {
        Write-Info "Running MISRA C:2012 compliance check..."
        
        $misraReport = Join-Path $MisraReportDir "misra_report.txt"
        $misraXmlReport = Join-Path $MisraReportDir "misra_report.xml"
        
        # Text report
        & $cppcheckPath --enable=all --language=c --std=c99 --check-level=exhaustive `
            -I "$CarPocDir\inc" -I "$CarPocDir\cfg" -I "$CarPocDir\sim" `
            "$CarPocDir\src" `
            --suppress=missingIncludeSystem --inline-suppr `
            --output-file="$misraReport" 2>&1 | Out-Null
        
        # XML report
        & $cppcheckPath --enable=all --language=c --std=c99 --check-level=exhaustive `
            -I "$CarPocDir\inc" -I "$CarPocDir\cfg" -I "$CarPocDir\sim" `
            "$CarPocDir\src" `
            --suppress=missingIncludeSystem --inline-suppr `
            --xml --output-file="$misraXmlReport" 2>&1 | Out-Null
        
        if (Test-Path $misraReport) {
            Write-Success "MISRA reports generated"
            Write-Info "  Text report: $misraReport"
            Write-Info "  XML report: $misraXmlReport"
        }
    } else {
        Write-Warning "Cppcheck not found. Skipping MISRA analysis."
    }
} else {
    Write-Warning "Skipping MISRA compliance checks"
}

# ============================================================================
# SECTION 5: CODE COVERAGE REPORT
# ============================================================================
if (!$SkipCoverage) {
    Write-Header "SECTION 5: GENERATING CODE COVERAGE REPORT"
    
    Set-Location $CarPocDir
    
    if (Get-Command python -ErrorAction SilentlyContinue) {
        $coverageScript = Join-Path $CarPocDir "generate_html_coverage_report.py"
        if (Test-Path $coverageScript) {
            Write-Info "Generating HTML coverage report..."
            $pythonResult = & python $coverageScript 2>&1
            
            $coverageHtml = Join-Path $CarPocDir "test_coverage_report.html"
            if (Test-Path $coverageHtml) {
                $destination = Join-Path $CoverageReportDir "coverage_report.html"
                Move-Item -Path $coverageHtml -Destination $destination -Force
                Write-Success "HTML coverage report saved to: $destination"
            }
        }
    } else {
        Write-Warning "Python not found. Skipping HTML coverage report."
    }
} else {
    Write-Warning "Skipping coverage report generation"
}

# ============================================================================
# SECTION 6: GENERATE SUMMARY REPORT
# ============================================================================
Write-Header "SECTION 6: GENERATING SUMMARY REPORT"

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$summaryFile = Join-Path $ReportsDir "test_summary_$timestamp.txt"

$summary = @"
Mercedes POC - Test Execution Summary
========================================

Execution Date: $(Get-Date -Format 'yyyy-MM-dd')
Execution Time: $(Get-Date -Format 'HH:mm:ss')

Unit Test Results:
  - Tests Passed: $($TestResults.Passed)
  - Tests Failed: $($TestResults.Failed)
  - Tests Skipped: $($TestResults.Skipped)

Test Details:
"@

foreach ($test in $TestResults.Details) {
    $summary += "`n  - $($test.Name) ($($test.Description)): $($test.Status) at $($test.Time)"
}

$summary += @"

Reports Generated:
"@

if (Test-Path "$MisraReportDir\misra_report.txt") {
    $summary += "`n  - MISRA Compliance Report"
}
if (Test-Path "$CoverageReportDir\coverage_report.html") {
    $summary += "`n  - Code Coverage Report"
}
if ((Get-ChildItem -Path $EvalReportDir -Filter "*.json" -ErrorAction SilentlyContinue).Count -gt 0) {
    $summary += "`n  - Evaluation Reports"
}

$summary += @"

All reports saved in: $ReportsDir
"@

$summary | Out-File -FilePath $summaryFile -Encoding UTF8
Write-Success "Test summary saved to: $summaryFile"

# Generate JSON summary for programmatic access
$jsonSummary = @{
    ExecutionDate = Get-Date -Format 'yyyy-MM-dd'
    ExecutionTime = Get-Date -Format 'HH:mm:ss'
    TestResults = $TestResults
    ReportsGenerated = @{
        MISRA = Test-Path "$MisraReportDir\misra_report.txt"
        Coverage = Test-Path "$CoverageReportDir\coverage_report.html"
        Evaluation = (Get-ChildItem -Path $EvalReportDir -Filter "*.json" -ErrorAction SilentlyContinue).Count -gt 0
    }
    ReportsDirectory = $ReportsDir
} | ConvertTo-Json -Depth 3

$jsonSummaryFile = Join-Path $ReportsDir "test_summary_$timestamp.json"
$jsonSummary | Out-File -FilePath $jsonSummaryFile -Encoding UTF8
Write-Info "JSON summary saved to: $jsonSummaryFile"

# ============================================================================
# COMPLETION
# ============================================================================
Write-Header "TEST EXECUTION COMPLETED"

Write-Host @"

All reports have been generated in: $ReportsDir

Report Structure:
  $ReportsDir
  ├── misra\          (MISRA compliance reports)
  ├── coverage\       (Code coverage reports)
  ├── evaluation\     (Evaluation test reports)
  └── test_summary_*.txt/json (Execution summaries)

Execution completed at $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')

"@ -ForegroundColor Green

Set-Location $ProjectRoot

# Return exit code based on test results
if ($TestResults.Failed -gt 0) {
    exit 1
} else {
    exit 0
}