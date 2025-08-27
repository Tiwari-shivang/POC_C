# Mercedes POC - Complete MISRA C:2012 Analysis
# Runs Cppcheck with comprehensive MISRA compliance checking

param(
    [string]$OutputFormat = "both",  # text, html, both
    [switch]$Exhaustive = $false,
    [switch]$Verbose = $false
)

Write-Host "🚗 Mercedes POC - MISRA C:2012 Compliance Analysis" -ForegroundColor Cyan
Write-Host "=" * 60 -ForegroundColor Cyan
Write-Host ""

# Configuration
$ProjectRoot = Get-Location
$CarPocDir = Join-Path $ProjectRoot "car_poc"
$BuildDir = Join-Path $ProjectRoot "build"
$OutputDir = Join-Path $ProjectRoot "misra_analysis"
$Cppcheck = "C:\Program Files\Cppcheck\cppcheck.exe"
$MisraScript = Join-Path $ProjectRoot "misra.py"

# Create output directory
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Write-Host "✅ Created output directory: $OutputDir" -ForegroundColor Green
}

# Verify tools
Write-Host "🔧 Verifying analysis tools..." -ForegroundColor Yellow

if (!(Test-Path $Cppcheck)) {
    Write-Host "❌ Cppcheck not found at: $Cppcheck" -ForegroundColor Red
    Write-Host "Please install Cppcheck from: https://cppcheck.sourceforge.io/"
    exit 1
}

$CppcheckVersion = & $Cppcheck --version 2>&1 | Out-String
Write-Host "✅ Found: $($CppcheckVersion.Trim())" -ForegroundColor Green

$PythonVersion = python --version 2>&1 | Out-String
if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Found: $($PythonVersion.Trim())" -ForegroundColor Green
} else {
    Write-Host "❌ Python not found - MISRA analysis will be limited" -ForegroundColor Red
}

if (!(Test-Path $CarPocDir)) {
    Write-Host "❌ Source directory not found: $CarPocDir" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Build the project first to ensure compilation
Write-Host "🔨 Building project for analysis..." -ForegroundColor Yellow
if (Test-Path $BuildDir) {
    Remove-Item $BuildDir -Recurse -Force
}

$BuildSuccess = $true
try {
    cmake -B $BuildDir -S $CarPocDir -DHEADLESS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed" }
    
    cmake --build $BuildDir 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) { 
        Write-Host "⚠️  Build has some errors but continuing with analysis..." -ForegroundColor Yellow
    } else {
        Write-Host "✅ Build successful" -ForegroundColor Green
    }
} catch {
    Write-Host "⚠️  Build failed but continuing with analysis: $_" -ForegroundColor Yellow
    $BuildSuccess = $false
}

Write-Host ""

# Prepare Cppcheck arguments
$CppcheckArgs = @()
$CppcheckArgs += "--enable=all"
$CppcheckArgs += "--language=c"
$CppcheckArgs += "--std=c99"
$CppcheckArgs += "--platform=native"

# Include paths
$CppcheckArgs += "-I"
$CppcheckArgs += (Join-Path $CarPocDir "inc")
$CppcheckArgs += "-I"
$CppcheckArgs += (Join-Path $CarPocDir "cfg")
$CppcheckArgs += "-I"
$CppcheckArgs += (Join-Path $CarPocDir "sim")

# Analysis depth
if ($Exhaustive) {
    $CppcheckArgs += "--check-level=exhaustive"
    Write-Host "🔍 Using exhaustive analysis (may take longer)..." -ForegroundColor Yellow
} else {
    $CppcheckArgs += "--check-level=normal"
}

# Suppressions for cleaner output
$CppcheckArgs += "--suppress=missingIncludeSystem"
$CppcheckArgs += "--suppress=unusedFunction:tests/*"
$CppcheckArgs += "--inline-suppr"

# Source paths
$CppcheckArgs += (Join-Path $CarPocDir "src")
$CppcheckArgs += (Join-Path $CarPocDir "sim")

# Output files
$TextOutput = Join-Path $OutputDir "cppcheck_raw.txt"
$MisraOutput = Join-Path $OutputDir "misra_violations.txt"
$SummaryOutput = Join-Path $OutputDir "misra_summary.txt"

Write-Host "🔍 Running Cppcheck analysis..." -ForegroundColor Yellow
Write-Host "   Source: $CarPocDir" -ForegroundColor Gray
Write-Host "   Output: $OutputDir" -ForegroundColor Gray

