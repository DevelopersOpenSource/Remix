@echo off
cd /d "%~dp0"
if not exist Remix.exe (
    echo Remix.exe ainda nao existe: compilando primeiro...
    call COMPILAR.bat
    exit /b
)
start "" Remix.exe
