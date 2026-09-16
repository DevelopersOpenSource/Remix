@echo off
setlocal enabledelayedexpansion
title Remix Player - compilar (Windows, MinGW-w64)
set "HERE=%~dp0"
cd /d "%HERE%"
set "ABRIR=1"
set "SAIDA_PROX="
set "SAIDA="
:args
if "%~1"=="" goto args_fim
if /i "%~1"=="/so" set "ABRIR="
if /i "%~1"=="/saida" set "SAIDA_PROX=1"
if defined SAIDA_PROX if /i not "%~1"=="/saida" (
    set "SAIDA=%~1"
    set "SAIDA_PROX="
)
shift
goto args
:args_fim
set "COMUM=%HERE%..\comum"
REM pasta deste script com barras normais (C:/.../windows/): usada no -B abaixo
set "BDIR=%HERE%"
set "BDIR=%BDIR:\=/%"

echo ============================================
echo   Remix Player - compilar a versao Windows
echo   compilador: MinGW-w64 / GCC
echo ============================================
echo.

:procurar
set "GPP="
for %%P in (g++.exe) do set "GPP=%%~$PATH:P"
if defined GPP call :checar_arch
if not defined GPP (
    for %%D in ("C:\msys64\ucrt64\bin" "C:\msys64\mingw64\bin" "C:\mingw64\bin" "%USERPROFILE%\scoop\apps\mingw\current\bin" "C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin") do (
        if not defined GPP if exist "%%~D\g++.exe" (
            set "GPP=%%~D\g++.exe"
            call :checar_arch
        )
    )
)
if not defined GPP goto :semcompilador

for %%D in ("%GPP%") do set "BINDIR=%%~dpD"
set "PATH=%BINDIR%;%PATH%"
echo Compilador: %GPP%
echo.

echo [1/3] Recursos: icone, informacoes de versao e manifesto...
"%BINDIR%windres.exe" -O coff app.rc -o app_res_novo.o
if errorlevel 1 goto :sem_windres
"%BINDIR%windres.exe" -O coff manifest.rc -o default-manifest_novo.o
if errorlevel 1 goto :sem_windres
move /y app_res_novo.o app_res.o >nul
move /y default-manifest_novo.o default-manifest.o >nul
goto :audio
:sem_windres
echo [aviso] windres falhou: usando o app_res.o e o default-manifest.o que ja vem na pasta.
del /q app_res_novo.o default-manifest_novo.o 2>nul

:audio
echo [2/3] Audio - miniaudio...
"%BINDIR%gcc.exe" -O2 -w -c -I"%COMUM%" "%COMUM%\audio_backend.c" -o audio_backend.o
if errorlevel 1 goto :falhou

echo [3/3] Remix.exe - demora uns 30-60 segundos...
REM -B: o GCC linka sozinho um "default-manifest.o"; com -B<esta pasta> ele usa o desta pasta
REM     (o nosso manifesto) no lugar do padrao do compilador - sem manifesto duplicado.
REM     Precisa ser caminho absoluto: relativo (-B./) o GCC do Windows nao aceita.
set "EXEOUT=%HERE%Remix.exe"
if defined SAIDA (
    set "EXEOUT=%SAIDA%\Remix.exe"
    if not exist "%SAIDA%" mkdir "%SAIDA%"
)
"%GPP%" -std=gnu++20 -O2 -w -municode -mwindows -static -s -B"%BDIR%" -I"%COMUM%" -I. main.cpp audio_backend.o app_res.o -o "%EXEOUT%" -lgdiplus -lshell32 -lcomdlg32 -lole32 -luuid -lwinmm -lwinhttp -lws2_32 -liphlpapi -ldwmapi
if errorlevel 1 goto :falhou
del /q audio_backend.o 2>nul

echo.
echo Pronto: %EXEOUT%
if defined ABRIR (
    echo Abrindo o player...
    start "" "%EXEOUT%"
)
exit /b 0

:falhou
echo.
echo [ERRO] A compilacao falhou. Veja as mensagens acima.
echo Dica: se o Remix estiver aberto, feche-o - inclusive pelo icone da bandeja, perto do relogio -
echo e rode este COMPILAR.bat de novo.
pause
exit /b 1

:checar_arch
set "MACH="
for /f "usebackq delims=" %%M in (`"%GPP%" -dumpmachine 2^>nul`) do set "MACH=%%M"
if /i not "!MACH:~0,6!"=="x86_64" (
    echo [aviso] "%GPP%" nao gera programa 64-bit ^(!MACH!^). Ignorando esse.
    set "GPP="
)
exit /b 0

:semcompilador
echo Nao encontrei o compilador MinGW-w64 ^(g++^) neste PC.
echo.
echo Jeito recomendado, gratis e oficial: MSYS2
echo   1. Instale o MSYS2: https://www.msys2.org/   ^(ou no terminal: winget install -e --id MSYS2.MSYS2^)
echo   2. Abra "MSYS2 UCRT64" no menu Iniciar e rode:
echo        pacman -S --needed mingw-w64-ucrt-x86_64-gcc
echo   3. Clique duas vezes neste COMPILAR.bat de novo.
echo.
if exist "C:\msys64\usr\bin\bash.exe" goto :so_gcc
where winget >nul 2>nul
if errorlevel 1 goto :sair_erro
choice /c SN /m "Quer que eu faca os passos 1 e 2 agora pelo winget, o instalador oficial da Microsoft"
if errorlevel 2 goto :sair_erro
winget install -e --id MSYS2.MSYS2
if not exist "C:\msys64\usr\bin\bash.exe" goto :winget_falhou
:instalar_gcc
echo.
echo Instalando o g++ no MSYS2 - quando o pacman perguntar, confirme com Enter...
"C:\msys64\usr\bin\bash.exe" -lc "pacman -S --needed mingw-w64-ucrt-x86_64-gcc"
echo.
goto :procurar
:so_gcc
echo O MSYS2 ja esta instalado em C:\msys64, falta so o g++.
choice /c SN /m "Instalar o g++ agora pelo pacman do MSYS2"
if errorlevel 2 goto :sair_erro
goto :instalar_gcc
:winget_falhou
echo [ERRO] O MSYS2 nao apareceu em C:\msys64. Instale pelo site https://www.msys2.org/ e rode de novo.
:sair_erro
echo.
pause
exit /b 1
