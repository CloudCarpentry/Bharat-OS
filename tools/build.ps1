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
    [ValidateSet("x86_64", "riscv64", "arm64")]
    [string]$Arch = "x86_64",
    [switch]$Clean = $false,
    [switch]$Run = $false,
    [switch]$DebugQemu = $false,
    [switch]$Payload = $false,
    [string]$Machine = "virt",
    [ValidateSet("ON", "OFF")][string]$BootGui = "ON",
    [ValidateSet("generic", "desktop", "server", "vm", "laptop")][string]$HardwareProfile = "generic"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$BuildDir = "$Root\build\$Arch"
if ($Payload -and $Arch -eq "riscv64") {
    $BuildDir = "$Root\build\$Arch-gcc"
}

$OutELF = "$BuildDir\kernel.elf"

$Toolchain = "$Root\cmake\toolchains\$Arch-elf.cmake"
if ($Payload -and $Arch -eq "riscv64") {
    $Toolchain = "$Root\cmake\toolchains\riscv64-elf-gcc.cmake"
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
        "-S", "$Root\kernel",
        "-B", $BuildDir,
        "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
        "-DBHARAT_BOOT_GUI=$BootGui",
        "-DBHARAT_BOOT_HW_PROFILE=$HardwareProfile",
        "-G", "Ninja",
        "--no-warn-unused-cli"
    )
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { fail "CMake configure failed" }
}

# ── Build ──────────────────────────────────────────────────────────────────
inf "Building kernel.elf"
if ($Payload -and $Arch -eq "riscv64") {
    & cmake --build $BuildDir --target kernel.payload.bin
    if ($LASTEXITCODE -ne 0) { fail "Build failed (payload)" }
} else {
    & cmake --build $BuildDir --target kernel.elf
    if ($LASTEXITCODE -ne 0) { fail "Build failed" }
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

if ($Payload -and $Arch -eq "riscv64") {
    $sizeKB = [math]::Round((Get-Item "$BuildDir\payload.bin").Length / 1KB, 1)
    ok "payload.bin -> $BuildDir\payload.bin  ($sizeKB KB)"
} else {
    $sizeKB = [math]::Round((Get-Item $OutELF).Length / 1KB, 1)
    ok "kernel.elf -> $OutELF  ($sizeKB KB)"
}

if ($Arch -eq "x86_64") { ok "kernel32.elf -> $OutELF32" }

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
    } elseif ($Arch -eq "riscv64") {
        if ($Payload) {
            $qemuArgs += @("-machine", $Machine, "-bios", "$BuildDir\payload.bin", "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
        } else {
            $qemuArgs += @("-machine", $Machine, "-kernel", $OutELF, "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
        }
    } elseif ($Arch -eq "arm64") {
        $qemuArgs += @("-machine", $Machine, "-cpu", "cortex-a53", "-kernel", $OutELF, "-m", "256M", "-nographic", "-serial", "mon:stdio", "-no-reboot")
    }

    if ($DebugQemu) {
        inf "GDB Server enabled on tcp::1234. Waiting for debugger..."
        $qemuArgs += @("-s", "-S")
    }

    & $qemuExe @qemuArgs
}
