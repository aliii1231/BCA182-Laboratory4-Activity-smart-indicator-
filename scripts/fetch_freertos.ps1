param([string]$Ref = "V11.1.0")

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$target = Join-Path $repoRoot "lib/FreeRTOS/Source"
$archive = Join-Path $env:TEMP "freertos-kernel-$Ref.zip"
$extractRoot = Join-Path $env:TEMP "freertos-kernel-$Ref"
$url = "https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/$Ref.zip"

if (Test-Path (Join-Path $target "include/FreeRTOS.h")) {
    Write-Host "FreeRTOS kernel already exists at $target"
    exit 0
}
if (Test-Path $target) { throw "Target exists but is incomplete; inspect it before retrying: $target" }

Write-Host "Downloading official FreeRTOS-Kernel $Ref..."
Invoke-WebRequest -Uri $url -OutFile $archive
if (Test-Path $extractRoot) { Remove-Item -LiteralPath $extractRoot -Recurse -Force }
Expand-Archive -LiteralPath $archive -DestinationPath $extractRoot
$source = Get-ChildItem -LiteralPath $extractRoot -Directory | Select-Object -First 1
if (-not $source -or -not (Test-Path (Join-Path $source.FullName "portable/GCC/ARM_CM4F/port.c"))) {
    throw "Downloaded archive does not contain the expected ARM_CM4F port."
}
New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
Copy-Item -LiteralPath $source.FullName -Destination $target -Recurse
if (-not (Test-Path (Join-Path $target "include/FreeRTOS.h")) -or -not (Test-Path (Join-Path $target "portable/MemMang/heap_4.c"))) {
    throw "Kernel extraction incomplete; verify the official archive contents."
}
Write-Host "Installed FreeRTOS $Ref into $target"
Write-Host "Run: pio run -e rt_spark_f407"
