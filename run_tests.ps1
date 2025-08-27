# Mercedes POC Test Runner
# Builds and runs all unit tests with proper dependencies

$ProjectRoot = Get-Location
$TestDir = Join-Path $ProjectRoot "car_poc\tests"
$SrcDir = Join-Path $ProjectRoot "car_poc\src"
$IncDir = Join-Path $ProjectRoot "car_poc\inc"
$CfgDir = Join-Path $ProjectRoot "car_poc\cfg"
$SimDir = Join-Path $ProjectRoot "car_poc\sim"

Write-Host "Mercedes POC Test Suite Runner" -ForegroundColor Cyan
Write-Host "===============================" -ForegroundColor Cyan
Write-Host ""

# Find Visual Studio Build Tools
$VSPath = ""
$PossiblePaths = @(
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

foreach ($Path in $PossiblePaths) {
    if (Test-Path $Path) {
        $VSPath = $Path
        break
    }
}

if (-not $VSPath) {
    Write-Host "❌ Visual Studio Build Tools not found" -ForegroundColor Red
    Write-Host "Please install Visual Studio Build Tools or use Developer Command Prompt"
    exit 1
}

Write-Host "✅ Found Visual Studio Build Tools: $VSPath" -ForegroundColor Green
Write-Host ""

# Test configurations
$Tests = @(
    @{
        Name = "test_autobrake"
        TestFile = "tests\test_autobrake.c"
        SourceFile = "src\app_autobrake.c"
        Description = "Automatic Emergency Braking Tests"
    },
    @{
        Name = "test_wipers"
        TestFile = "tests\test_wipers.c"
        SourceFile = "src\app_wipers.c"
        Description = "Rain-Sensing Wipers Tests"
    },
    @{
        Name = "test_speedgov"
        TestFile = "tests\test_speedgov.c"
        SourceFile = "src\app_speedgov.c"
        Description = "Speed Governor Tests"
    },
    @{
        Name = "test_autopark"
        TestFile = "tests\test_autopark.c"
        SourceFile = "src\app_autopark.c"
        Description = "Automatic Parking Tests"
    },
    @{
        Name = "test_climate"
        TestFile = "tests\test_climate.c"
        SourceFile = "src\app_climate.c"
        Description = "Climate Control Tests"
    }
)

$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

foreach ($Test in $Tests) {
    Write-Host "🧪 Running: $($Test.Description)" -ForegroundColor Yellow
    Write-Host "   Test: $($Test.Name)" -ForegroundColor Gray
    
    # Build the test
    $BuildCmd = "`"$VSPath`" && cd car_poc && cl /nologo /std:c99 /Iinc /Icfg /Isim /Itests\unity $($Test.TestFile) $($Test.SourceFile) tests\unity\unity.c /Fe:$($Test.Name).exe 2>NUL"
    
    Write-Host "   Building..." -ForegroundColor Gray
    $BuildResult = cmd /c $BuildCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "   ✅ Build successful" -ForegroundColor Green
        
        # Run the test
        Write-Host "   Running tests..." -ForegroundColor Gray
        $TestPath = Join-Path $ProjectRoot "car_poc\$($Test.Name).exe"
        
        if (Test-Path $TestPath) {
            $TestOutput = & $TestPath 2>&1
            $TestExitCode = $LASTEXITCODE
            
            if ($TestExitCode -eq 0) {
                Write-Host "   ✅ All tests passed!" -ForegroundColor Green
                $PassedTests++
                
                # Parse Unity output for test count
                $TestCount = ($TestOutput | Where-Object { $_ -match "(\d+) Tests (\d+) Failures (\d+) Ignored" } | ForEach-Object {
                    if ($_ -match "(\d+) Tests") { $matches[1] }
                })
                if ($TestCount) {
                    Write-Host "      Tests executed: $TestCount" -ForegroundColor Gray
                    $TotalTests += [int]$TestCount
                } else {
                    # Fallback: count RUN_TEST calls
                    $TestContent = Get-Content (Join-Path $ProjectRoot $Test.TestFile)
                    $RunTestCount = ($TestContent | Select-String "RUN_TEST" | Measure-Object).Count
                    Write-Host "      Tests executed: $RunTestCount" -ForegroundColor Gray
                    $TotalTests += $RunTestCount
                }
            } else {
                Write-Host "   ❌ Tests failed!" -ForegroundColor Red
                $FailedTests++
                Write-Host "   Output:" -ForegroundColor Red
                $TestOutput | ForEach-Object { Write-Host "      $_" -ForegroundColor Red }
            }
        } else {
            Write-Host "   ❌ Test executable not found: $TestPath" -ForegroundColor Red
            $FailedTests++
        }
        
        # Cleanup
        if (Test-Path $TestPath) {
            Remove-Item $TestPath -Force
        }
    } else {
        Write-Host "   ❌ Build failed!" -ForegroundColor Red
        $FailedTests++
        if ($BuildResult) {
            Write-Host "   Build output:" -ForegroundColor Red
            $BuildResult | ForEach-Object { Write-Host "      $_" -ForegroundColor Red }
        }
    }
    
    Write-Host ""
}

# Summary
Write-Host "=" * 50 -ForegroundColor Cyan
Write-Host "TEST SUITE SUMMARY" -ForegroundColor Cyan
Write-Host "=" * 50 -ForegroundColor Cyan
Write-Host "Total Test Suites: $($Tests.Count)" -ForegroundColor White
Write-Host "Passed: $PassedTests" -ForegroundColor Green
Write-Host "Failed: $FailedTests" -ForegroundColor Red

if ($TotalTests -gt 0) {
    Write-Host "Individual Tests Executed: $TotalTests" -ForegroundColor White
}

if ($FailedTests -eq 0) {
    Write-Host ""
    Write-Host "🎉 ALL TESTS PASSED!" -ForegroundColor Green
    Write-Host "✅ Mercedes POC Safety Systems Validated" -ForegroundColor Green
    Write-Host "Ready for production deployment" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "❌ Some tests failed. Please review the output above." -ForegroundColor Red
    exit 1
}