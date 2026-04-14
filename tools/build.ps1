#Requires -Version 5.1
<#
.SYNOPSIS
    COMPATIBILITY SHIM
    Do not add new build/run logic here.
    Authoritative implementation is tools\build.py.
    Any new flag or run behavior must be implemented in tools\build.py only.
#>
param (
    [Parameter(Position=0, ValueFromRemainingArguments=$true)]
    [string[]]$ArgsList
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$scriptPath = Join-Path $PSScriptRoot "build.py"
& python $scriptPath @ArgsList
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
