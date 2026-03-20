[CmdletBinding()]
param(
  [Parameter()]
  [switch]$Clean,

  [Parameter()]
  [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = 'Stop'

function Resolve-MSBuildPath {
  $command = Get-Command msbuild -ErrorAction SilentlyContinue
  if ($command) {
    return $command.Source
  }

  $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path $vswhere) {
    $match = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    if ($match) {
      return $match
    }
  }

  throw 'MSBuild.exe was not found. Install Visual Studio Build Tools 2022 or add MSBuild to PATH.'
}

function Invoke-SolutionTarget {
  param(
    [Parameter(Mandatory = $true)]
    [string]$MSBuildPath,

    [Parameter(Mandatory = $true)]
    [string]$SolutionPath,

    [Parameter(Mandatory = $true)]
    [string]$Configuration,

    [Parameter(Mandatory = $true)]
    [string]$Platform,

    [Parameter()]
    [string]$Target = 'Build'
  )

  Write-Host '============================================================'
  Write-Host "$Target $Configuration|$Platform"
  Write-Host '============================================================'
  & $MSBuildPath $SolutionPath "/t:$Target" "/p:Configuration=$Configuration" "/p:Platform=$Platform"
  if ($LASTEXITCODE -ne 0) {
    throw "$Target failed for $Configuration|$Platform"
  }
  Write-Host ''
}

$solutionPath = Join-Path $RepoRoot 'Windows\NexPad.sln'
if (!(Test-Path $solutionPath)) {
  throw "Solution not found at $solutionPath"
}

$msbuildPath = Resolve-MSBuildPath
Write-Host "Using MSBuild: $msbuildPath"
Write-Host "Solution: $solutionPath"
Write-Host ''

$configurations = @('Debug', 'Release')
$platforms = @('Win32', 'x64')

foreach ($configuration in $configurations) {
  foreach ($platform in $platforms) {
    if ($Clean) {
      Invoke-SolutionTarget -MSBuildPath $msbuildPath -SolutionPath $solutionPath -Configuration $configuration -Platform $platform -Target 'Clean'
    }

    Invoke-SolutionTarget -MSBuildPath $msbuildPath -SolutionPath $solutionPath -Configuration $configuration -Platform $platform -Target 'Build'
  }
}

if ($Clean) {
  Write-Host 'Completed clean and build for all available solution targets.'
}
else {
  Write-Host 'Completed build for all available solution targets.'
}