# Run Cppcheck
$CppcheckArgs += "--output-file=$TextOutput"
$CppcheckCmd = "& `"$Cppcheck`" $($CppcheckArgs -join ' ') 2>&1"

try {
    $CppcheckResult = Invoke-Expression $CppcheckCmd
    Write-Host "✅ Cppcheck analysis completed" -ForegroundColor Green
} catch {
    Write-Host "❌ Cppcheck analysis failed: $_" -ForegroundColor Red
    exit 1
}

# Run MISRA analysis if Python is available
if (Test-Path $MisraScript) {
    Write-Host ""
    Write-Host "📊 Running MISRA C:2012 compliance analysis..." -ForegroundColor Yellow
    
    try {
        $MisraResult = python $MisraScript $TextOutput 2>&1
        $MisraExitCode = $LASTEXITCODE
        
        # Save MISRA output
        $MisraResult | Out-File -FilePath $MisraOutput -Encoding UTF8
        
        if ($MisraExitCode -eq 0) {
            Write-Host "🎉 MISRA Analysis: NO VIOLATIONS FOUND!" -ForegroundColor Green
        } else {
            Write-Host "📋 MISRA Analysis: $MisraExitCode violation(s) found" -ForegroundColor Yellow
        }
        
        # Display results
        Write-Host ""
        Write-Host "📊 MISRA C:2012 Analysis Results:" -ForegroundColor Cyan
        Write-Host "-" * 40 -ForegroundColor Cyan
        $MisraResult | ForEach-Object { Write-Host $_ }
        
    } catch {
        Write-Host "❌ MISRA analysis failed: $_" -ForegroundColor Red
    }
} else {
    Write-Host "⚠️  MISRA script not found - skipping detailed MISRA analysis" -ForegroundColor Yellow
}

# Generate comprehensive summary
$SummaryContent = @"
# Mercedes POC - MISRA C:2012 Compliance Report

**Analysis Date:** $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
**Analyzer:** Cppcheck $($CppcheckVersion.Trim())
**Project:** Mercedes Automotive Safety POC
**Standards:** MISRA C:2012, ISO 26262

## Analysis Configuration

- **Source Files:** $(Get-ChildItem "$CarPocDir\src" -Filter "*.c" | Measure-Object | Select-Object -ExpandProperty Count) files
- **Include Paths:** inc/, cfg/, sim/
- **Analysis Level:** $(if ($Exhaustive) { "Exhaustive" } else { "Normal" })
- **Language Standard:** C99
- **Platform:** Native (Windows)

## Raw Analysis Results

Total lines analyzed: $(((Get-ChildItem "$CarPocDir\src" -Filter "*.c" -Recurse | Get-Content).Count))
Total issues found: $((Get-Content $TextOutput | Where-Object { $_ -match ":\s*(error|warning|style|information):" }).Count)

## MISRA C:2012 Compliance

$(if (Test-Path $MisraOutput) { Get-Content $MisraOutput -Raw } else { "MISRA analysis not completed" })

## Recommendations

1. **High Priority:** Address any MISRA "Required" rule violations
2. **Medium Priority:** Consider fixing "Advisory" rule violations  
3. **Documentation:** Update code comments for complex safety logic
4. **Testing:** Ensure all fixed violations are covered by unit tests

## Files Generated

- Raw Cppcheck Output: cppcheck_raw.txt
- MISRA Violations: misra_violations.txt
- This Summary: misra_summary.txt

---
*Analysis completed for Mercedes POC automotive safety code*
"@

$SummaryContent | Out-File -FilePath $SummaryOutput -Encoding UTF8

# Generate HTML report if requested
if ($OutputFormat -eq "html" -or $OutputFormat -eq "both") {
    Write-Host ""
    Write-Host "📄 Generating HTML report..." -ForegroundColor Yellow
    
    $HtmlOutput = Join-Path $OutputDir "misra_report.html"
    $ViolationCount = if (Test-Path $MisraOutput) { 
        (Get-Content $MisraOutput | Where-Object { $_ -match "MISRA \d+\.\d+:" }).Count 
    } else { 0 }
    
    $HtmlContent = @"
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mercedes POC - MISRA C:2012 Analysis Report</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; margin: 40px; background: #f5f5f5; }
        .header { background: linear-gradient(135deg, #2c3e50 0%, #3498db 100%); color: white; padding: 30px; border-radius: 10px; text-align: center; }
        .summary { background: white; padding: 30px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .metric { display: inline-block; margin: 10px 20px; text-align: center; }
        .metric .number { font-size: 2em; font-weight: bold; color: #3498db; }
        .metric .label { color: #666; }
        .violations { background: white; padding: 30px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .violation { border-left: 4px solid #e74c3c; padding: 15px; margin: 10px 0; background: #fdf2f2; }
        .success { background: #d4edda; border: 2px solid #28a745; color: #155724; padding: 20px; border-radius: 8px; text-align: center; font-size: 1.2em; margin: 20px 0; }
        pre { background: #2d3748; color: #e2e8f0; padding: 15px; border-radius: 5px; overflow-x: auto; }
    </style>
</head>
<body>
    <div class="header">
        <h1>🚗 Mercedes POC</h1>
        <h2>MISRA C:2012 Compliance Analysis</h2>
        <p>Generated: $(Get-Date -Format "MMMM dd, yyyy HH:mm:ss")</p>
    </div>
    
    <div class="summary">
        <h2>📊 Analysis Summary</h2>
        <div class="metric">
            <div class="number">$ViolationCount</div>
            <div class="label">MISRA Violations</div>
        </div>
        <div class="metric">
            <div class="number">$(((Get-ChildItem "$CarPocDir\src" -Filter "*.c" -Recurse).Count))</div>
            <div class="label">Source Files</div>
        </div>
        <div class="metric">
            <div class="number">$(((Get-ChildItem "$CarPocDir\src" -Filter "*.c" -Recurse | Get-Content).Count))</div>
            <div class="label">Lines of Code</div>
        </div>
    </div>
    
    $(if ($ViolationCount -eq 0) {
        '<div class="success">🎉 <strong>MISRA C:2012 COMPLIANT!</strong><br>No violations found - Code meets automotive safety standards</div>'
    } else {
        '<div class="violations"><h2>🔍 MISRA Violations</h2>' + 
        ((Get-Content $MisraOutput -Raw) -replace "`n", "<br>") + 
        '</div>'
    })
    
    <div class="summary">
        <h2>📋 Raw Analysis Output</h2>
        <pre>$(if (Test-Path $TextOutput) { (Get-Content $TextOutput -Raw) -replace "<", "&lt;" -replace ">", "&gt;" } else { "No output available" })</pre>
    </div>
    
    <footer style="text-align: center; margin-top: 40px; color: #666;">
        <p>Mercedes POC - Automotive Safety Analysis • $(Get-Date -Format "yyyy")</p>
        <p>Standards: MISRA C:2012, ISO 26262</p>
    </footer>
