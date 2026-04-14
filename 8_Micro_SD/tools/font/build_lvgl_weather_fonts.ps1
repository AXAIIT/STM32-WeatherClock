param(
    [string]$FontPath = "C:\Windows\Fonts\simhei.ttf",
    [string]$OutputDir = ".\out",
    [int]$Bpp = 2,
    [int]$LvVersion = 8
)

$ErrorActionPreference = "Stop"

$nodeCmd = Get-Command "node" -ErrorAction SilentlyContinue
if (-not $nodeCmd) {
    $nodeDir = "C:\Program Files\nodejs"
    if (Test-Path (Join-Path $nodeDir "node.exe")) {
        if (-not ($env:Path -like "*$nodeDir*")) {
            $env:Path = "$nodeDir;$env:Path"
        }
    }
}

if (-not (Test-Path $FontPath)) {
    throw "Font file not found: $FontPath"
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$chars18 = Join-Path $scriptDir "chars_18.txt"
$chars24 = Join-Path $scriptDir "chars_24.txt"
$weatherPhrases = Join-Path $scriptDir "weather_phrases.txt"

$fontConvCmd = $null
$cmd = Get-Command "lv_font_conv.cmd" -ErrorAction SilentlyContinue
if ($cmd) {
    $fontConvCmd = $cmd.Source
}
if (-not $fontConvCmd) {
    $fallback = Join-Path $env:APPDATA "npm\lv_font_conv.cmd"
    if (Test-Path $fallback) {
        $fontConvCmd = $fallback
    }
}
if (-not $fontConvCmd) {
    throw "lv_font_conv.cmd not found. Run: npm.cmd install -g lv_font_conv"
}

if (-not (Test-Path $weatherPhrases)) { throw "Missing weather phrase source: $weatherPhrases" }

function Get-WeatherPhraseLines {
    param([string]$Path)

    $lines = Get-Content -Path $Path -Encoding UTF8
    $result = New-Object System.Collections.Generic.List[string]

    foreach($line in $lines) {
        $trim = $line.Trim()
        if([string]::IsNullOrWhiteSpace($trim)) {
            continue
        }
        [void]$result.Add($trim)
    }

    if($result.Count -eq 0) {
        throw "Weather phrase source is empty: $Path"
    }

    return $result
}

function Write-CharListFile {
    param(
        [string]$Path,
        [string[]]$PrefixLines,
        [System.Collections.Generic.List[string]]$WeatherLines,
        [string[]]$SuffixLines
    )

    $allLines = New-Object System.Collections.Generic.List[string]

    foreach($line in $PrefixLines) {
        [void]$allLines.Add($line)
    }
    foreach($line in $WeatherLines) {
        [void]$allLines.Add($line)
    }
    foreach($line in $SuffixLines) {
        [void]$allLines.Add($line)
    }

    Set-Content -Path $Path -Value $allLines -Encoding UTF8
}

$weatherLines = Get-WeatherPhraseLines -Path $weatherPhrases
$degreeC = [string][char]0x2103
$commonSuffix = "-/:%$degreeC"

Write-CharListFile -Path $chars18 `
                  -PrefixLines @() `
                  -WeatherLines $weatherLines `
                  -SuffixLines @($commonSuffix, "0123456789", "AMP")

Write-CharListFile -Path $chars24 `
                  -PrefixLines @() `
                  -WeatherLines $weatherLines `
                  -SuffixLines @($commonSuffix, "0123456789")

Write-Host "[GEN] chars_18.txt / chars_24.txt regenerated from weather_phrases.txt"

function Get-RangeArgFromCharFile {
    param([string]$Path)

    $raw = Get-Content -Path $Path -Raw -Encoding UTF8
    if ([string]::IsNullOrWhiteSpace($raw)) {
        throw "Char file is empty: $Path"
    }

    $set = New-Object 'System.Collections.Generic.HashSet[int]'
    $chars = $raw.ToCharArray()

    foreach ($ch in $chars) {
        $cp = [int][char]$ch
        if (($cp -eq 10) -or ($cp -eq 13)) {
            continue
        }
        [void]$set.Add($cp)
    }

    if ($set.Count -eq 0) {
        throw "No valid codepoints in: $Path"
    }

    $arr = New-Object int[] $set.Count
    $set.CopyTo($arr)
    [Array]::Sort($arr)

    $parts = New-Object System.Collections.Generic.List[string]
    $start = $arr[0]
    $prev = $arr[0]

    for ($i = 1; $i -lt $arr.Length; $i++) {
        $cur = $arr[$i]
        if ($cur -eq ($prev + 1)) {
            $prev = $cur
            continue
        }

        if ($start -eq $prev) {
            [void]$parts.Add(("0x{0:X}" -f $start))
        } else {
            [void]$parts.Add(("0x{0:X}-0x{1:X}" -f $start, $prev))
        }

        $start = $cur
        $prev = $cur
    }

    if ($start -eq $prev) {
        [void]$parts.Add(("0x{0:X}" -f $start))
    } else {
        [void]$parts.Add(("0x{0:X}-0x{1:X}" -f $start, $prev))
    }

    return ($parts -join ",")
}

function Get-SymbolArgFromCharFile {
    param([string]$Path)

    $raw = Get-Content -Path $Path -Raw -Encoding UTF8
    if ([string]::IsNullOrWhiteSpace($raw)) {
        throw "Char file is empty: $Path"
    }

    $set = New-Object 'System.Collections.Generic.HashSet[int]'
    $builder = New-Object System.Text.StringBuilder

    foreach($ch in $raw.ToCharArray()) {
        $cp = [int][char]$ch
        if (($cp -eq 10) -or ($cp -eq 13)) {
            continue
        }
        if($set.Add($cp)) {
            [void]$builder.Append($ch)
        }
    }

    if($builder.Length -eq 0) {
        throw "No valid symbols in: $Path"
    }

    return $builder.ToString()
}

$symbols18 = Get-SymbolArgFromCharFile -Path $chars18
$symbols24 = Get-SymbolArgFromCharFile -Path $chars24

$range18 = Get-RangeArgFromCharFile -Path $chars18
$range24 = Get-RangeArgFromCharFile -Path $chars24

$outAbs = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path (Get-Location).Path $OutputDir }
New-Item -Path $outAbs -ItemType Directory -Force | Out-Null

