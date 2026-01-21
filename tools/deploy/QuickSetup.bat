@echo off
:: Bronco Controls - Quick Setup (One-Click Installer)
:: This file downloads and runs the automated installer
echo.
echo ====================================================
echo    Bronco Controls - Quick Setup
echo ====================================================
echo.
echo This will:
echo   1. Check for ESP32 device connection
echo   2. Install USB drivers if needed
echo   3. Flash the latest firmware
echo   4. Open the web interface
echo.
echo Make sure your ESP32 device is connected via USB!
echo.
pause

PowerShell -NoProfile -ExecutionPolicy Bypass -Command ^
"Remove-Item \"$env:LOCALAPPDATA\BroncoControls\" -Recurse -Force -ErrorAction SilentlyContinue; ^
$timestamp = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds(); ^
$url = \"https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/BroncoFlasher.ps1?nocache=$timestamp\"; ^
iex (irm $url)"

if %errorlevel% neq 0 (
    echo.
    echo.
    echo ====================================================
    echo    Installation failed or was cancelled
    echo ====================================================
    echo.
    echo Common issues:
    echo   - ESP32 not connected
    echo   - Wrong USB cable (needs data, not just charging)
    echo   - Another program is using the COM port
    echo.
    echo Try again after checking the above.
    echo.
    pause
) else (
    echo.
    echo ====================================================
    echo    Installation Complete!
    echo ====================================================
    echo.
    pause
)
