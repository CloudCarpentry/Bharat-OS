#Requires -Version 5.1
<#
.SYNOPSIS
    Bharat-OS user-facing wrapper script
    Real implementation lives in tools\build.py
#>
param (
    [Parameter(Position=0, ValueFromRemainingArguments=$true)]
    [string[]]$ArgsList
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

& python tools\build.py @ArgsList
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
