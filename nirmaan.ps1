#Requires -Version 5.1

$env:PYTHONPATH = $PSScriptRoot
python "$PSScriptRoot\tools\nirmaan\cli.py" @args
