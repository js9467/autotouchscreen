# InfinityBox PowerCell J1939 CAN Test Script
# Based on POWERCELL Standard J1939 Manual
# Sends test frames to various PowerCell addresses

$DeviceIP = "192.168.7.116"
$BaseURL = "http://$DeviceIP/api/can/send"

Write-Host "`n=== InfinityBox PowerCell J1939 Test ===" -ForegroundColor Cyan
Write-Host "Device: $DeviceIP`n" -ForegroundColor Yellow

# Define PowerCell PGN addresses from manual
# PGN FF01 = Address A1, FF02 = A2, FF03 = A3, etc.
$PGNs = @(
    @{Address="A1"; PGN=0xFF01},
    @{Address="A2"; PGN=0xFF02},
    @{Address="A3"; PGN=0xFF03},
    @{Address="A4"; PGN=0xFF04},
    @{Address="A5"; PGN=0xFF05},
    @{Address="A6"; PGN=0xFF06},
    @{Address="A7"; PGN=0xFF07},
    @{Address="A8"; PGN=0xFF08}
)

# Test patterns from manual
# Byte layout: 
# Bytes 1-4: Outputs 1-10 (Track), 11-20 (Soft-Start 2sec), 21-32 (PWM Track)
# Bytes 5-8: Outputs 33-62 (PWM Track), XX (unused)

$TestPatterns = @(
    @{
        Name = "All Outputs OFF"
        Data = @(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
        Description = "Turn off all outputs"
    },
    @{
        Name = "Output 1 ON (Byte 1 Bit 0)"
        Data = @(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
        Description = "Connector B, Cavity B, Track personality"
    },
    @{
        Name = "Output 2 ON (Byte 1 Bit 1)"
        Data = @(0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
        Description = "Connector B, Cavity C, Track personality"
    },
    @{
        Name = "Outputs 1-8 ON"
        Data = @(0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
        Description = "First 8 outputs ON"
    },
    @{
        Name = "Outputs 9-10 ON (Soft-Start)"
        Data = @(0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
        Description = "Outputs 9-10 with soft-start"
    },
    @{
        Name = "All First 16 Outputs ON"
        Data = @(0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
        Description = "Outputs 1-16 ON"
    },
    @{
        Name = "Alternating Pattern"
        Data = @(0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x00)
        Description = "Alternating outputs for testing"
    },
    @{
        Name = "All Outputs ON"
        Data = @(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00)
        Description = "Maximum outputs ON"
    }
)

function Send-CANFrame {
    param(
        [int]$PGN,
        [array]$Data,
        [string]$Description
    )
    
    $body = @{
        pgn = $PGN
        priority = 6
        source = 0xF9
        destination = 0xFF
        data = $Data
    } | ConvertTo-Json
    
    try {
        $response = Invoke-RestMethod -Uri $BaseURL -Method POST -Body $body -ContentType "application/json" -TimeoutSec 2
        if ($response.success) {
            Write-Host "  ✓ Sent PGN 0x$($response.pgn) ($($response.bytes) bytes)" -ForegroundColor Green
        } else {
            Write-Host "  ✗ Failed to send frame" -ForegroundColor Red
        }
    } catch {
        Write-Host "  ✗ Error: $($_.Exception.Message)" -ForegroundColor Red
    }
}

# Test 1: Send frames to all PowerCell addresses
Write-Host "`n--- Test 1: Scan All PowerCell Addresses (A1-A8) ---" -ForegroundColor Cyan
Write-Host "Sending 'Output 1 ON' pattern to each address...`n" -ForegroundColor Yellow

foreach ($pgn in $PGNs) {
    Write-Host "Address $($pgn.Address) (PGN 0x$($pgn.PGN.ToString('X4'))):" -ForegroundColor White
    Send-CANFrame -PGN $pgn.PGN -Data @(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) -Description "Test output 1"
    Start-Sleep -Milliseconds 500
}

# Test 2: Cycle through patterns on first address
Write-Host "`n--- Test 2: Pattern Cycling on Address A1 (PGN 0xFF01) ---" -ForegroundColor Cyan
Write-Host "Cycling through different output patterns...`n" -ForegroundColor Yellow

foreach ($pattern in $TestPatterns) {
    Write-Host "$($pattern.Name): $($pattern.Description)" -ForegroundColor White
    Send-CANFrame -PGN 0xFF01 -Data $pattern.Data -Description $pattern.Description
    Start-Sleep -Milliseconds 800
}

# Test 3: Rapid fire test
Write-Host "`n--- Test 3: Rapid Frame Transmission ---" -ForegroundColor Cyan
Write-Host "Sending 20 frames rapidly to test response...`n" -ForegroundColor Yellow

for ($i = 1; $i -le 20; $i++) {
    $data = if ($i % 2 -eq 0) { @(0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) } 
            else { @(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) }
    
    Write-Host "  Frame $i" -NoNewline
    Send-CANFrame -PGN 0xFF01 -Data $data -Description "Rapid test $i"
    Start-Sleep -Milliseconds 100
}

# Test 4: Send to multiple addresses with different patterns
Write-Host "`n--- Test 4: Multi-Address Pattern ---" -ForegroundColor Cyan
Write-Host "Sending different patterns to A1-A4...`n" -ForegroundColor Yellow

Send-CANFrame -PGN 0xFF01 -Data @(0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) -Description "A1: Output 1"
Start-Sleep -Milliseconds 200
Send-CANFrame -PGN 0xFF02 -Data @(0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) -Description "A2: Output 2"
Start-Sleep -Milliseconds 200
Send-CANFrame -PGN 0xFF03 -Data @(0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) -Description "A3: Output 3"
Start-Sleep -Milliseconds 200
Send-CANFrame -PGN 0xFF04 -Data @(0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) -Description "A4: Output 4"

# Final: Turn everything off
Write-Host "`n--- Cleanup: Turning All Outputs OFF ---" -ForegroundColor Cyan
foreach ($pgn in $PGNs) {
    Send-CANFrame -PGN $pgn.PGN -Data @(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00) -Description "Off"
    Start-Sleep -Milliseconds 200
}

Write-Host "`n=== Test Complete ===" -ForegroundColor Green
Write-Host "`nCheck your InfinityBox for:" -ForegroundColor Yellow
Write-Host "  1. Blue LED blinking (indicates CAN traffic received)" -ForegroundColor White
Write-Host "  2. Output activity on the configured channels" -ForegroundColor White
Write-Host "  3. Check your ESP32 serial monitor for CAN transmission logs" -ForegroundColor White
Write-Host "`nIf no response, verify:" -ForegroundColor Yellow
Write-Host "  - CAN bus termination (120Ω resistors)" -ForegroundColor White
Write-Host "  - CAN_H and CAN_L wiring is correct" -ForegroundColor White
Write-Host "  - PowerCell is powered and configured to correct address" -ForegroundColor White
Write-Host "  - Baud rate is 250kbps on both devices`n" -ForegroundColor White
