param(
    [string]$SourceImage,
    [string]$CodesFile = ".\weather_codes.txt",
    [string]$OutputDir = ".\out",
    [int]$Width = 88,
    [int]$Height = 67,
    [string]$ColorFormat = "RGB565A8",
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

if([string]::IsNullOrWhiteSpace($SourceImage)) {
    $template = Join-Path $scriptRoot "template_100.bin"
    if((-not (Test-Path $template)) -or $Force) {
        $builder = Join-Path $scriptRoot "build_icon_template_from_c.ps1"
        & powershell.exe -ExecutionPolicy Bypass -File $builder -OutputBin $template
        if($LASTEXITCODE -ne 0) {
            throw "Failed to generate template bin from C source."
        }
    }
    $SourceImage = $template
}

function Resolve-LvImgConv {
    $cmd = Get-Command "lv_img_conv" -ErrorAction SilentlyContinue
    if($cmd) { return $cmd.Source }

    $cmd = Get-Command "lv_img_conv.cmd" -ErrorAction SilentlyContinue
    if($cmd) { return $cmd.Source }

    $fallback = Join-Path $env:APPDATA "npm\lv_img_conv.cmd"
    if(Test-Path $fallback) { return $fallback }

    return $null
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
    if($LASTEXITCODE -eq 0) {
        return
    }

    & $Exe $InputPath "-o" $OutputPath "-f" "bin" "-c" $Cf "--width" "$ImageWidth" "--height" "$ImageHeight" "--no-compress"
    if($LASTEXITCODE -eq 0) {
        return
    }

    & $Exe "--input" $InputPath "--output" $OutputPath "--format" "bin" "--cf" $Cf "--width" "$ImageWidth" "--height" "$ImageHeight" "--no-compress"
    if($LASTEXITCODE -eq 0) {
        return
    }

    throw "lv_img_conv invocation failed. Run '$Exe -h' and adjust argument template in script."
}

if(-not (Test-Path $SourceImage)) {
    throw "Source image not found: $SourceImage"
}

if(-not (Test-Path $CodesFile)) {
    throw "Codes file not found: $CodesFile"
}

$codes = Get-Content -Path $CodesFile -Encoding UTF8 |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -match '^[0-9]+$' } |
    ForEach-Object { [int]$_ } |
    Sort-Object -Unique

if($codes.Count -eq 0) {
    throw "No valid weather codes in: $CodesFile"
}

$outAbs = if([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path (Get-Location).Path $OutputDir }
New-Item -Path $outAbs -ItemType Directory -Force | Out-Null

$ext = [System.IO.Path]::GetExtension($SourceImage).ToLowerInvariant()

if($ext -eq ".bin") {
    foreach($code in $codes) {
        $target = Join-Path $outAbs ("{0}.bin" -f $code)
        if((Test-Path $target) -and (-not $Force)) {
            continue
        }
        Copy-Item -Path $SourceImage -Destination $target -Force
    }

    Write-Host "Done (copy mode):" 
    Write-Host " - source: $SourceImage"
    Write-Host " - output: $outAbs"
    Write-Host " - files: $($codes.Count)"
    exit 0
}

$exe = Resolve-LvImgConv
if(-not $exe) {
    throw "lv_img_conv not found. Install with: npm.cmd install -g lv_img_conv"
}

$okCount = 0
foreach($code in $codes) {
    $target = Join-Path $outAbs ("{0}.bin" -f $code)
    if((Test-Path $target) -and (-not $Force)) {
        continue
    }

    Invoke-LvImgConv -Exe $exe -InputPath $SourceImage -OutputPath $target -ImageWidth $Width -ImageHeight $Height -Cf $ColorFormat

    if((Test-Path $target) -and ((Get-Item $target).Length -gt 0)) {
        $okCount++
    }
}

if($okCount -eq 0) {
    throw "No icon bin generated. Check lv_img_conv parameters and source image format."
}

Write-Host "Done:"
Write-Host " - source: $SourceImage"
Write-Host " - output: $outAbs"
Write-Host " - generated: $okCount files"
Write-Host "Copy to SD: /icon/weather/*.bin"
