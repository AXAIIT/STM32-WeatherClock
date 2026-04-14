param(
    [string]$CSource = "..\..\App\LVGL_myGui\src\generated\images\_Sunny_Cloudy_alpha_88x67.c",
    [string]$OutputBin = ".\template_100.bin",
    [int]$Width = 88,
    [int]$Height = 67,
    [int]$ColorFormatValue = 5
)

$ErrorActionPreference = "Stop"

function Get-HexBytesFromBlock {
    param(
        [string]$Text,
        [string]$StartMarker,
        [string]$EndMarker
    )

    $start = $Text.IndexOf($StartMarker)
    if($start -lt 0) {
        throw "Start marker not found: $StartMarker"
    }

    $sub = $Text.Substring($start)
    $end = $sub.IndexOf($EndMarker)
    if($end -lt 0) {
        throw "End marker not found after start marker: $EndMarker"
    }

    $block = $sub.Substring(0, $end)
    $matches = [regex]::Matches($block, '0x([0-9A-Fa-f]{2})')
    if($matches.Count -eq 0) {
        throw "No hex bytes found in selected section."
    }

    $bytes = New-Object byte[] $matches.Count
    for($i = 0; $i -lt $matches.Count; $i++) {
        $bytes[$i] = [Convert]::ToByte($matches[$i].Groups[1].Value, 16)
    }
    return $bytes
}

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cAbs = if([System.IO.Path]::IsPathRooted($CSource)) { $CSource } else { Join-Path $scriptRoot $CSource }
$outAbs = if([System.IO.Path]::IsPathRooted($OutputBin)) { $OutputBin } else { Join-Path $scriptRoot $OutputBin }

if(-not (Test-Path $cAbs)) {
    throw "C source not found: $cAbs"
}

$content = Get-Content -Path $cAbs -Raw -Encoding UTF8

$pixelBytes = Get-HexBytesFromBlock -Text $content `
    -StartMarker "#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP == 0" `
    -EndMarker "#if LV_COLOR_DEPTH == 16 && LV_COLOR_16_SWAP != 0"

$header = ([uint32]($ColorFormatValue -band 0x1F)) `
    -bor ([uint32](($Width -band 0x7FF) -shl 10)) `
    -bor ([uint32](($Height -band 0x7FF) -shl 21))

$headerBytes = [BitConverter]::GetBytes($header)
$outBytes = New-Object byte[] ($headerBytes.Length + $pixelBytes.Length)
[System.Array]::Copy($headerBytes, 0, $outBytes, 0, $headerBytes.Length)
[System.Array]::Copy($pixelBytes, 0, $outBytes, $headerBytes.Length, $pixelBytes.Length)

$outDir = Split-Path -Parent $outAbs
if(-not (Test-Path $outDir)) {
    New-Item -Path $outDir -ItemType Directory -Force | Out-Null
}

[System.IO.File]::WriteAllBytes($outAbs, $outBytes)

Write-Host "Template generated:"
Write-Host " - source: $cAbs"
Write-Host " - output: $outAbs"
Write-Host " - header: 0x$('{0:X8}' -f $header)"
Write-Host " - payload bytes: $($pixelBytes.Length)"
Write-Host " - total bytes: $($outBytes.Length)"
