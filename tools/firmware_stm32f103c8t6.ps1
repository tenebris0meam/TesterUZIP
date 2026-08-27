param (
    [Parameter(Mandatory=$true)]
    [string]$FirmwareFile
)

# Configuration paths
$OpenOcdPath = "openocd"
$GdbPath     = "arm-none-eabi-gdb"

# Convert to absolute path to avoid escaping issues
try {
    $FirmwarePath = Resolve-Path -Path $FirmwareFile -ErrorAction Stop
} catch {
    Write-Error "Firmware file not found: $FirmwareFile"
    exit 1
}
# Заменяем обратные слеши на прямые, чтобы избежать проблем с экранированием
$FirmwarePathForCommand = $FirmwarePath.Path.Replace('\', '/')

# Determine base address for .bin files (ELF/HEX contain it)
$BaseAddress = ""
if ($FirmwarePath.Path -match "\.bin$") {
    $BaseAddress = "0x08000000"
}

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

# 2. Create temporary file with GDB commands
$GdbCommandsFile = New-TemporaryFile
$Commands = @(
    "target extended-remote localhost:3333"
    "monitor reset halt"
)

# Build flash command with properly escaped path
$FlashCmd = if ($BaseAddress) {
    "monitor flash write_image erase `"$($FirmwarePathForCommand)`" $BaseAddress"
} else {
    "monitor flash write_image erase `"$($FirmwarePathForCommand)`""
}
$Commands += $FlashCmd

$Commands += @(
    "monitor reset"
    "quit"
)

# Write to file (UTF-8 without BOM)
[System.IO.File]::WriteAllLines($GdbCommandsFile.FullName, $Commands)

# 3. Run GDB
Write-Host "Connecting to GDB and flashing firmware..." -ForegroundColor Cyan
& $GdbPath --batch -x $GdbCommandsFile.FullName

# 4. Clean up
Remove-Item -Path $GdbCommandsFile.FullName -Force

Write-Host "Stopping OpenOCD..." -ForegroundColor Cyan
Stop-Process -Id $OpenOcdProcess.Id -Force

Write-Host "Firmware flashed successfully!" -ForegroundColor Green