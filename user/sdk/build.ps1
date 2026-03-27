<#
.SYNOPSIS
Bharat-OS SDK Build Script for Windows.

.DESCRIPTION
Configures and builds the Bharat-OS SDK and sample application.

.PARAMETER Arch
The target architecture (default: x86_64).

.PARAMETER Clean
Cleans the build directory before building.

.EXAMPLE
.\build.ps1 -Arch arm64 -Clean
#>

param (
    [string]$Arch = "x86_64",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

# Environment validation
if (-not (Get-Command "cmake" -ErrorAction SilentlyContinue)) {
    Write-Error "Error: 'cmake' is not installed or not in PATH."
    exit 1
}

$BuildDir = "build\$Arch"
$ToolchainFile = "..\..\cmake\toolchains\${Arch}-elf.cmake"

if (-not (Test-Path $ToolchainFile)) {
    Write-Error "Error: Toolchain file not found at $ToolchainFile. Ensure you are running this from the user\sdk\ directory."
    exit 1
}

if ($Clean) {
    if (Test-Path $BuildDir) {
        Write-Host "Cleaning build directory: $BuildDir"
        Remove-Item -Recurse -Force $BuildDir
    }
}

Write-Host "========================================"
Write-Host " Building Bharat-OS SDK                 "
Write-Host " Architecture: $Arch"
Write-Host " Build Dir:    $BuildDir"
Write-Host "========================================"

Write-Host "Configuring CMake..."
$cmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-DCMAKE_TOOLCHAIN_FILE=$ToolchainFile"
)

# Call CMake to configure
$process = Start-Process -FilePath "cmake" -ArgumentList $cmakeArgs -NoNewWindow -Wait -PassThru
if ($process.ExitCode -ne 0) {
    Write-Error "Error: CMake configuration failed."
    exit $process.ExitCode
}

Write-Host "Building..."
$buildArgs = @(
    "--build", $BuildDir
)
$process = Start-Process -FilePath "cmake" -ArgumentList $buildArgs -NoNewWindow -Wait -PassThru
if ($process.ExitCode -ne 0) {
    Write-Error "Error: Build failed."
    exit $process.ExitCode
}

Write-Host "========================================"
Write-Host " Build successful!                      "
Write-Host " SDK and sample app are in $BuildDir    "
Write-Host "========================================"
