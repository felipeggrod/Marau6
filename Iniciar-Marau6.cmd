@echo off
setlocal
cd /d "%~dp0"
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Start-Marau6.ps1"
if errorlevel 1 (
    echo.
    echo Nao foi possivel iniciar o Marau6. Confira se o Docker Desktop esta aberto.
    pause
    exit /b 1
)
echo.
echo Os servicos Marau6 estao prontos. Esta janela pode ser fechada.
pause
