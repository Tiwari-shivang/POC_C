# === Step 1. Set Variables ===
$Repo      = "C:\Users\shivang.tiwari\Mercedes_POC"
$CarPocDir = Join-Path $Repo "car_poc"
$BuildDir  = Join-Path $Repo "build"
$Cppcheck  = "C:\Program Files\Cppcheck\cppcheck.exe"
$MisraPy   = "C:\Program Files\Cppcheck\addons\misra.py"
$HtmlRep   = "C:\Program Files\Cppcheck\htmlreport\cppcheck-htmlreport"
$Sdl2Dir   = "C:\SDKs\SDL2-2.30.12\cmake"   # set to your SDL2 cmake folder; leave blank for HEADLESS

# === Step 2. Verify Tools ===
$ErrorActionPreference = "Stop"
Write-Host "Checking CMake version..."
cmd /c "cmake --version"
Write-Host "Checking for build tools..."
Write-Host "Checking Cppcheck version..."
& $Cppcheck --version

# === Step 3. Configure + Build ===
Write-Host "Setting up build directory..."
if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force }
New-Item -ItemType Directory -Path $BuildDir | Out-Null

$cmakeArgs = @("-B", $BuildDir, "-S", $CarPocDir, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
if (Test-Path $Sdl2Dir) { 
    $cmakeArgs += @("-DHEADLESS=OFF", "-DSDL2_DIR=$Sdl2Dir") 
} else { 
    $cmakeArgs += @("-DHEADLESS=ON") 
}

Write-Host "Configuring with CMake..."
cmd /c ("cmake " + ($cmakeArgs -join " "))
Write-Host "Building project..."
cmd /c "cmake --build `"$BuildDir`" -j"

Write-Host "Build completed (some test link errors are expected)..."

# === Step 4. Run Cppcheck (with MISRA if available) ===
Write-Host "Running Cppcheck analysis..."
$TxtOut = Join-Path $BuildDir "cppcheck.txt"
$IncludePaths = @("-I", (Join-Path $CarPocDir "inc"), "-I", (Join-Path $CarPocDir "cfg"), "-I", (Join-Path $CarPocDir "sim"))
$SourcePaths = @((Join-Path $CarPocDir "src"), (Join-Path $CarPocDir "sim"))

# Check if MISRA addon exists
if (Test-Path $MisraPy) {
    Write-Host "Using MISRA addon..."
    & $Cppcheck --enable=all `
                --language=c `
                --addon="$MisraPy" `
                $IncludePaths `
                $SourcePaths `
                --suppress=missingIncludeSystem `
                --inline-suppr `
                --output-file="$TxtOut" `
                2>&1 | Out-String | Write-Host
} else {
    Write-Host "MISRA addon not found, running standard analysis..."
    & $Cppcheck --enable=all `
                --language=c `
                $IncludePaths `
                $SourcePaths `
                --suppress=missingIncludeSystem `
                --inline-suppr `
                --output-file="$TxtOut" `
                2>&1 | Out-String | Write-Host
}
Write-Host "Cppcheck text report: $TxtOut"

# === Step 5. HTML Report (optional, requires Python) ===
$HtmlDir = Join-Path $BuildDir "cppcheck-html"
try {
  Write-Host "Generating HTML report..."
  python "$HtmlRep" --file="$TxtOut" --report-dir="$HtmlDir" --source-dir="$CarPocDir"
  Write-Host "Cppcheck HTML report: $HtmlDir\index.html"
} catch {
  Write-Warning "HTML report skipped (Python not found)."
}

# === Step 6. Compute KPIs ===
Write-Host "Computing KPIs..."
$summary = Select-String -Path $TxtOut -Pattern "^\[.+\] " | Measure-Object
$loc = (Get-ChildItem -Path $CarPocDir -Include *.c,*.h -Recurse | Get-Content | Measure-Object -Line).Lines
if ($loc -eq 0) { $loc = 1 }
$kloc = [math]::Round($loc / 1000.0, 2)
Write-Host ("Findings: {0}  |  LOC: {1}  |  Violations/KLOC: {2}" -f $summary.Count, $loc, [math]::Round($summary.Count/[double]$kloc,2))

# === Step 7. Copy results to root directory ===
Write-Host "Copying results to root directory..."
$RootTxtOut = Join-Path $Repo "cppcheck_results.txt"
Copy-Item $TxtOut $RootTxtOut -Force
Write-Host "Results copied to: $RootTxtOut"

if (Test-Path $HtmlDir) {
    $RootHtmlDir = Join-Path $Repo "cppcheck-html-report"
    if (Test-Path $RootHtmlDir) { Remove-Item $RootHtmlDir -Recurse -Force }
    Copy-Item $HtmlDir $RootHtmlDir -Recurse -Force
    Write-Host "HTML report copied to: $RootHtmlDir\index.html"
}