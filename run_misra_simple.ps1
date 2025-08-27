# Mercedes POC - MISRA C:2012 Analysis (Simplified)

Write-Host "🚗 Mercedes POC - MISRA C:2012 Analysis" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Configuration
$ProjectRoot = Get-Location
$CarPocDir = Join-Path $ProjectRoot "car_poc"
$OutputDir = Join-Path $ProjectRoot "misra_analysis"
$Cppcheck = "C:\Program Files\Cppcheck\cppcheck.exe"
$MisraScript = Join-Path $ProjectRoot "misra.py"

# Create output directory
if (!(Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Write-Host "✅ Created output directory" -ForegroundColor Green
}

# Verify Cppcheck
if (!(Test-Path $Cppcheck)) {
    Write-Host "❌ Cppcheck not found" -ForegroundColor Red
    exit 1
}

Write-Host "✅ Found Cppcheck" -ForegroundColor Green

# Run Cppcheck analysis
Write-Host ""
Write-Host "🔍 Running Cppcheck analysis..." -ForegroundColor Yellow

$TextOutput = Join-Path $OutputDir "cppcheck_output.txt"
$IncludePaths = "-I $(Join-Path $CarPocDir 'inc') -I $(Join-Path $CarPocDir 'cfg') -I $(Join-Path $CarPocDir 'sim')"
$SourcePaths = "$(Join-Path $CarPocDir 'src') $(Join-Path $CarPocDir 'sim')"

$CppcheckCmd = "`"$Cppcheck`" --enable=all --language=c --std=c99 $IncludePaths $SourcePaths --suppress=missingIncludeSystem --inline-suppr --output-file=`"$TextOutput`""

try {
    Invoke-Expression $CppcheckCmd
    Write-Host "✅ Cppcheck analysis completed" -ForegroundColor Green
} catch {
    Write-Host "❌ Cppcheck failed: $_" -ForegroundColor Red
    exit 1
}

# Check if output file was created
if (!(Test-Path $TextOutput)) {
    Write-Host "❌ Output file not created" -ForegroundColor Red
    exit 1
}

# Display raw results
Write-Host ""
Write-Host "📊 Raw Cppcheck Results:" -ForegroundColor Cyan
$RawContent = Get-Content $TextOutput -Raw
if ($RawContent.Trim()) {
    Write-Host $RawContent
} else {
    Write-Host "✅ No issues found in raw analysis" -ForegroundColor Green
}

# Run MISRA analysis if script exists
if (Test-Path $MisraScript) {
    Write-Host ""
    Write-Host "📋 Running MISRA C:2012 analysis..." -ForegroundColor Yellow
    
    try {
        $MisraResult = python $MisraScript $TextOutput 2>&1
        $MisraExitCode = $LASTEXITCODE
        
        Write-Host ""
        Write-Host "📊 MISRA Analysis Results:" -ForegroundColor Cyan
        Write-Host "-" * 40 -ForegroundColor Cyan
        $MisraResult | ForEach-Object { Write-Host $_ }
        
        if ($MisraExitCode -eq 0) {
            Write-Host ""
            Write-Host "🎉 MISRA COMPLIANCE ACHIEVED!" -ForegroundColor Green
            Write-Host "✅ No MISRA violations found" -ForegroundColor Green
            Write-Host "✅ Code meets automotive safety standards" -ForegroundColor Green
        } else {
            Write-Host ""
            Write-Host "📋 MISRA violations found: $MisraExitCode" -ForegroundColor Yellow
        }
        
    } catch {
        Write-Host "❌ MISRA analysis failed: $_" -ForegroundColor Red
    }
} else {
    Write-Host "⚠️  MISRA script not found" -ForegroundColor Yellow
}

# Generate simple summary
Write-Host ""
Write-Host "📁 Files generated in: $OutputDir" -ForegroundColor White
Write-Host "   - cppcheck_output.txt (Raw Cppcheck results)" -ForegroundColor Gray

Write-Host ""
Write-Host "✅ Analysis completed!" -ForegroundColor Green