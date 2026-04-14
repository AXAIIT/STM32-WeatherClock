Add-Type -AssemblyName System.Drawing

$fontPath = 'C:\Windows\Fonts\simhei.ttf'
$outPath = 'c:\Users\GY\Desktop\My_Project\Weather_Clock\6_LVGL8.3_STM32F407ZGT6_Gui_Guider_FreeRTOS\App\LVGL_myGui\src\generated\guider_fonts\lv_font_WeekdayCN_Subset_30.c'

$chars = @([char]0x661F,[char]0x671F,[char]0x4E00,[char]0x4E8C,[char]0x4E09,[char]0x56DB,[char]0x4E94,[char]0x516D,[char]0x65E5)
$fontSizePx = 30
$canvasW = 96
$canvasH = 96

$font = New-Object System.Drawing.Text.PrivateFontCollection
$font.AddFontFile($fontPath)
$drawFont = New-Object System.Drawing.Font($font.Families[0], $fontSizePx, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)

$bmpBytes = New-Object System.Collections.Generic.List[byte]
$glyphDescs = @()

$tmpBmp = New-Object System.Drawing.Bitmap($canvasW, $canvasH, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($tmpBmp)
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$g.Clear([System.Drawing.Color]::Transparent)

$lineHeight = [Math]::Ceiling($drawFont.GetHeight($g))
$baseLine = [Math]::Ceiling($drawFont.FontFamily.GetCellAscent([System.Drawing.FontStyle]::Regular) * $drawFont.Size / $drawFont.FontFamily.GetEmHeight([System.Drawing.FontStyle]::Regular))

$whiteBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)

foreach($ch in $chars) {
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.DrawString($ch, $drawFont, $whiteBrush, 0, 0)

    $minX = $canvasW
    $minY = $canvasH
    $maxX = -1
    $maxY = -1

    for($y=0; $y -lt $canvasH; $y++) {
        for($x=0; $x -lt $canvasW; $x++) {
            $px = $tmpBmp.GetPixel($x, $y)
            if($px.A -gt 0) {
                if($x -lt $minX) { $minX = $x }
                if($y -lt $minY) { $minY = $y }
                if($x -gt $maxX) { $maxX = $x }
                if($y -gt $maxY) { $maxY = $y }
            }
        }
    }

    if($maxX -lt 0 -or $maxY -lt 0) {
        $glyphDescs += [PSCustomObject]@{
            code = [int][char]$ch
            bitmap_index = $bmpBytes.Count
            adv_w = 0
            box_w = 0
            box_h = 0
            ofs_x = 0
            ofs_y = 0
        }
        continue
    }

    $boxW = $maxX - $minX + 1
    $boxH = $maxY - $minY + 1

    $measure = $g.MeasureString($ch, $drawFont, 1000, [System.Drawing.StringFormat]::GenericTypographic)
    $advW = 480

    $bitmapIndex = $bmpBytes.Count

    for($y=0; $y -lt $boxH; $y++) {
        $x = 0
        while($x -lt $boxW) {
            $p1 = $tmpBmp.GetPixel($minX + $x, $minY + $y)
            $v1 = [Math]::Round($p1.A / 17.0)
            if($v1 -lt 0) { $v1 = 0 }
            if($v1 -gt 15) { $v1 = 15 }

            $v2 = 0
            if(($x + 1) -lt $boxW) {
                $p2 = $tmpBmp.GetPixel($minX + $x + 1, $minY + $y)
                $v2 = [Math]::Round($p2.A / 17.0)
                if($v2 -lt 0) { $v2 = 0 }
                if($v2 -gt 15) { $v2 = 15 }
            }

            $b = (($v1 -band 0xF) -shl 4) -bor ($v2 -band 0xF)
            $bmpBytes.Add([byte]$b)
            $x += 2
        }
    }

    $ofsY = $baseLine - ($minY + $boxH)

    $glyphDescs += [PSCustomObject]@{
        code = [int][char]$ch
        bitmap_index = $bitmapIndex
        adv_w = [int]$advW
        box_w = [int]$boxW
        box_h = [int]$boxH
        ofs_x = 0
        ofs_y = [int]$ofsY
    }
}

$g.Dispose()
$tmpBmp.Dispose()
$whiteBrush.Dispose()
$drawFont.Dispose()

$sorted = $glyphDescs | Sort-Object code
$minCode = ($sorted | Select-Object -First 1).code
$maxCode = ($sorted | Select-Object -Last 1).code