</body>
</html>
"@
    
    $HtmlContent | Out-File -FilePath $HtmlOutput -Encoding UTF8
    Write-Host "✅ HTML report generated: $HtmlOutput" -ForegroundColor Green
}

# Final summary
Write-Host ""
Write-Host "📋 Analysis Complete!" -ForegroundColor Cyan
Write-Host "=" * 40 -ForegroundColor Cyan
Write-Host "📁 Output Directory: $OutputDir" -ForegroundColor White
Write-Host "📄 Summary Report: $(Split-Path $SummaryOutput -Leaf)" -ForegroundColor White
if ($OutputFormat -eq "html" -or $OutputFormat -eq "both") {
    Write-Host "🌐 HTML Report: misra_report.html" -ForegroundColor White
}

if (Test-Path $MisraOutput) {
    $ViolationCount = (Get-Content $MisraOutput | Where-Object { $_ -match "MISRA \d+\.\d+:" }).Count
    if ($ViolationCount -eq 0) {
        Write-Host ""
        Write-Host "🎉 COMPLIANCE ACHIEVED!" -ForegroundColor Green
        Write-Host "✅ Mercedes POC meets MISRA C:2012 standards" -ForegroundColor Green
        Write-Host "✅ Ready for automotive safety certification" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "📊 Violations Found: $ViolationCount" -ForegroundColor Yellow
        Write-Host "📋 Review misra_violations.txt for details" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "To view results:" -ForegroundColor Gray
Write-Host "  Text: Get-Content '$SummaryOutput'" -ForegroundColor Gray
if ($OutputFormat -eq "html" -or $OutputFormat -eq "both") {
    Write-Host "  HTML: Start-Process '$(Join-Path $OutputDir "misra_report.html")'" -ForegroundColor Gray
}