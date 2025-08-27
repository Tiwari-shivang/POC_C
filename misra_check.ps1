Write-Host "Mercedes POC - MISRA C:2012 Analysis" -ForegroundColor Cyan

$Cppcheck = "C:\Program Files\Cppcheck\cppcheck.exe"
$CarPocDir = "car_poc"
$OutputFile = "misra_analysis_output.txt"

Write-Host "Running Cppcheck analysis..."

& $Cppcheck --enable=all --language=c --std=c99 -I car_poc/inc -I car_poc/cfg -I car_poc/sim car_poc/src car_poc/sim --suppress=missingIncludeSystem --inline-suppr --output-file=$OutputFile

Write-Host "Analysis completed. Output saved to: $OutputFile"

if (Test-Path $OutputFile) {
    $content = Get-Content $OutputFile -Raw
    if ($content.Trim()) {
        Write-Host ""
        Write-Host "Issues found:" -ForegroundColor Yellow
        Write-Host $content
    } else {
        Write-Host ""
        Write-Host "No issues found! Code is clean." -ForegroundColor Green
    }
} else {
    Write-Host "Output file not created" -ForegroundColor Red
}

Write-Host ""
Write-Host "Running MISRA analysis..."
if (Test-Path "misra.py") {
    python misra.py $OutputFile
} else {
    Write-Host "MISRA script not found"
}