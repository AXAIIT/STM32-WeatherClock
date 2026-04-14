param(
    [string]$FontPath = "C:\Windows\Fonts\simhei.ttf",
    [string]$OutputDir = ".\out",
    [int]$Bpp = 4
)

$ErrorActionPreference = "Stop"

$nodeCmd = Get-Command "node" -ErrorAction SilentlyContinue
if ($nodeCmd -eq $null) {
    $nodeDir = "C:\Program Files\nodejs"
    if (Test-Path (Join-Path $nodeDir "node.exe")) {
        if (-not ($env:Path -like "*$nodeDir*")) {
            $env:Path = "$nodeDir;$env:Path"
        }
    }
}

if (-not (Test-Path $FontPath)) {
    throw "字体文件不存在: $FontPath"
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$chars17 = Join-Path $scriptDir "chars_17.txt"
$chars30 = Join-Path $scriptDir "chars_30.txt"

$fontConvCmd = $null
$cmd = Get-Command "lv_font_conv.cmd" -ErrorAction SilentlyContinue
if ($cmd -ne $null) {
    $fontConvCmd = $cmd.Source
}
if (-not $fontConvCmd) {
    $fallback = Join-Path $env:APPDATA "npm\lv_font_conv.cmd"
    if (Test-Path $fallback) {
        $fontConvCmd = $fallback
    }
}
if (-not $fontConvCmd) {
    throw "未找到 lv_font_conv.cmd。请先执行: npm.cmd install -g lv_font_conv"
}

if (-not (Test-Path $chars17)) { throw "缺少字符清单: $chars17" }
if (-not (Test-Path $chars30)) { throw "缺少字符清单: $chars30" }

$symbols17 = ((Get-Content -Path $chars17 -Encoding UTF8) -join "")
$symbols30 = ((Get-Content -Path $chars30 -Encoding UTF8) -join "")

if ([string]::IsNullOrWhiteSpace($symbols17)) { throw "chars_17.txt 内容为空" }
if ([string]::IsNullOrWhiteSpace($symbols30)) { throw "chars_30.txt 内容为空" }

$outAbs = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path (Get-Location).Path $OutputDir }
New-Item -Path $outAbs -ItemType Directory -Force | Out-Null

$font17Out = Join-Path $outAbs "font_cn_17.bin"
$font30Out = Join-Path $outAbs "font_cn_30.bin"

Write-Host "[1/2] 生成 17px 字库..."
& $fontConvCmd --font $FontPath --size 17 --bpp $Bpp --format bin --symbols $symbols17 -o $font17Out
if ($LASTEXITCODE -ne 0) { throw "17px 字库生成失败" }

Write-Host "[2/2] 生成 30px 字库..."
& $fontConvCmd --font $FontPath --size 30 --bpp $Bpp --format bin --symbols $symbols30 -o $font30Out
if ($LASTEXITCODE -ne 0) { throw "30px 字库生成失败" }

Write-Host "完成:"
Write-Host " - $font17Out"
Write-Host " - $font30Out"
Write-Host "建议拷贝到 SD: /font/font_cn_17.bin, /font/font_cn_30.bin"
