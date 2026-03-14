#Requires -Version 5.1
<#
.SYNOPSIS
    Bharat-OS kernel build and QEMU run script for Windows (PowerShell 5+)
.PARAMETER Arch
    Target architecture: x86_64 (default), riscv64
.PARAMETER Clean
    Remove the build directory before building
.PARAMETER Run
    Boot the compiled kernel in QEMU after a successful build
.EXAMPLE
    .\tools\build.ps1
    .\tools\build.ps1 -Arch riscv64
    .\tools\build.ps1 -Arch x86_64 -Clean -Run
    .\tools\build.ps1 -Arch x86_64 -BootGui OFF -HardwareProfile vm
#>
param(
    [string]$Board = "",
    [string]$Arch = "",
    [string]$ToolchainOverride = "",
    [switch]$Clean = $false,
    [switch]$Run = $false,
    [switch]$DebugQemu = $false,
    [switch]$Payload = $false,
    [switch]$Flash = $false,
    [string]$Machine = "",
    [ValidateSet("ON", "OFF")][string]$BootGui = "ON",
    [string]$HardwareProfile = "",
    [string]$BootTier = "",
    [string]$Profile = "desktop",
    [string]$Personality = "none"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot

$BoardsJsonPath = "$Root\tools\boards\boards.json"
$BoardInfo = $null

if ($Board -ne "") {
    if (-not (Test-Path $BoardsJsonPath)) {
        Write-Host "  [!] Error: boards.json not found at $BoardsJsonPath" -ForegroundColor Red
        exit 1
    }

    $BoardsData = Get-Content $BoardsJsonPath | ConvertFrom-Json

    if (-not $BoardsData.boards.PSObject.Properties.Match($Board).Count) {
        Write-Host "  [!] Error: Board '$Board' not found in boards.json" -ForegroundColor Red
        exit 1
    }

    $BoardInfo = $BoardsData.boards.$Board

    if ($Arch -ne "" -and $Arch -ne $BoardInfo.arch) {
        Write-Host "  [!] Error: Board '$Board' requires arch '$($BoardInfo.arch)', but arch '$Arch' was provided." -ForegroundColor Red
        exit 1
    }

    $Arch = $BoardInfo.arch
    if ($Machine -eq "") { $Machine = $BoardInfo.machine }
    if ($HardwareProfile -eq "") { $HardwareProfile = $BoardInfo.hardware_profile }
    if ($BootTier -eq "") { $BootTier = $BoardInfo.tier }
}

if ($Arch -eq "") { $Arch = "x86_64" }
if ($Machine -eq "") { $Machine = "virt" }
if ($HardwareProfile -eq "") { $HardwareProfile = "generic" }
if ($BootTier -eq "") { $BootTier = "LINUX_LIKE" }

$ProfileClean = $Profile -replace ',', '-'
$PersonalityClean = $Personality -replace ',', '-'

$BuildDir = "$Root\build\${Arch}_${ProfileClean}_${HardwareProfile}_${PersonalityClean}"
if ($Board -ne "") {
    $BuildDir = "${BuildDir}_${Board}"
}

if ($Payload -and $Arch -eq "riscv64") {
    $BuildDir = "$BuildDir-gcc"
}

$OutELF = "$BuildDir\kernel\kernel.elf"
$OutELF32 = "$BuildDir\kernel\kernel32.elf"

# Resolve Toolchain
$Toolchain = ""
if ($ToolchainOverride -ne "") {
    if (Test-Path "$Root\cmake\toolchains\$Arch-$ToolchainOverride.cmake") {
        $Toolchain = "$Root\cmake\toolchains\$Arch-$ToolchainOverride.cmake"
    }
    elseif (Test-Path "$Root\$ToolchainOverride") {
        $Toolchain = "$Root\$ToolchainOverride"
    }
    elseif ($BoardInfo -ne $null -and $BoardInfo.toolchains -ne $null -and $BoardInfo.toolchains.$ToolchainOverride -ne $null) {
        $Toolchain = "$Root\$($BoardInfo.toolchains.$ToolchainOverride)"
    }
    else {
        Write-Host "  [!] Error: Toolchain override '$ToolchainOverride' not found." -ForegroundColor Red
        exit 1
    }
}
elseif ($BoardInfo -ne $null -and $BoardInfo.default_toolchain -ne $null) {
    $defTc = $BoardInfo.default_toolchain
    $Toolchain = "$Root\$($BoardInfo.toolchains.$defTc)"
}
else {
    if ($Arch -eq "x86_64") { $Toolchain = "$Root\cmake\toolchains\x86_64-elf.cmake" }
    elseif ($Arch -eq "riscv64") {
        if ($Payload) { $Toolchain = "$Root\cmake\toolchains\riscv64-elf-gcc.cmake" }
        else { $Toolchain = "$Root\cmake\toolchains\riscv64-elf.cmake" }
    }
    elseif ($Arch -eq "arm64") { $Toolchain = "$Root\cmake\toolchains\arm64-elf.cmake" }
}

# ── Ensure LLVM tools are on PATH ─────────────────────────────────────────
$llvmBin = "C:\Program Files\LLVM\bin"
if ((Test-Path $llvmBin) -and ($env:PATH -notlike "*LLVM*")) {
    $env:PATH = "$llvmBin;$env:PATH"
}

function inf([string]$m) { Write-Host "  [.] $m" -ForegroundColor Cyan }
function ok([string]$m) { Write-Host "  [+] $m" -ForegroundColor Green }
function fail([string]$m) { Write-Host "  [!] $m" -ForegroundColor Red; exit 1 }

Write-Host ""
Write-Host "  Bharat-OS Build  (arch: $Arch)" -ForegroundColor DarkYellow
Write-Host "  --------------------------------" -ForegroundColor DarkYellow
Write-Host ""

# ── Verify toolchain exists ────────────────────────────────────────────────
if (-not (Test-Path $Toolchain)) { fail "Toolchain not found: $Toolchain" }

# ── Clean ──────────────────────────────────────────────────────────────────
if ($Clean -and (Test-Path $BuildDir)) {
    inf "Cleaning $BuildDir"
    Remove-Item $BuildDir -Recurse -Force
}

# ── Configure (only if no cache exists) ────────────────────────────────────
if (-not (Test-Path "$BuildDir\CMakeCache.txt")) {
    inf "Configuring (CMake)"
    $cmakeArgs = @(
        "-S", "$Root",
        "-B", $BuildDir,
        "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
        "-DBHARAT_BOOT_GUI=$BootGui",
        "-DBHARAT_BOOT_HW_PROFILE=$HardwareProfile",
        "-DBHARAT_BOOT_TIER=$BootTier",
        "-G", "Ninja",
        "--no-warn-unused-cli"
    )

    if ($Profile -ne "") {
        $Profiles = $Profile.Split(',')
        foreach ($p in $Profiles) {
            $pUpper = $p.ToUpper()
            $cmakeArgs += "-DBHARAT_PROFILE_$pUpper=1"
        }
    }

    if ($Personality -ne "") {
        $Personalities = $Personality.Split(',')
        foreach ($p in $Personalities) {
            $pUpper = $p.ToUpper()
            $cmakeArgs += "-DBHARAT_PERSONALITY_$pUpper=1"
        }
    }

    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { fail "CMake configure failed" }
}

# ── Build ──────────────────────────────────────────────────────────────────
inf "Building kernel.elf"
if ($Payload -and $Arch -eq "riscv64") {
    & cmake --build $BuildDir --target kernel.payload.bin
    if ($LASTEXITCODE -ne 0) { fail "Build failed (payload)" }
    $sizeKB = [math]::Round((Get-Item "$BuildDir\payload.bin").Length / 1KB, 1)
    ok "payload.bin -> $BuildDir\payload.bin  ($sizeKB KB)"

    if ($env:OPENSBI_DIR -and (Test-Path $env:OPENSBI_DIR)) {
        inf "Building OpenSBI fw_payload.elf (requires wsl/make or cross-compiler in PATH)"
        # Use simple make command, assumes Windows has a GNU make compatible environment
        # or bash available if building OpenSBI. For now, executing via bash or make
        try {
            # Try to build using make directly if available
            & make -C "$env:OPENSBI_DIR" PLATFORM=generic CROSS_COMPILE=riscv64-unknown-elf- FW_PAYLOAD_PATH="$BuildDir\payload.bin" O="$BuildDir\opensbi" | Out-Null
            if ($LASTEXITCODE -eq 0 -and (Test-Path "$BuildDir\opensbi\platform\generic\firmware\fw_payload.elf")) {
                Copy-Item "$BuildDir\opensbi\platform\generic\firmware\fw_payload.elf" "$BuildDir\fw_payload.elf"
                $sizeKB_fw = [math]::Round((Get-Item "$BuildDir\fw_payload.elf").Length / 1KB, 1)
                ok "fw_payload.elf -> $BuildDir\fw_payload.elf  ($sizeKB_fw KB)"
            }
            else {
                inf "OpenSBI build failed or fw_payload.elf not generated."
            }
        }
        catch {
            inf "Make not found, skipping OpenSBI build."
        }
    }
}
else {
    & cmake --build $BuildDir --target kernel.elf
    if ($LASTEXITCODE -ne 0) { fail "Build failed" }
    $sizeKB = [math]::Round((Get-Item $OutELF).Length / 1KB, 1)
    ok "kernel.elf -> $OutELF  ($sizeKB KB)"
}

# ── Convert to 32-bit ELF for x86_64 (Requirement for Multiboot) ──────────
$KernelBinary = $OutELF
if ($Arch -eq "x86_64") {
    $OutELF32 = "$BuildDir\kernel32.elf"
    inf "Converting to 32-bit ELF (Multiboot compatibility)"
    & llvm-objcopy -I elf64-x86-64 -O elf32-i386 $OutELF $OutELF32
    if ($LASTEXITCODE -ne 0) { fail "ELF conversion failed" }
    $KernelBinary = $OutELF32
}

if ($Arch -eq "x86_64") { ok "kernel32.elf -> $OutELF32" }

if ($Arch -eq "x86_64") { ok "kernel32.elf -> $OutELF32" }

# ── Flash ──────────────────────────────────────────────────────────────────
if ($Flash) {
    if ($BoardInfo -ne $null -and $BoardInfo.flash_script_dir -ne $null) {
        $FlashScript = "$Root\$($BoardInfo.flash_script_dir)\flash.ps1"
        if (Test-Path $FlashScript) {
            inf "Flashing using script: $FlashScript"
            & $FlashScript -KernelBinary $OutELF
            if ($LASTEXITCODE -ne 0) { fail "Flash failed" }
        }
        else {
            fail "Flash script not found at $FlashScript"
        }
    }
    else {
        fail "Flash requested, but no valid flash script dir configured for board."
    }
}

# ── QEMU ──────────────────────────────────────────────────────────────────
if ($Run) {
    $qemuExe = switch ($Arch) {
        "x86_64" { "C:\Program Files\qemu\qemu-system-x86_64.exe" }
        "riscv64" { "C:\Program Files\qemu\qemu-system-riscv64.exe" }
        "arm64" { "C:\Program Files\qemu\qemu-system-aarch64.exe" }
    }
    if (-not (Test-Path $qemuExe)) { fail "QEMU not found at: $qemuExe" }

    Write-Host ""
    ok "Booting in QEMU (press Ctrl+A then X to quit)..."
    Write-Host ""

    $qemuArgs = @()

    if ($Arch -eq "x86_64") {
        $qemuArgs += @("-kernel", $KernelBinary, "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
    }
    elseif ($Arch -eq "riscv64") {
        if ($Payload) {
            if (Test-Path "$BuildDir\fw_payload.elf") {
                $qemuArgs += @("-machine", $Machine, "-bios", "none", "-kernel", "$BuildDir\fw_payload.elf", "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
            }
            else {
                $qemuArgs += @("-machine", $Machine, "-bios", "$BuildDir\payload.bin", "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
            }
        }
        else {
            $qemuArgs += @("-machine", $Machine, "-kernel", $OutELF, "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
        }
    }
    elseif ($Arch -eq "arm64") {
        $qemuArgs += @("-machine", $Machine, "-cpu", "cortex-a53", "-kernel", $OutELF, "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
    }

    if ($DebugQemu) {
        inf "GDB Server enabled on tcp::1234. Waiting for debugger..."
        $qemuArgs += @("-s", "-S")
    }

    & $qemuExe @qemuArgs
}
