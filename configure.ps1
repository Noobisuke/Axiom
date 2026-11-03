<##
.SYNOPSIS
    Configure Axiom Chess Engine build using CMake presets.

.DESCRIPTION
    This script configures the Axiom project using CMake presets defined in CMakePresets.json.
    Presets: release (default), debug, relwithdebinfo

.PARAMETER Preset
    The CMake preset to use. Valid values: release, debug, relwithdebinfo

.EXAMPLE
    .\configure.ps1           # Release build
    .\configure.ps1 debug     # Debug build with sanitizers
    .\configure.ps1 relwithdebinfo  # Optimized with debug symbols
#>

param(
    [ValidateSet('release','debug','relwithdebinfo')]
    [string]$Preset = 'release'
)

$ninjaPath = "C:\Users\Aryan\chess-engine\tools\ninja.exe"
if (-not (Test-Path $ninjaPath)) {
    Write-Error "Ninja not found at $ninjaPath. Download from https://github.com/ninja-build/ninja/releases"
    exit 1
}

cmake --preset=$Preset -DCMAKE_MAKE_PROGRAM=$ninjaPath