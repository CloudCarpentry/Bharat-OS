@echo off
setlocal
set PYTHONPATH=%~dp0
python "%~dp0tools\nirmaan\cli.py" %*
