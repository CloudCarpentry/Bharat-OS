param(
    [string]$Arch = "x86_64"
)

$gdb = "gdb-multiarch"
if (-not (Get-Command $gdb -ErrorAction SilentlyContinue)) {
    $gdb = "gdb"
}

Write-Host "Starting GDB targeting localhost:1234 for $Arch..." -ForegroundColor Cyan

$args = @(
    "-ex", "target remote localhost:1234",
    "-ex", "layout src",
    "-ex", "break kernel_main",
    "-ex", "continue"
)

& $gdb $args
