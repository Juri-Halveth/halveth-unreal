@echo off
setlocal
if not exist "C:\Program Files\Git\bin\bash.exe" (
  echo Git Bash fehlt. Git for Windows installieren.
  pause
  exit /b 2
)
cd /d "%~dp0"
if "%~1"=="" (
  "C:\Program Files\Git\bin\bash.exe" --noprofile --norc ./VOID.sh play 0
) else (
  "C:\Program Files\Git\bin\bash.exe" --noprofile --norc ./VOID.sh %*
)
exit /b %errorlevel%
