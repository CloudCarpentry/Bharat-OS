#Requires -Version 5.1
<#
.SYNOPSIS
    Bharat-OS PowerShell Shell Launcher (bosh.ps1)
.DESCRIPTION
    Sets up the Bharat-OS build environment in the current PowerShell session
    and provides helper commands for Windows developers.
.PARAMETER Arch
    Target architecture: x86_64 (default), riscv, arm64
.PARAMETER Command
    Optional: a single command to run non-interactively
.EXAMPLE
    .\tools\bosh.ps1
    .\tools\bosh.ps1 -Arch riscv
    .\tools\bosh.ps1 -Command "cmake --build build/x86_64"
#>
param(
    [ValidateSet("x86_64","riscv","arm64")]
    [string]$Arch = "x86_64",
    [string]$Command = ""
)

Set-StrictMode -Version Latest

# ── Resolve root ──────────────────────────────────────────────────────────────
$BharatRoot = Split-Path -Parent $PSScriptRoot

# ── Colour helpers ────────────────────────────────────────────────────────────
function Write-Saffron([string]$msg) { Write-Host $msg -ForegroundColor DarkYellow -NoNewline }
function Write-Green([string]$msg)   { Write-Host $msg -ForegroundColor Green -NoNewline }
function Write-Ln { Write-Host "" }

function Show-Banner {
    Write-Saffron "╔══════════════════════════════════════════╗"; Write-Ln
    Write-Saffron "║  "; Write-Host "Bharat-OS Build Shell (PowerShell)" -NoNewline; Write-Saffron "   ║"; Write-Ln
    Write-Saffron "║  "; Write-Green "Verified. Sovereign. Open."; Write-Saffron "               ║"; Write-Ln
    Write-Saffron "╚══════════════════════════════════════════╝"; Write-Ln
    Write-Host "  Root : $BharatRoot"
    Write-Host "  Arch : " -NoNewline; Write-Green $Arch; Write-Ln
    Write-Host ""
}

# ── Environment ───────────────────────────────────────────────────────────────
$env:BHARAT_ROOT      = $BharatRoot
$env:BHARAT_ARCH      = $Arch
$env:BHARAT_BUILD_DIR = "$BharatRoot\build\$Arch"

$CrossCompile = switch ($Arch) {
    "x86_64" { "x86_64-elf-" }
    "riscv"  { "riscv64-unknown-elf-" }
    "arm64"  { "aarch64-elf-" }
}
$env:CROSS_COMPILE = $CrossCompile
$env:PATH = "$BharatRoot\tools;$env:PATH"

# ── Import the PowerShell module if available ─────────────────────────────────
$ModulePath = "$BharatRoot\tools\BharatOS.psm1"
if (Test-Path $ModulePath) {
    Import-Module $ModulePath -Force
}

# ── Dispatch ──────────────────────────────────────────────────────────────────
if ($Command -ne "") {
    Invoke-Expression $Command
} else {
    Show-Banner
    Write-Host "Type 'help' for Bharat-OS helper commands. Type 'exit' to leave." -ForegroundColor Gray
}
