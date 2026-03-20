[CmdletBinding()]
param(
    [Parameter()]
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\Windows\NexPad\NexPad.ico"),

    [Parameter()]
    [string]$PreviewPngPath = (Join-Path $PSScriptRoot "..\docs\assets\nexpad-icon-preview.png")
)

$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

function New-RoundedRectanglePath {
    param(
        [Parameter(Mandatory = $true)]
        [float]$X,

        [Parameter(Mandatory = $true)]
        [float]$Y,

        [Parameter(Mandatory = $true)]
        [float]$Width,

        [Parameter(Mandatory = $true)]
        [float]$Height,

        [Parameter(Mandatory = $true)]
        [float]$Radius
    )

    $diameter = $Radius * 2.0
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddArc($X, $Y, $diameter, $diameter, 180, 90)
    $path.AddArc($X + $Width - $diameter, $Y, $diameter, $diameter, 270, 90)
    $path.AddArc($X + $Width - $diameter, $Y + $Height - $diameter, $diameter, $diameter, 0, 90)
    $path.AddArc($X, $Y + $Height - $diameter, $diameter, $diameter, 90, 90)
    $path.CloseFigure()
    return $path
}

function New-CrossPath {
    param(
        [Parameter(Mandatory = $true)]
        [float]$CenterX,

        [Parameter(Mandatory = $true)]
        [float]$CenterY,

        [Parameter(Mandatory = $true)]
        [float]$ArmLength,

        [Parameter(Mandatory = $true)]
        [float]$Thickness
    )

    $half = $Thickness / 2.0
    $horizontal = New-Object System.Drawing.RectangleF ($CenterX - $ArmLength), ($CenterY - $half), ($ArmLength * 2.0), $Thickness
    $vertical = New-Object System.Drawing.RectangleF ($CenterX - $half), ($CenterY - $ArmLength), $Thickness, ($ArmLength * 2.0)
    $path = New-Object System.Drawing.Drawing2D.GraphicsPath
    $path.AddRectangle($horizontal)
    $path.AddRectangle($vertical)
    return $path
}

function Get-Bitmap {
    param(
        [Parameter(Mandatory = $true)]
        [int]$Size
    )

    $bitmap = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([System.Drawing.Color]::Transparent)

    $rect = New-Object System.Drawing.RectangleF 0, 0, $Size, $Size
    $pad = [float]($Size * 0.08)
    $cardSize = [float]($Size - ($pad * 2.0))
    $radius = [float]($Size * 0.22)
    $cardPath = New-RoundedRectanglePath -X $pad -Y $pad -Width $cardSize -Height $cardSize -Radius $radius

    $bgTop = [System.Drawing.Color]::FromArgb(255, 11, 18, 32)
    $bgBottom = [System.Drawing.Color]::FromArgb(255, 18, 88, 122)
    $bgBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $rect, $bgTop, $bgBottom, 90.0
    $graphics.FillPath($bgBrush, $cardPath)

    $shineRect = New-Object System.Drawing.RectangleF $pad, ($pad - ($Size * 0.08)), $cardSize, ($cardSize * 0.65)
    $shineBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush $shineRect,
    ([System.Drawing.Color]::FromArgb(130, 255, 255, 255)),
    ([System.Drawing.Color]::FromArgb(0, 255, 255, 255)),
    90.0
    $graphics.FillPath($shineBrush, $cardPath)

    $borderPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(100, 210, 245, 255)), ([float]($Size * 0.03))
    $graphics.DrawPath($borderPen, $cardPath)

    $crossPath = New-CrossPath -CenterX ([float]($Size * 0.31)) -CenterY ([float]($Size * 0.52)) -ArmLength ([float]($Size * 0.11)) -Thickness ([float]($Size * 0.075))
    $crossBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 88, 235, 255))
    $graphics.FillPath($crossBrush, $crossPath)
    $crossCoreBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 214, 251, 255))
    $graphics.FillEllipse($crossCoreBrush, [float]($Size * 0.275), [float]($Size * 0.485), [float]($Size * 0.07), [float]($Size * 0.07))

    $ringPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(255, 255, 138, 92)), ([float]($Size * 0.06))
    $graphics.DrawEllipse($ringPen, [float]($Size * 0.59), [float]($Size * 0.39), [float]($Size * 0.21), [float]($Size * 0.21))
    $stickBrush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 255, 214, 198))
    $graphics.FillEllipse($stickBrush, [float]($Size * 0.655), [float]($Size * 0.455), [float]($Size * 0.08), [float]($Size * 0.08))

    $nPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(245, 255, 255, 255)), ([float]($Size * 0.085))
    $nPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $nPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $graphics.DrawLine($nPen, [float]($Size * 0.33), [float]($Size * 0.72), [float]($Size * 0.33), [float]($Size * 0.28))
    $graphics.DrawLine($nPen, [float]($Size * 0.33), [float]($Size * 0.28), [float]($Size * 0.64), [float]($Size * 0.72))
    $graphics.DrawLine($nPen, [float]($Size * 0.64), [float]($Size * 0.72), [float]($Size * 0.64), [float]($Size * 0.33))

    $accentPen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(180, 34, 204, 255)), ([float]($Size * 0.022))
    $graphics.DrawArc($accentPen, [float]($Size * 0.13), [float]($Size * 0.12), [float]($Size * 0.74), [float]($Size * 0.74), 205, 110)

    $accentPen.Dispose()
    $nPen.Dispose()
    $stickBrush.Dispose()
    $ringPen.Dispose()
    $crossCoreBrush.Dispose()
    $crossBrush.Dispose()
    $crossPath.Dispose()
    $borderPen.Dispose()
    $shineBrush.Dispose()
    $bgBrush.Dispose()
    $cardPath.Dispose()
    $graphics.Dispose()

    return $bitmap
}

