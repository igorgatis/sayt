$ErrorActionPreference = "Stop"

$SaytBinary = "sayt.com"
$SaytUrl = "https://github.com/igorgatis/sayt/releases/latest/download/$SaytBinary"

function Get-CacheDir {
  if ($env:LOCALAPPDATA) {
    return Join-Path $env:LOCALAPPDATA "sayt"
  } elseif ($env:TEMP) {
    return Join-Path $env:TEMP "sayt"
  } else {
    return "C:\Temp\sayt"
  }
}

$CacheDir = Get-CacheDir

function Find-Sayt {
  $localPath = Join-Path -Path "." -ChildPath $SaytBinary
  if (Test-Path $localPath) {
    return (Resolve-Path $localPath).Path
  }

  $cachePath = Join-Path -Path $CacheDir -ChildPath $SaytBinary
  if (Test-Path $cachePath) {
    return $cachePath
  }

  $inPath = Get-Command $SaytBinary -ErrorAction SilentlyContinue
  if ($inPath) {
    return $inPath.Source
  }

  return $null
}

function Download-Sayt {
  if (-not (Test-Path $CacheDir)) {
    New-Item -ItemType Directory -Path $CacheDir -Force | Out-Null
  }
  $dest = Join-Path -Path $CacheDir -ChildPath $SaytBinary

  Write-Host "Downloading $SaytBinary from $SaytUrl..." -ForegroundColor Yellow

  try {
    Invoke-WebRequest -Uri $SaytUrl -OutFile $dest -UseBasicParsing
  } catch {
    Write-Error "Failed to download $SaytBinary: $_"
    exit 1
  }

  return $dest
}

$saytPath = Find-Sayt
if (-not $saytPath) {
  $saytPath = Download-Sayt
}

& $saytPath @args
