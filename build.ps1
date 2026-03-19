<#
.SYNOPSIS
    Bharat-OS Build Wrapper Script
.DESCRIPTION
    Wraps the underlying CMake presets mapped from a custom build_config.yml
#>

param (
    [string]$BuildConfig = "default_dev",
    [switch]$Run
)

$YamlPath = "build_config.yml"
if (-not (Get-Command yq -ErrorAction SilentlyContinue)) {
    Write-Host "yq could not be found. Please install it to parse build_config.yml or fallback will be used."
    Write-Host "Running preset 'windows-hosttools-debug' as fallback..."
    cmake --preset windows-hosttools-debug
    cmake --build --preset windows-hosttools-debug
    exit
}

# Ensure the config exists
$CheckConfig = (yq eval ".builds.$BuildConfig" $YamlPath)
if ($CheckConfig -match "null") {
    Write-Host "Build config '$BuildConfig' not found in build_config.yml"
    exit 1
}

$Preset = (yq eval ".builds.$BuildConfig.preset" $YamlPath)
$Arch = (yq eval ".builds.$BuildConfig.arch" $YamlPath)
$Profile = (yq eval ".builds.$BuildConfig.profile" $YamlPath)
$Personality = (yq eval ".builds.$BuildConfig.personality" $YamlPath)
$Board = (yq eval ".builds.$BuildConfig.board" $YamlPath)
$Gui = (yq eval ".builds.$BuildConfig.gui" $YamlPath)
$RunYaml = (yq eval ".builds.$BuildConfig.run" $YamlPath)

$GuiFlag = "OFF"
if ($Gui -eq "true") { $GuiFlag = "ON" }

Write-Host "Configuring: $Preset (Arch: $Arch, Profile: $Profile, Personality: $Personality, Board: $Board)"

cmake --preset $Preset `
    -DBHARAT_ARCH_FAMILY=$Arch `
    -DBHARAT_DEVICE_PROFILE=$Profile `
    -DBHARAT_PERSONALITY_PROFILE=$Personality `
    -DBHARAT_BOOT_GUI=$GuiFlag

Write-Host "Building..."
cmake --build --preset $Preset

if ($Run -or ($RunYaml -eq "true")) {
    Write-Host "Running (assuming target is executable for host or qemu is in path)..."
    # Placeholder for platform-specific launch commands similar to Linux script
}
