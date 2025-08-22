# 🚗 Car POC – Running Cppcheck with MISRA on Windows

This guide explains how to **build your Car POC project** (C, CMake, SDL2 optional) and run **Cppcheck with MISRA addon** to generate text and HTML reports.

---

## 🔹 Prerequisites
- Windows 10/11 with PowerShell
- [CMake](https://cmake.org/download/) (with Ninja installed)
- [Cppcheck (MSI Installer)](https://github.com/danmar/cppcheck/releases) – e.g., `cppcheck-2.18.0-x64-Setup.msi`
- (Optional) [Python](https://www.python.org/downloads/) – needed for HTML report generation
- SDL2 SDK (if GUI mode is desired), e.g., installed at `C:/SDKs/SDL2-2.30.12/cmake`

---

## 🔹 PowerShell Script Prompt for Claude Code

Copy-paste the following into Claude Code or your local PowerShell to **build and analyze** the project.

```powershell
# === Step 1. Set Variables ===
$Repo      = "C:\Work\car_poc"
$BuildDir  = Join-Path $Repo "build"
$Cppcheck  = "C:\Program Files\Cppcheck\cppcheck.exe"
$MisraPy   = "C:\Program Files\Cppcheck\addons\misra.py"
$HtmlRep   = "C:\Program Files\Cppcheck\htmlreport\cppcheck-htmlreport"
$Sdl2Dir   = "C:\SDKs\SDL2-2.30.12\cmake"   # set to your SDL2 cmake folder; leave blank for HEADLESS

# === Step 2. Verify Tools ===
$ErrorActionPreference = "Stop"
cmd /c "cmake --version"
cmd /c "ninja --version"
& $Cppcheck --version

# === Step 3. Configure + Build ===
if (Test-Path $BuildDir) { Remove-Item $BuildDir -Recurse -Force }
New-Item -ItemType Directory -Path $BuildDir | Out-Null

$cmakeArgs = @("-B", $BuildDir, "-S", $Repo, "-G", "Ninja", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
if (Test-Path $Sdl2Dir) { $cmakeArgs += @("-DHEADLESS=OFF", "-DSDL2_DIR=$Sdl2Dir") } else { $cmakeArgs += @("-DHEADLESS=ON") }
cmd /c ("cmake " + ($cmakeArgs -join " "))
cmd /c "cmake --build `"$BuildDir`" -j"

if (!(Test-Path (Join-Path $BuildDir 'compile_commands.json'))) { throw 'compile_commands.json not found' }

# === Step 4. Run Cppcheck (MISRA) ===
$TxtOut = Join-Path $BuildDir "cppcheck.txt"
& $Cppcheck --enable=all `
            --language=c `
            --addon="$MisraPy" `
            --project=(Join-Path $BuildDir "compile_commands.json") `
            --suppress=missingIncludeSystem `
            --inline-suppr `
            --output-file="$TxtOut"
Write-Host "Cppcheck text report: $TxtOut"

# === Step 5. HTML Report (optional, requires Python) ===
$HtmlDir = Join-Path $BuildDir "cppcheck-html"
try {
  python "$HtmlRep" --file="$TxtOut" --report-dir="$HtmlDir" --source-dir="$Repo"
  Write-Host "Cppcheck HTML report: $HtmlDir\index.html"
} catch {
  Write-Warning "HTML report skipped (Python not found)."
}

# === Step 6. Compute KPIs ===
$summary = Select-String -Path $TxtOut -Pattern "^\[.+\] " | Measure-Object
$loc = (Get-ChildItem -Path $Repo -Include *.c,*.h -Recurse | Get-Content | Measure-Object -Line).Lines
if ($loc -eq 0) { $loc = 1 }
$kloc = [math]::Round($loc / 1000.0, 2)
Write-Host ("Findings: {0}  |  LOC: {1}  |  Violations/KLOC: {2}" -f $summary.Count, $loc, [math]::Round($summary.Count/[double]$kloc,2))
```

---

## 🔹 What You Get
- **`cppcheck.txt`** → Text report with all findings
- **`cppcheck-html/index.html`** → Polished HTML report (if Python installed)
- **KPIs** printed in console:
  - Total findings
  - Lines of code
  - Violations per KLOC

---

## 🔹 Notes
- If SDL2 isn’t installed, set `$Sdl2Dir = ""` → runs HEADLESS mode.  
- If you see “missing include” warnings, confirm the project builds cleanly.  
- Cppcheck MISRA addon covers many but not all MISRA rules — suitable for **POC/demo** purposes.  

---
