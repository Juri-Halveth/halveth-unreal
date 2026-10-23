@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -File "%~dp0scripts\astral.ps1" -Mode Play
if errorlevel 1 pause
