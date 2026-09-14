@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Remix Player - instalar dependencias
set "HERE=%~dp0"
cd /d "%HERE%"
set "AUTO="
set "DIRETO="
:args
if "%~1"=="" goto args_fim
if /i "%~1"=="/sim" set "AUTO=1"
if /i "%~1"=="/direto" set "DIRETO=1"
shift
goto args
:args_fim
set "LINKS=%LOCALAPPDATA%\Microsoft\WinGet\Links"
set "TOOLS=%HERE%tools"

rem ---- qual Windows: 10 (build 10240 a 21999) ou 11 (22000 em diante) ----
set "BUILD=0"
for /f "tokens=3" %%B in ('reg query "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" /v CurrentBuild 2^>nul ^| findstr /i "CurrentBuild"') do set "BUILD=%%B"
set "WINNOME=Windows"
if %BUILD% GEQ 10240 set "WINNOME=Windows 10"
if %BUILD% GEQ 22000 set "WINNOME=Windows 11"

rem ---- ferramentas do proprio Windows: winget (11 e 10 atualizado), curl e tar (10 1803 em diante) ----
set "CURL="
if exist "%SystemRoot%\System32\curl.exe" set "CURL=%SystemRoot%\System32\curl.exe"
if not defined CURL for /f "delims=" %%C in ('where curl.exe 2^>nul') do if not defined CURL set "CURL=%%C"
set "TAR="
if exist "%SystemRoot%\System32\tar.exe" set "TAR=%SystemRoot%\System32\tar.exe"
if not defined TAR for /f "delims=" %%C in ('where tar.exe 2^>nul') do if not defined TAR set "TAR=%%C"
set "WINGET="
where winget >nul 2>nul && set "WINGET=1"

echo ==============================================================
echo    Remix Player - instalar dependencias (musica online)
echo ==============================================================
echo.
echo  Sistema: %WINNOME% (build %BUILD%)
echo.
echo  O Remix toca as musicas do seu PC sem instalar nada.
echo  Para BUSCAR, TOCAR ONLINE (streaming) e BAIXAR musicas ele usa
echo  tres programas gratuitos e de codigo aberto:
echo.
echo    yt-dlp  - encontra o audio no YouTube, YouTube Music e SoundCloud
echo    FFmpeg  - converte o audio (e faz o Remix tocar M4A, AAC, Opus e WMA)
echo    Deno    - sem ele o YouTube e o YouTube Music nao tocam
echo.
if %BUILD% GTR 0 if %BUILD% LSS 10240 goto antigo
if defined DIRETO goto direto
if defined WINGET goto comwinget
goto semwinget

:comwinget
echo  Jeito recomendado: winget, o instalador oficial da Microsoft. Ele ja vem no
echo  Windows 11 e nos Windows 10 atualizados. Instala so para o seu usuario, sem
echo  pedir administrador.
echo.
if defined AUTO goto instalar
choice /c SN /n /m " Instalar (ou atualizar) agora pelo winget? [S/N] "
if errorlevel 2 goto cancelado
:instalar
echo.
call :pacote yt-dlp.yt-dlp yt-dlp
call :pacote Gyan.FFmpeg FFmpeg
call :pacote DenoLand.Deno Deno
call :conferir_tudo
if not defined FALTA goto pronto
echo  O winget nao deixou tudo pronto. Isso acontece com o winget desatualizado
echo  ou em PCs de empresa.
echo.
if not defined CURL goto naopronto
if not defined TAR goto naopronto
if defined AUTO goto direto
choice /c SN /n /m " Baixar o que falta direto dos sites oficiais para a pasta tools? [S/N] "
if errorlevel 2 goto naopronto
goto direto

:semwinget
if "%WINNOME%"=="Windows 11" goto semwinget11
echo  O winget nao foi encontrado neste %WINNOME%.
goto semwinget_menu
:semwinget11
echo  O winget deveria vir no Windows 11, mas nao foi encontrado. Normalmente e so
echo  atualizar o "Instalador de Aplicativo" na Microsoft Store (opcao 2).
:semwinget_menu
echo.
set "PODE_DIRETO="
if defined CURL if defined TAR set "PODE_DIRETO=1"
if defined PODE_DIRETO echo    1 - Baixar direto dos sites oficiais para a pasta "tools" ao lado do Remix.exe
if defined PODE_DIRETO echo        usa o curl e o tar que ja vem no Windows; cerca de 200 MB
if not defined PODE_DIRETO echo    1 - [indisponivel: este Windows nao tem curl e tar]
echo    2 - Abrir a Microsoft Store no "Instalador de Aplicativo", que traz o winget;
echo        depois de instalar, rode este arquivo de novo
echo    3 - Instalar a mao: abre as paginas oficiais e a pasta "tools"
echo    4 - Sair
echo.
if defined AUTO if defined PODE_DIRETO goto direto
if defined AUTO goto cancelado
choice /c 1234 /n /m " Escolha 1, 2, 3 ou 4: "
if errorlevel 4 goto cancelado
if errorlevel 3 goto manual
if errorlevel 2 goto loja
if defined PODE_DIRETO goto direto
goto semwinget_menu

:direto
if not defined CURL goto semcurl
if not defined TAR goto semcurl
echo ==============================================================
echo  Baixando dos sites oficiais para a pasta tools:
echo  %TOOLS%
echo ==============================================================
if not exist "%TOOLS%" mkdir "%TOOLS%"
set "TMPD=%TEMP%\remix-deps-%RANDOM%%RANDOM%"
mkdir "%TMPD%" >nul 2>nul
call :conferir_tudo quieto
if defined FALTA_yt-dlp call :baixar_ytdlp
if defined FALTA_deno call :baixar_deno
if defined FALTA_ffmpeg call :baixar_ffmpeg
rmdir /s /q "%TMPD%" >nul 2>nul
echo.
call :conferir_tudo
if defined FALTA goto naopronto
goto pronto

