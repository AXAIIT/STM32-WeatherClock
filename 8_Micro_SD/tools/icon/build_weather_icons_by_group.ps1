param(
    [string]$GroupIconDir = ".\group_icons",
    [string]$MappingFile = ".\weather_code_groups.csv",
    [string]$OutputDir = ".\out",
    [int]$Width = 88,
    [int]$Height = 67,
    [string]$ColorFormat = "RGB565A8",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Resolve-LvImgConv {
    $cmd = Get-Command "lv_img_conv" -ErrorAction SilentlyContinue
    if($cmd) {
        $candidate = $cmd.Source
        if(Test-Path $candidate) {
            $content = Get-Content $candidate -TotalCount 40 -ErrorAction SilentlyContinue
            if(($content -join "`n") -match "/usr/bin/perl") {
                $candidate = $null
            }
        }
        if($candidate) { return $candidate }
    }

    $cmd = Get-Command "lv_img_conv.cmd" -ErrorAction SilentlyContinue
    if($cmd) {
        $candidate = $cmd.Source
        if(Test-Path $candidate) {
            $content = Get-Content $candidate -TotalCount 40 -ErrorAction SilentlyContinue
            if(($content -join "`n") -match "/usr/bin/perl") {
                $candidate = $null
            }
        }
        if($candidate) { return $candidate }
    }

    $fallback = Join-Path $env:APPDATA "npm\lv_img_conv.cmd"
    if(Test-Path $fallback) {
        $content = Get-Content $fallback -TotalCount 40 -ErrorAction SilentlyContinue
        if((($content -join "`n") -notmatch "/usr/bin/perl")) {
            return $fallback
        }
    }

    return $null
}

function New-LvglV8Header {
    param(
        [int]$ColorFormat,
        [int]$Width,
        [int]$Height
    )

    if($Width -lt 1 -or $Width -gt 2047) { throw "Width out of range (1..2047): $Width" }
    if($Height -lt 1 -or $Height -gt 2047) { throw "Height out of range (1..2047): $Height" }
    if($ColorFormat -lt 0 -or $ColorFormat -gt 31) { throw "Color format out of range (0..31): $ColorFormat" }

    $value = ($ColorFormat -band 0x1F)
    $value = $value -bor (($Width -band 0x7FF) -shl 10)
    $value = $value -bor (($Height -band 0x7FF) -shl 21)

    return [System.BitConverter]::GetBytes([UInt32]$value)
}

function Convert-PngToLvglBinNative {
    param(
        [string]$InputPath,
        [string]$OutputPath,
        [int]$ImageWidth,
        [int]$ImageHeight,
        [string]$Cf
    )

    if($Cf -ne "RGB565A8") {
        throw "Native converter only supports RGB565A8. Current: $Cf"
    }

    Add-Type -AssemblyName System.Drawing

    $bitmap = $null
    $workBitmap = $null
    $stream = $null
    try {
        $stream = [System.IO.File]::OpenRead($InputPath)
        $bitmap = [System.Drawing.Bitmap]::FromStream($stream)

        if($bitmap.Width -ne $ImageWidth -or $bitmap.Height -ne $ImageHeight) {
            $workBitmap = New-Object System.Drawing.Bitmap($ImageWidth, $ImageHeight, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $graphics = [System.Drawing.Graphics]::FromImage($workBitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.DrawImage($bitmap, 0, 0, $ImageWidth, $ImageHeight)
            }
            finally {
                $graphics.Dispose()
            }
        }
        else {
            $workBitmap = $bitmap
        }

        $header = New-LvglV8Header -ColorFormat 5 -Width $ImageWidth -Height $ImageHeight
        $pixelCount = $ImageWidth * $ImageHeight
        $payload = New-Object byte[] ($pixelCount * 3)
        $idx = 0

        for($y = 0; $y -lt $ImageHeight; $y++) {
            for($x = 0; $x -lt $ImageWidth; $x++) {
                $c = $workBitmap.GetPixel($x, $y)
                $rgb565 = ((($c.R -band 0xF8) -shl 8) -bor (($c.G -band 0xFC) -shl 3) -bor (($c.B -band 0xF8) -shr 3))

                $payload[$idx] = [byte]($rgb565 -band 0xFF)
                $payload[$idx + 1] = [byte](($rgb565 -shr 8) -band 0xFF)
                $payload[$idx + 2] = [byte]$c.A
                $idx += 3
            }
        }

        $outStream = $null
        try {
            $outStream = [System.IO.File]::Open($OutputPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
            $outStream.Write($header, 0, $header.Length)
            $outStream.Write($payload, 0, $payload.Length)
        }
        finally {
            if($outStream) { $outStream.Dispose() }
        }
    }
    finally {
        if($workBitmap -and ($workBitmap -ne $bitmap)) { $workBitmap.Dispose() }
        if($bitmap) { $bitmap.Dispose() }
        if($stream) { $stream.Dispose() }
    }
}

function Invoke-LvImgConv {
    param(
        [string]$Exe,
        [string]$InputPath,
        [string]$OutputPath,
        [int]$ImageWidth,
        [int]$ImageHeight,
        [string]$Cf
    )

    & $Exe "--input" $InputPath "--output" $OutputPath "--output-format" "bin" "--color-format" $Cf "--width" "$ImageWidth" "--height" "$ImageHeight" "--no-compress"
    if($LASTEXITCODE -eq 0) { return }

    & $Exe $InputPath "-o" $OutputPath "-f" "bin" "-c" $Cf "--width" "$ImageWidth" "--height" "$ImageHeight" "--no-compress"
    if($LASTEXITCODE -eq 0) { return }

    & $Exe "--input" $InputPath "--output" $OutputPath "--format" "bin" "--cf" $Cf "--width" "$ImageWidth" "--height" "$ImageHeight" "--no-compress"
    if($LASTEXITCODE -eq 0) { return }

    throw "lv_img_conv invocation failed for '$InputPath'."
}

if(-not (Test-Path $GroupIconDir)) {
    throw "Group icon directory not found: $GroupIconDir"
}

if(-not (Test-Path $MappingFile)) {
    throw "Mapping file not found: $MappingFile"
}

$rows = Import-Csv -Path $MappingFile -Encoding UTF8
if(($rows -eq $null) -or ($rows.Count -eq 0)) {
    throw "No mapping rows found in: $MappingFile"
}

$requiredGroups = $rows | ForEach-Object { $_.group.Trim() } | Sort-Object -Unique

$cacheDir = Join-Path $GroupIconDir ".cache_bin"
New-Item -Path $cacheDir -ItemType Directory -Force | Out-Null

$resolvedBinByGroup = @{}

$conv = Resolve-LvImgConv

$missingGroups = @()
foreach($group in $requiredGroups) {
    $groupPng = Join-Path $GroupIconDir ("{0}.png" -f $group)
    $groupBin = Join-Path $GroupIconDir ("{0}.bin" -f $group)

    if(Test-Path $groupPng) {
        $cachedBin = Join-Path $cacheDir ("{0}.bin" -f $group)
        if($Force -or (-not (Test-Path $cachedBin))) {
            if($conv) {
                try {
                    Invoke-LvImgConv -Exe $conv -InputPath $groupPng -OutputPath $cachedBin -ImageWidth $Width -ImageHeight $Height -Cf $ColorFormat
                }
                catch {
                    Write-Host "lv_img_conv failed, fallback to native converter: $($_.Exception.Message)" -ForegroundColor Yellow
                    Convert-PngToLvglBinNative -InputPath $groupPng -OutputPath $cachedBin -ImageWidth $Width -ImageHeight $Height -Cf $ColorFormat
                }
            }
            else {
                Convert-PngToLvglBinNative -InputPath $groupPng -OutputPath $cachedBin -ImageWidth $Width -ImageHeight $Height -Cf $ColorFormat
            }
        }

        $resolvedBinByGroup[$group] = $cachedBin
        continue
    }

    if(Test-Path $groupBin) {
        $resolvedBinByGroup[$group] = $groupBin
        continue
    }

    $missingGroups += $group
}

if($missingGroups.Count -gt 0) {
    Write-Host "Missing group icon files:" -ForegroundColor Yellow
    $missingGroups | ForEach-Object {
        Write-Host (" - {0}.png or {0}.bin" -f $_) -ForegroundColor Yellow
    }
    throw "Please provide missing group templates in: $GroupIconDir"
}

$outAbs = if([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path (Get-Location).Path $OutputDir }
New-Item -Path $outAbs -ItemType Directory -Force | Out-Null

$generated = 0
foreach($row in $rows) {
    $codeText = $row.code.Trim()
    if($codeText -notmatch '^[0-9]+$') {
        continue
    }

    $code = [int]$codeText
    $group = $row.group.Trim()

    $src = [string]$resolvedBinByGroup[$group]
    $dst = Join-Path $outAbs ("{0}.bin" -f $code)

    if((Test-Path $dst) -and (-not $Force)) {
        continue
    }

    Copy-Item -Path $src -Destination $dst -Force
    $generated++
}

Write-Host "Done (group mode):"
Write-Host " - mapping: $MappingFile"
Write-Host " - group icons: $GroupIconDir"
Write-Host " - color format: $ColorFormat"
Write-Host " - size: ${Width}x${Height}"
Write-Host " - output: $outAbs"
Write-Host " - generated: $generated files"
Write-Host "Copy to SD: /icon/weather/*.bin"