$font18Out = Join-Path $outAbs "FONT18.BIN"
$font24Out = Join-Path $outAbs "FONT24.BIN"

$helpText = & $fontConvCmd -h 2>&1 | Out-String
$supportLvVersion = $false
if ($helpText -match "--lv-version") {
    $supportLvVersion = $true
}

if (-not $supportLvVersion) {
    Write-Warning "Installed lv_font_conv does not expose --lv-version. Keep using fixed lv_font_conv build and validate on-device after generation."
}

$args18 = @("--font", $FontPath, "--size", "18", "--bpp", "$Bpp", "--format", "bin", "--no-compress", "--no-prefilter")
if ($supportLvVersion) {
    $args18 += @("--lv-version", "$LvVersion")
}
$args18 += @("--symbols", $symbols18, "-o", $font18Out)

$args24 = @("--font", $FontPath, "--size", "24", "--bpp", "$Bpp", "--format", "bin", "--no-compress", "--no-prefilter")
if ($supportLvVersion) {
    $args24 += @("--lv-version", "$LvVersion")
}
$args24 += @("--symbols", $symbols24, "-o", $font24Out)

Write-Host "[1/2] Building 18px font..."
& $fontConvCmd @args18
if ($LASTEXITCODE -ne 0) { throw "18px font build failed" }

Write-Host "[2/2] Building 24px font..."
& $fontConvCmd @args24
if ($LASTEXITCODE -ne 0) { throw "24px font build failed" }

$size18 = (Get-Item $font18Out).Length
$size24 = (Get-Item $font24Out).Length

if (($size18 -lt 2048) -or ($size24 -lt 4096)) {
    throw "Font output size abnormal (18=$size18, 24=$size24). Check lv_font_conv version and font file."
}

Write-Host "Done:"
Write-Host " - $font18Out"
Write-Host " - $font24Out"
Write-Host " - size: FONT18=$size18 bytes, FONT24=$size24 bytes"
Write-Host " - symbols: 18=$($symbols18.Length) chars, 24=$($symbols24.Length) chars"
Write-Host " - ranges(debug): 18 parts=$($range18.Split(',').Count), 24 parts=$($range24.Split(',').Count)"
if ($supportLvVersion) {
    Write-Host " - lv-version: $LvVersion"
} else {
    Write-Host " - lv-version: N/A (converter has no flag)"
}
Write-Host "Copy to SD: /font/FONT18.BIN, /font/FONT24.BIN"
