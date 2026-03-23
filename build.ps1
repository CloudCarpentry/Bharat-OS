param (
    [Parameter(Position=0, ValueFromRemainingArguments=$true)]
    [string[]]$ArgsList
)
python tools\build.py @ArgsList
