#Requires -Version 5.1
param (
    [Parameter(Position=0, ValueFromRemainingArguments=$true)]
    [string[]]$ArgsList
)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
& "$RootDir\build.ps1" @ArgsList
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
