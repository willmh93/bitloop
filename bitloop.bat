@echo off
REM bitloop.bat
set SCRIPT_DIR=%~dp0
python "%SCRIPT_DIR%scripts\cli.py" %*
