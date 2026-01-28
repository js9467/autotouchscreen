# Common IPs to try
$ips = @(
    "192.168.4.250",   # Default AP
    "192.168.7.116",   # From earlier OTA attempts
    "192.168.1.116",   # Common router range
    "192.168.0.116",   # Common router range
    "10.0.0.116"       # Some networks
)

Write-Host "Scanning for Bronco Controls device..." -ForegroundColor Cyan
Write-Host ""

foreach ($ip in $ips) {
    Write-Host "Trying $ip... " -NoNewline
    try {
        $result = Invoke-RestMethod -Uri "http://$ip/api/status" -Method GET -TimeoutSec 2 -ErrorAction Stop
        Write-Host "✓ FOUND!" -ForegroundColor Green
        Write-Host "  Version: $($result.firmware_version)" -ForegroundColor White
        Write-Host "  Use this IP: $ip" -ForegroundColor Yellow
        Write-Host ""
        exit 0
    } catch {
        Write-Host "✗" -ForegroundColor DarkGray
    }
}

Write-Host ""
Write-Host "Device not found on common IPs." -ForegroundColor Red
Write-Host "Check your WiFi settings or connect to the device AP network." -ForegroundColor Yellow
