# BharatOS.psm1 — Bharat-OS PowerShell Helper Module
# Exposes common build, run and test commands for Windows developers.

function Get-BharatRoot {
    $here = $PSScriptRoot
    # Walk up from tools/ to repo root
    return Split-Path -Parent $here
}

# ── Build ─────────────────────────────────────────────────────────────────────

<#
.SYNOPSIS Build the Bharat-OS kernel for the specified architecture.
.PARAMETER Arch  Target: x86_64 (default), riscv, arm64
.PARAMETER Clean Wipe build directory first
#>
function Invoke-BharatBuild {
    param(
        [ValidateSet("x86_64","riscv","arm64")]
        [string]$Arch = "x86_64",
        [switch]$Clean
    )
    $root  = Get-BharatRoot
    $build = "$root\build\$Arch"

    if ($Clean -and (Test-Path $build)) {
        Write-Host "[bosh] Cleaning $build ..." -ForegroundColor DarkYellow
        Remove-Item $build -Recurse -Force
    }

    Write-Host "[bosh] Configuring for $Arch ..." -ForegroundColor DarkYellow
    cmake -S "$root\kernel" -B $build -DARCH=$Arch -G Ninja

    Write-Host "[bosh] Building kernel.elf ..." -ForegroundColor DarkYellow
    cmake --build $build --target kernel.elf
    Write-Host "[bosh] Build complete: $build\kernel.elf" -ForegroundColor Green
}
Set-Alias -Name bharat-build -Value Invoke-BharatBuild

# ── Run in QEMU ───────────────────────────────────────────────────────────────

<#
.SYNOPSIS Boot the Bharat-OS kernel image in QEMU.
.PARAMETER Arch   Target architecture (must match a completed build)
.PARAMETER Memory RAM in MB (default 256)
#>
function Invoke-BharatRun {
    param(
        [ValidateSet("x86_64","riscv")]
        [string]$Arch = "x86_64",
        [int]$Memory = 256
    )
    $root   = Get-BharatRoot
    $kernel = "$root\build\$Arch\kernel.elf"

    if (-not (Test-Path $kernel)) {
        Write-Error "kernel.elf not found. Run 'bharat-build -Arch $Arch' first."
        return
    }

    switch ($Arch) {
        "x86_64" {
            qemu-system-x86_64 -kernel $kernel -m $Memory -nographic -serial mon:stdio
        }
        "riscv" {
            qemu-system-riscv64 -machine virt -kernel $kernel -m $Memory -nographic -serial mon:stdio
        }
    }
}
Set-Alias -Name bharat-run -Value Invoke-BharatRun

# ── Clean ─────────────────────────────────────────────────────────────────────

function Invoke-BharatClean {
    param([string]$Arch = "")
    $root = Get-BharatRoot
    $target = if ($Arch) { "$root\build\$Arch" } else { "$root\build" }
    if (Test-Path $target) {
        Remove-Item $target -Recurse -Force
        Write-Host "[bosh] Cleaned: $target" -ForegroundColor Green
    } else {
        Write-Host "[bosh] Nothing to clean." -ForegroundColor Gray
    }
}
Set-Alias -Name bharat-clean -Value Invoke-BharatClean

# ── Module exports ────────────────────────────────────────────────────────────
Export-ModuleMember -Function Invoke-BharatBuild, Invoke-BharatRun, Invoke-BharatClean `
                    -Alias    bharat-build, bharat-run, bharat-clean
