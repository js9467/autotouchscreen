$send = @{pgn="FF41"; data=@(17,0,0,0,0,0,0,0)} | ConvertTo-Json
Write-Host "Sending CAN frame..."
$result = Invoke-RestMethod -Method Post -Uri "http://192.168.7.116/api/can/send" -Body $send -ContentType "application/json"
Write-Host "Result: $($result | ConvertTo-Json)"

Start-Sleep 2

Write-Host "`nChecking for responses..."
$rx = Invoke-RestMethod -Uri "http://192.168.7.116/api/can/receive?timeout=2000"
Write-Host "Received $($rx.count) frames"
$rx.messages | ForEach-Object { 
    Write-Host "ID: $($_.id) Data: $($_.data -join ' ')" 
}
