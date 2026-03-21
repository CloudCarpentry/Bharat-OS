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

function Get-CanonicalConfigValue([string]$RawValue) {
    if ([string]::IsNullOrWhiteSpace($RawValue) -or $RawValue -eq "null") {
        return ""
    }
    return (($RawValue -split ",")[0]).Trim().ToUpperInvariant()
}

$ProfileCanonical = Get-CanonicalConfigValue $Profile
$PersonalityCanonical = Get-CanonicalConfigValue $Personality
$BoardCanonical = if ([string]::IsNullOrWhiteSpace($Board) -or $Board -eq "null") { "" } else { (($Board -split ",")[0]).Trim() }

Write-Host "Configuring: $Preset (Arch: $Arch, Profile: $ProfileCanonical, Personality: $PersonalityCanonical, Board: $BoardCanonical)"

cmake --preset $Preset `
    -DBHARAT_ARCH_FAMILY=$Arch `
    -DBHARAT_DEVICE_PROFILE=$ProfileCanonical `
    -DBHARAT_PERSONALITY_PROFILE=$PersonalityCanonical `
    -DBHARAT_TARGET_BOARD=$BoardCanonical `
    -DBHARAT_BOOT_GUI=$GuiFlag

Write-Host "Building..."
cmake --build --preset $Preset

if ($Run -or ($RunYaml -eq "true")) {
    Write-Host "Running (assuming target is executable for host or qemu is in path)..."
    # Placeholder for platform-specific launch commands similar to Linux script
    if ($Arch -eq "arm32" -and $Board -eq "avh-corstone310") {
        VHT_Corstone_SSE-310.exe -a build/$Preset/kernel.elf
    }
}
