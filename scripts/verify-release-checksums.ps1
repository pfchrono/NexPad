[CmdletBinding()]
param(
  [Parameter()]
  [string]$ArtifactsRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\artifacts")).Path
)

$ErrorActionPreference = 'Stop'

if (!(Test-Path $ArtifactsRoot)) {
  throw "Artifacts directory not found: $ArtifactsRoot"
}

$checksumFiles = Get-ChildItem -Path $ArtifactsRoot -Filter '*.sha256' -File | Sort-Object Name
if ($checksumFiles.Count -eq 0) {
  throw "No checksum files were found under $ArtifactsRoot"
}

foreach ($checksumFile in $checksumFiles) {
  $content = (Get-Content -Path $checksumFile.FullName -Raw).Trim()
  if ([string]::IsNullOrWhiteSpace($content)) {
    throw "Checksum file is empty: $($checksumFile.Name)"
  }

  $parts = $content -split '\s+\*', 2
  if ($parts.Count -ne 2) {
    throw "Checksum file format is invalid: $($checksumFile.Name)"
  }

  $expectedHash = $parts[0].Trim().ToLowerInvariant()
  $targetName = $parts[1].Trim()
  $targetPath = Join-Path $ArtifactsRoot $targetName

  if (!(Test-Path $targetPath)) {
    throw "Referenced artifact was not found for $($checksumFile.Name): $targetName"
  }

  $actualHash = (Get-FileHash -Path $targetPath -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($actualHash -ne $expectedHash) {
    throw "Checksum mismatch for $targetName"
  }

  Write-Host "Verified $targetName"
}

Write-Host 'All artifact checksum files verified successfully.'
