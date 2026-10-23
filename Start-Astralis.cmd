@echo off
cd /d "%~dp0"
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -File "%~dp0scripts\astral.ps1" -Mode Play
if errorlevel 1 (
  pause
  exit /b 1
)
exit /b 0