:semcurl
echo  Este Windows nao tem o curl e o tar (eles vieram no Windows 10 versao 1803).
echo  Use o winget ou instale a mao: rode este arquivo de novo e escolha a opcao 2 ou 3.
goto fim

:antigo
echo  Este Windows e anterior ao Windows 10 (build %BUILD%). O Remix e feito para
echo  Windows 10 e 11: aqui so da para instalar a mao.
echo.
if defined AUTO goto cancelado
choice /c SN /n /m " Abrir as paginas oficiais para baixar a mao? [S/N] "
if errorlevel 2 goto cancelado
goto manual

:loja
start "" "ms-windows-store://pdp/?productid=9NBLGGH4NNS1"
goto fim

:manual
if not exist "%TOOLS%" mkdir "%TOOLS%"
start "" "https://github.com/yt-dlp/yt-dlp/releases/latest"
start "" "https://www.gyan.dev/ffmpeg/builds/"
start "" "https://github.com/denoland/deno/releases/latest"
start "" "%TOOLS%"
echo  Coloque yt-dlp.exe, ffmpeg.exe e deno.exe na pasta tools que abriu.
goto fim

:pronto
echo  Tudo pronto. Abra o Remix.exe: ele encontra esses programas sozinho.
goto fim

:naopronto
echo  Ainda falta:!FALTA!
echo  Rode este arquivo de novo. Se continuar faltando, instale a mao: baixe cada
echo  programa na pagina oficial e coloque o .exe na pasta tools ao lado do Remix.exe.
goto fim

:cancelado
echo  Nada foi instalado.
:fim
echo.
if not defined AUTO pause
exit /b 0

rem ======================= subrotinas =======================

:pacote
echo --------------------------------------------------------------
echo  %~2
echo --------------------------------------------------------------
call winget list --id %~1 -e --source winget --accept-source-agreements >nul 2>nul
if errorlevel 1 (
    call winget install --id %~1 -e --source winget --silent --accept-package-agreements --accept-source-agreements
) else (
    echo  Ja instalado. Procurando atualizacao...
    call winget upgrade --id %~1 -e --source winget --silent --accept-package-agreements --accept-source-agreements
)
echo.
exit /b 0

:baixar_ytdlp
echo.
echo  yt-dlp
"%CURL%" -fL --retry 3 -o "%TOOLS%\yt-dlp.exe.part" "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe"
if errorlevel 1 goto baixar_erro
move /y "%TOOLS%\yt-dlp.exe.part" "%TOOLS%\yt-dlp.exe" >nul
exit /b 0

:baixar_deno
echo.
echo  Deno
"%CURL%" -fL --retry 3 -o "%TMPD%\deno.zip" "https://github.com/denoland/deno/releases/latest/download/deno-x86_64-pc-windows-msvc.zip"
if errorlevel 1 goto baixar_erro
"%TAR%" -xf "%TMPD%\deno.zip" -C "%TMPD%"
if not exist "%TMPD%\deno.exe" goto baixar_erro
copy /y "%TMPD%\deno.exe" "%TOOLS%\deno.exe" >nul
exit /b 0

:baixar_ffmpeg
echo.
echo  FFmpeg - arquivo grande, pode levar alguns minutos
"%CURL%" -fL --retry 3 -o "%TMPD%\ffmpeg.zip" "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip"
if errorlevel 1 goto baixar_erro
mkdir "%TMPD%\ff" >nul 2>nul
"%TAR%" -xf "%TMPD%\ffmpeg.zip" -C "%TMPD%\ff"
set "FFX="
for /r "%TMPD%\ff" %%F in (ffmpeg.exe) do if exist "%%F" if not defined FFX set "FFX=%%F"
if not defined FFX goto baixar_erro
copy /y "!FFX!" "%TOOLS%\ffmpeg.exe" >nul
exit /b 0

:baixar_erro
echo    [erro] nao consegui baixar ou extrair. Confira a internet e rode de novo.
exit /b 1

:conferir_tudo
set "QUIETO=%~1"
if not defined QUIETO echo ==============================================================
if not defined QUIETO echo  Conferindo
if not defined QUIETO echo ==============================================================
set "FALTA="
set "UPATH="
set "MPATH="
for /f "skip=2 tokens=2,*" %%A in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "UPATH=%%B"
for /f "skip=2 tokens=2,*" %%A in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "MPATH=%%B"
call set "BUSCA=%TOOLS%;%LINKS%;%HERE%;%UPATH%;%MPATH%;%PATH%"
call :conferir yt-dlp
call :conferir ffmpeg
call :conferir deno
if not defined QUIETO echo.
exit /b 0

:conferir
set "ACHOU="
set "FALTA_%~1="
for %%D in ("!BUSCA:;=" "!") do (
    if not defined ACHOU if not "%%~D"=="" if exist "%%~D\%~1.exe" set "ACHOU=%%~D\%~1.exe"
)
if defined ACHOU (
    if not defined QUIETO echo    [ok]     %~1   !ACHOU!
) else (
    if not defined QUIETO echo    [falta]  %~1
    set "FALTA=!FALTA! %~1"
    set "FALTA_%~1=1"
)
exit /b 0