function Get-IconImageBytes {
    param(
        [Parameter(Mandatory = $true)]
        [System.Drawing.Bitmap]$Bitmap
    )

    $width = $Bitmap.Width
    $height = $Bitmap.Height
    $maskStride = [int]([Math]::Ceiling($width / 32.0) * 4)
    $pixelBytesLength = $width * $height * 4
    $maskBytesLength = $maskStride * $height
    $imageSize = 40 + $pixelBytesLength + $maskBytesLength

    $stream = New-Object System.IO.MemoryStream
    $writer = New-Object System.IO.BinaryWriter $stream

    $writer.Write([UInt32]40)
    $writer.Write([Int32]$width)
    $writer.Write([Int32]($height * 2))
    $writer.Write([UInt16]1)
    $writer.Write([UInt16]32)
    $writer.Write([UInt32]0)
    $writer.Write([UInt32]($pixelBytesLength + $maskBytesLength))
    $writer.Write([Int32]0)
    $writer.Write([Int32]0)
    $writer.Write([UInt32]0)
    $writer.Write([UInt32]0)

    for ($y = $height - 1; $y -ge 0; $y--) {
        for ($x = 0; $x -lt $width; $x++) {
            $pixel = $Bitmap.GetPixel($x, $y)
            $writer.Write([byte]$pixel.B)
            $writer.Write([byte]$pixel.G)
            $writer.Write([byte]$pixel.R)
            $writer.Write([byte]$pixel.A)
        }
    }

    $maskBytes = New-Object byte[] $maskBytesLength
    $writer.Write($maskBytes)

    $writer.Flush()
    $bytes = $stream.ToArray()
    $writer.Dispose()
    $stream.Dispose()

    return $bytes
}

$previewDirectory = Split-Path -Parent $PreviewPngPath
if (!(Test-Path $previewDirectory)) {
    New-Item -ItemType Directory -Path $previewDirectory -Force | Out-Null
}

$masterBitmap = Get-Bitmap -Size 1024
$masterBitmap.Save($PreviewPngPath, [System.Drawing.Imaging.ImageFormat]::Png)
$masterBitmap.Dispose()

$outputDirectory = Split-Path -Parent $OutputPath
if (!(Test-Path $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}

$magickCommand = Get-Command magick -ErrorAction SilentlyContinue
if (!$magickCommand) {
    throw "ImageMagick is required to generate NexPad.ico. Install it with winget install --id ImageMagick.Q16 --exact"
}

$convertArguments = @(
    $PreviewPngPath,
    '-background', 'none',
    '-define', 'icon:auto-resize=256,128,64,48,32,24,16',
    $OutputPath
)

& $magickCommand.Source @convertArguments
if ($LASTEXITCODE -ne 0) {
    throw "ImageMagick failed while generating $OutputPath"
}

Write-Host "Wrote preview PNG to $PreviewPngPath"
Write-Host "Wrote icon to $OutputPath"