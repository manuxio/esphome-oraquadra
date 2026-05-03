# OraQuadra ESPHome build wrapper
#
# Why this script:
#   - Mounts a persistent /cache volume so PlatformIO toolchain (~3 GB) is not
#     re-downloaded between runs.
#   - Enables PlatformIO's content-hash build cache (esphome.platformio_options
#     in YAML) — unchanged .cpp files skip compilation entirely on subsequent
#     builds (drops ~5 min builds to ~30 s).
#
# Usage:
#   .\build.ps1                      → compile (default)
#   .\build.ps1 logs                 → tail device logs over network
#   .\build.ps1 upload               → flash via OTA / serial
#   .\build.ps1 run                  → compile + upload + logs (full cycle)
#   .\build.ps1 clean                → wipe build artifacts (NOT the toolchain cache)
#   .\build.ps1 nuke                 → wipe everything including .docker_cache (~3 GB)

param(
    [string]$Action = "compile",
    [string]$YamlFile = "oraquadra.yaml"
)

# NOTE: do NOT set $ErrorActionPreference = "Stop". Docker writes informational
# messages to stderr (e.g. "INFO ESPHome 2026.4.3") and PowerShell would
# interpret those as fatal errors under Stop, killing the build immediately.

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$CacheDir    = Join-Path $ProjectRoot ".docker_cache"
$Yaml        = $YamlFile
$Image       = "ghcr.io/esphome/esphome:latest"

# Ensure cache dir exists (it survives across runs)
if (-not (Test-Path $CacheDir)) {
    Write-Host "Creating fresh cache dir: $CacheDir"
    New-Item -ItemType Directory -Force -Path $CacheDir | Out-Null
}

switch ($Action) {
    "clean" {
        Write-Host "Cleaning build artifacts (keeping toolchain cache)…"
        $build = Join-Path $ProjectRoot ".esphome\build"
        if (Test-Path $build) { cmd /c rd /s /q $build }
        Write-Host "Done. Next build will re-link but reuse cached objects."
        exit 0
    }
    "nuke" {
        Write-Host "Wiping EVERYTHING (toolchain + build + cache)…"
        $esphome = Join-Path $ProjectRoot ".esphome"
        if (Test-Path $esphome)  { cmd /c rd /s /q $esphome }
        if (Test-Path $CacheDir) { cmd /c rd /s /q $CacheDir }
        Write-Host "Gone. Next build will re-download ~3 GB of toolchain."
        exit 0
    }
}

# Compile / upload / logs / run all use the same docker invocation
$dockerArgs = @(
    "run", "--rm",
    "-v", "${ProjectRoot}:/config",
    "-v", "${CacheDir}:/cache",
    "-e", "PLATFORMIO_BUILD_CACHE_DIR=/cache/pio_build_cache",
    $Image, $Action, $Yaml
)

# Interactive actions (logs, run) need a TTY
if ($Action -in @("logs", "run", "upload")) {
    $dockerArgs = @($dockerArgs[0]) + @("-it") + $dockerArgs[1..($dockerArgs.Length - 1)]
}

Write-Host "→ docker $($dockerArgs -join ' ')"
& docker @dockerArgs
exit $LASTEXITCODE