$unicodeOffsets = @()
foreach($it in $sorted) {
    $unicodeOffsets += ($it.code - $minCode)
}

$sb = New-Object System.Text.StringBuilder

[void]$sb.AppendLine('#include "lvgl.h"')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#ifndef LV_FONT_WEEKDAY_CN_SUBSET_30')
[void]$sb.AppendLine('#define LV_FONT_WEEKDAY_CN_SUBSET_30 1')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#if LV_FONT_WEEKDAY_CN_SUBSET_30')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {')

for($i=0; $i -lt $bmpBytes.Count; $i += 16) {
    $chunk = $bmpBytes[$i..([Math]::Min($i+15, $bmpBytes.Count-1))]
    $line = ($chunk | ForEach-Object { ('0x{0:X2}' -f $_) }) -join ', '
    [void]$sb.AppendLine("    $line,")
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {')
[void]$sb.AppendLine('    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},')

$gid = 1
foreach($it in $sorted) {
    [void]$sb.AppendLine("    {.bitmap_index = $($it.bitmap_index), .adv_w = $($it.adv_w), .box_w = $($it.box_w), .box_h = $($it.box_h), .ofs_x = $($it.ofs_x), .ofs_y = $($it.ofs_y)},")
    $gid++
}
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')

$offsetHex = ($unicodeOffsets | ForEach-Object { '0x{0:X}' -f $_ }) -join ', '
[void]$sb.AppendLine("static const uint16_t unicode_list_0[] = {$offsetHex};")
[void]$sb.AppendLine('')
[void]$sb.AppendLine('static const lv_font_fmt_txt_cmap_t cmaps[] =')
[void]$sb.AppendLine('{')
[void]$sb.AppendLine('    {')
[void]$sb.AppendLine("        .range_start = $minCode, .range_length = $($maxCode - $minCode + 1), .glyph_id_start = 1,")
[void]$sb.AppendLine("        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = $($unicodeOffsets.Count), .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY")
[void]$sb.AppendLine('    }')
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#if LVGL_VERSION_MAJOR == 8')
[void]$sb.AppendLine('static lv_font_fmt_txt_glyph_cache_t cache;')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#if LVGL_VERSION_MAJOR >= 8')
[void]$sb.AppendLine('static const lv_font_fmt_txt_dsc_t font_dsc = {')
[void]$sb.AppendLine('#else')
[void]$sb.AppendLine('static lv_font_fmt_txt_dsc_t font_dsc = {')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('    .glyph_bitmap = glyph_bitmap,')
[void]$sb.AppendLine('    .glyph_dsc = glyph_dsc,')
[void]$sb.AppendLine('    .cmaps = cmaps,')
[void]$sb.AppendLine('    .kern_dsc = NULL,')
[void]$sb.AppendLine('    .kern_scale = 0,')
[void]$sb.AppendLine('    .cmap_num = 1,')
[void]$sb.AppendLine('    .bpp = 4,')
[void]$sb.AppendLine('    .kern_classes = 0,')
[void]$sb.AppendLine('    .bitmap_format = 0,')
[void]$sb.AppendLine('#if LVGL_VERSION_MAJOR == 8')
[void]$sb.AppendLine('    .cache = &cache')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#if LVGL_VERSION_MAJOR >= 8')
[void]$sb.AppendLine('const lv_font_t lv_font_WeekdayCN_Subset_30 = {')
[void]$sb.AppendLine('#else')
[void]$sb.AppendLine('lv_font_t lv_font_WeekdayCN_Subset_30 = {')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,')
[void]$sb.AppendLine('    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,')
[void]$sb.AppendLine("    .line_height = $lineHeight,")
[void]$sb.AppendLine("    .base_line = $([Math]::Max(0, $lineHeight - $baseLine)),")
[void]$sb.AppendLine('#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)')
[void]$sb.AppendLine('    .subpx = LV_FONT_SUBPX_NONE,')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8')
[void]$sb.AppendLine('    .underline_position = -2,')
[void]$sb.AppendLine('    .underline_thickness = 1,')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('    .dsc = &font_dsc,')
[void]$sb.AppendLine('#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9')
[void]$sb.AppendLine('    .fallback = NULL,')
[void]$sb.AppendLine('#endif')
[void]$sb.AppendLine('    .user_data = NULL,')
[void]$sb.AppendLine('};')
[void]$sb.AppendLine('')
[void]$sb.AppendLine('#endif')

[IO.File]::WriteAllText($outPath, $sb.ToString(), [Text.Encoding]::UTF8)
Write-Host "Generated: $outPath"