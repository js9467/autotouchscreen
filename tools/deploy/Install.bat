@echo off
:: Bronco Controls Flasher - Downloads and runs the installer
echo.
echo ============================================
echo    Bronco Controls ESP32 Flasher
echo ============================================
echo.
echo Downloading installer...
echo.

PowerShell -NoProfile -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/js9467/autotouchscreen/main/tools/deploy/BroncoFlasher.ps1 | iex"

if %errorlevel% neq 0 (
    echo.
    echo Installation failed. Press any key to exit...
    pause >nul
) else (
    echo.
    echo Press any key to exit...
    pause >nul
)
