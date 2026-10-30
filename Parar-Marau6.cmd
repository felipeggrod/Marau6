@echo off
setlocal
cd /d "%~dp0"
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Stop-Marau6.ps1"
if errorlevel 1 (
    echo Nao foi possivel parar os servicos Marau6.
    pause
    exit /b 1
)
echo Apenas os servicos Marau6 foram parados. Os dados foram preservados.
pause
