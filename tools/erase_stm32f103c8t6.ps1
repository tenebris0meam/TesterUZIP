# Configuration paths
$OpenOcdPath = "openocd"
$GdbPath     = "arm-none-eabi-gdb"

# 1. Start OpenOCD in the background
Write-Host "Starting OpenOCD..." -ForegroundColor Cyan
$OpenOcdProcess = Start-Process -FilePath $OpenOcdPath `
    -ArgumentList "-f interface/stlink.cfg -f target/stm32f1x.cfg" `
    -NoNewWindow -PassThru

# Wait for OpenOCD to initialize
Start-Sleep -Seconds 2

if ($OpenOcdProcess.HasExited) {
    Write-Error "Failed to start OpenOCD. Check ST-Link connection."
    exit 1
}

# 2. Create temporary file with GDB commands (Strict UTF-8 without BOM)
$GdbCommandsFile = New-TemporaryFile
$Commands = @(
    "target remote localhost:3333"
    "monitor reset halt"
    "monitor stm32f1x mass_erase 0"
    "monitor reset halt"
    "quit"
)
# Using .NET to force UTF-8 without BOM
[System.IO.File]::WriteAllLines($GdbCommandsFile.FullName, $Commands)

# 3. Run GDB to execute the commands
Write-Host "Connecting to GDB and erasing flash memory..." -ForegroundColor Cyan
& $GdbPath --batch -x $GdbCommandsFile.FullName

# 4. Clean up
Remove-Item -Path $GdbCommandsFile.FullName -Force

Write-Host "Stopping OpenOCD..." -ForegroundColor Cyan
Stop-Process -Id $OpenOcdProcess.Id -Force

Write-Host "MCU memory cleared successfully!" -ForegroundColor Green

