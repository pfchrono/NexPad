[CmdletBinding()]
param(
  [Parameter()]
  [string]$Version = "local",

  [Parameter()]
  [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

function Resolve-MSBuildPath {
  $command = Get-Command msbuild -ErrorAction SilentlyContinue
  if ($command) {
    return $command.Source
  }

  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhere) {
    $match = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
    if ($match) {
      return $match
    }
  }

  throw "MSBuild.exe was not found. Install Visual Studio Build Tools 2022 or run from a shell where MSBuild is already on PATH."
}

function Invoke-SolutionBuild {
  param(
    [Parameter(Mandatory = $true)]
    [string]$MSBuildPath,

    [Parameter(Mandatory = $true)]
    [string]$SolutionPath,

    [Parameter(Mandatory = $true)]
    [string]$Platform
  )

  Write-Host "Building Release|$Platform ..."
  & $MSBuildPath $SolutionPath "/p:Configuration=Release" "/p:Platform=$Platform"
  if ($LASTEXITCODE -ne 0) {
    throw "MSBuild failed for platform $Platform."
  }
}

function Copy-DirectoryContents {
  param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$Destination
  )

  if (!(Test-Path $Source)) {
    return
  }

  New-Item -ItemType Directory -Path $Destination -Force | Out-Null
  Copy-Item (Join-Path $Source "*") $Destination -Recurse -Force
}

function New-ZipFromDirectory {
  param(
    [Parameter(Mandatory = $true)]
    [string]$SourceDirectory,

    [Parameter(Mandatory = $true)]
    [string]$ZipPath
  )

  if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
  }

  Compress-Archive -Path (Join-Path $SourceDirectory "*") -DestinationPath $ZipPath -CompressionLevel Optimal
}

function Write-ChecksumFile {
  param(
    [Parameter(Mandatory = $true)]
    [string]$FilePath
  )

  $hash = Get-FileHash -Path $FilePath -Algorithm SHA256
  $checksumPath = "$FilePath.sha256"
  $line = "{0} *{1}" -f $hash.Hash.ToLowerInvariant(), (Split-Path $FilePath -Leaf)
  Set-Content -Path $checksumPath -Value $line -Encoding ascii
  return $checksumPath
}

$solutionPath = Join-Path $RepoRoot "Windows\NexPad.sln"
$releaseRoot = Join-Path $RepoRoot "release"
$artifactsRoot = Join-Path $RepoRoot "artifacts"
$stagingRoot = Join-Path $artifactsRoot "staging"

if (!(Test-Path $solutionPath)) {
  throw "Solution not found at $solutionPath"
}

Remove-Item $artifactsRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $releaseRoot -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $RepoRoot "x64") -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item (Join-Path $RepoRoot "NexPad.exe") -Force -ErrorAction SilentlyContinue

New-Item -ItemType Directory -Path $artifactsRoot -Force | Out-Null
New-Item -ItemType Directory -Path $stagingRoot -Force | Out-Null

$msbuildPath = Resolve-MSBuildPath
Invoke-SolutionBuild -MSBuildPath $msbuildPath -SolutionPath $solutionPath -Platform "Win32"
Invoke-SolutionBuild -MSBuildPath $msbuildPath -SolutionPath $solutionPath -Platform "x64"

$win32Stage = Join-Path $stagingRoot "win32"
$x64Stage = Join-Path $stagingRoot "x64"

New-Item -ItemType Directory -Path $win32Stage -Force | Out-Null
New-Item -ItemType Directory -Path $x64Stage -Force | Out-Null

Copy-Item (Join-Path $releaseRoot "NexPad.exe") $win32Stage -Force
Copy-Item (Join-Path $releaseRoot "config.ini") $win32Stage -Force
Copy-Item (Join-Path $releaseRoot "README.md") $win32Stage -Force
Copy-Item (Join-Path $releaseRoot "LICENSE") $win32Stage -Force
Copy-DirectoryContents -Source (Join-Path $releaseRoot "presets") -Destination (Join-Path $win32Stage "presets")

Copy-Item (Join-Path $releaseRoot "x64\NexPad.exe") $x64Stage -Force
Copy-Item (Join-Path $releaseRoot "x64\config.ini") $x64Stage -Force
Copy-Item (Join-Path $releaseRoot "README.md") $x64Stage -Force
Copy-Item (Join-Path $releaseRoot "LICENSE") $x64Stage -Force
Copy-DirectoryContents -Source (Join-Path $releaseRoot "x64\presets") -Destination (Join-Path $x64Stage "presets")

$win32Zip = Join-Path $artifactsRoot ("NexPad-win32-{0}.zip" -f $Version)
$x64Zip = Join-Path $artifactsRoot ("NexPad-x64-{0}.zip" -f $Version)

New-ZipFromDirectory -SourceDirectory $win32Stage -ZipPath $win32Zip
New-ZipFromDirectory -SourceDirectory $x64Stage -ZipPath $x64Zip

$win32Checksum = Write-ChecksumFile -FilePath $win32Zip
$x64Checksum = Write-ChecksumFile -FilePath $x64Zip

Write-Host "Packaged artifacts:"
Write-Host " - $win32Zip"
Write-Host " - $x64Zip"
Write-Host " - $win32Checksum"
Write-Host " - $x64Checksum"
