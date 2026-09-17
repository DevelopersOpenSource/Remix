@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Remix Player - montar pasta Remix (libs + compilar + copiar tudo)
set "HERE=%~dp0"
set "SELF=%~f0"
cd /d "%HERE%"
set "AUTO="
set "QUERSTEMS="
:args
if "%~1"=="" goto args_fim
if /i "%~1"=="/sim" set "AUTO=1"
if /i "%~1"=="/stems" set "QUERSTEMS=1"
shift
goto args
:args_fim

rem ---- onde estamos: arvore de codigo (windows\) ou dentro da pasta Remix montada? ----
for %%I in ("%HERE%..") do set "ROOT=%%~fI"
rem Modo PORTATIL: o bat foi copiado para dentro da propria pasta Remix, que tem
rem assets\tools ao lado. Modo FONTE (a pasta windows\ do codigo): nao tem assets.
rem Obs: a pasta movel normalmente mora DENTRO da arvore (Remix\ dentro de D:\Remix),
rem entao o ".." dela aponta pro codigo - por isso o teste de assets\tools vem primeiro.
set "SRC=0"
if not exist "%HERE%assets\tools" if exist "%ROOT%\comum\config.h" if exist "%ROOT%\assets\branding" set "SRC=1"
if "%SRC%"=="1" (set "OUT=%ROOT%\Remix") else (set "OUT=%HERE%.")
for %%I in ("%OUT%") do set "OUT=%%~fI"
set "TOOLS=%OUT%\assets\tools"
for %%I in ("%TOOLS%") do set "TOOLS=%%~fI"

rem ---- qual Windows: 10 (build 10240 a 21999) ou 11 (22000 em diante) ----
set "BUILD=0"
for /f "tokens=3" %%B in ('reg query "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" /v CurrentBuild 2^>nul ^| findstr /i "CurrentBuild"') do set "BUILD=%%B"
set "WINNOME=Windows"
if %BUILD% GEQ 10240 set "WINNOME=Windows 10"
if %BUILD% GEQ 22000 set "WINNOME=Windows 11"

rem ---- ferramentas do proprio Windows: curl e tar (10 1803 em diante) ----
set "CURL="
if exist "%SystemRoot%\System32\curl.exe" set "CURL=%SystemRoot%\System32\curl.exe"
if not defined CURL for /f "delims=" %%C in ('where curl.exe 2^>nul') do if not defined CURL set "CURL=%%C"
set "TAR="
if exist "%SystemRoot%\System32\tar.exe" set "TAR=%SystemRoot%\System32\tar.exe"
if not defined TAR for /f "delims=" %%C in ('where tar.exe 2^>nul') do if not defined TAR set "TAR=%%C"

echo ==============================================================
echo    Remix Player - montar a pasta Remix
echo ==============================================================
echo.
echo  Sistema: %WINNOME% (build %BUILD%)
if "%SRC%"=="1" (
    echo  Modo: CODIGO FONTE - atualiza as libs, compila o Remix.exe e
    echo        monta tudo na pasta movel:
    echo        %OUT%
) else (
    echo  Modo: PASTA PORTATIL - so atualiza as libs musicais:
    echo        %TOOLS%
)
echo.
echo  O Remix toca as musicas do seu PC sem instalar nada. Para BUSCAR,
echo  TOCAR ONLINE (streaming) e BAIXAR musicas ele usa tres programas
echo  gratuitos e de codigo aberto, que ficam SO dentro da pasta Remix:
echo.
echo    yt-dlp  - encontra o audio no YouTube, YouTube Music e SoundCloud
echo    FFmpeg  - converte o audio (e toca M4A, AAC, Opus e WMA)
echo    Deno    - sem ele o YouTube e o YouTube Music nao tocam
echo.
echo  Pasta das ferramentas: %TOOLS%
echo.
if %BUILD% GTR 0 if %BUILD% LSS 10240 goto antigo
if not defined CURL goto semcurl
if not defined TAR goto semcurl
if defined AUTO goto baixar
choice /c SN /n /m " Baixar (ou atualizar) as libs agora? [S/N] "
if errorlevel 2 goto cancelado
goto baixar

:baixar
echo ==============================================================
echo  Baixando dos sites oficiais para a pasta
echo  %TOOLS%
echo ==============================================================
if not exist "%TOOLS%" mkdir "%TOOLS%"
set "TMPD=%TEMP%\remix-deps-%RANDOM%%RANDOM%"
mkdir "%TMPD%" >nul 2>nul
call :conferir_tudo quieto
if defined FALTA_yt-dlp call :baixar_ytdlp
if defined FALTA_deno call :baixar_deno
if defined FALTA_ffmpeg call :baixar_ffmpeg
if defined FALTA_cloudflared call :baixar_cloudflared
call :stems_talvez
rmdir /s /q "%TMPD%" >nul 2>nul
echo.
call :conferir_tudo
if defined FALTA (
    echo  Ainda falta:!FALTA!
    echo  Rode este arquivo de novo. Se continuar faltando, baixe cada programa na
    echo  pagina oficial e coloque o .exe em %TOOLS%
    goto fim
)
if not "%SRC%"=="1" goto pronto
goto compilar

:compilar
if not exist "%OUT%" mkdir "%OUT%"
echo ==============================================================
echo  Compilando o Remix.exe (MinGW-w64 / GCC)...
echo  destino: %OUT%\Remix.exe  (dentro da propria pasta)
echo ==============================================================
call "%HERE%COMPILAR.bat" /so /saida "%OUT%"
if errorlevel 1 goto compilar_erro
goto montar

:compilar_erro
echo.
echo  [ERRO] A compilacao falhou.
echo  Dica: se o Remix estiver aberto, feche-o - inclusive pelo icone da bandeja,
echo  perto do relogio - e rode este arquivo de novo. Veja as mensagens acima.
goto fim

:montar
echo.
echo ==============================================================
echo  Montando a pasta movel:
echo  %OUT%
echo ==============================================================
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%\assets\branding" mkdir "%OUT%\assets\branding"
if not exist "%OUT%\assets\fonts" mkdir "%OUT%\assets\fonts"
if not exist "%OUT%\assets\themes" mkdir "%OUT%\assets\themes"
xcopy /e /y /q "%ROOT%\assets\branding" "%OUT%\assets\branding\" >nul
xcopy /e /y /q "%ROOT%\assets\fonts" "%OUT%\assets\fonts\" >nul
xcopy /e /y /q "%ROOT%\assets\themes" "%OUT%\assets\themes\" >nul
if exist "%ROOT%\config.ini" (
    copy /y "%ROOT%\config.ini" "%OUT%\config.ini" >nul
) else (
    >"%OUT%\config.ini" echo [General]
    >>"%OUT%\config.ini" echo MusicFolder=Musica
)
if not exist "%OUT%\Musica" mkdir "%OUT%\Musica"
if exist "%ROOT%\Musica\LEIA-ME.txt" copy /y "%ROOT%\Musica\LEIA-ME.txt" "%OUT%\Musica\LEIA-ME.txt" >nul
copy /y "%HERE%LEIA-ME-PORTATIL.txt" "%OUT%\LEIA-ME.txt" >nul
copy /y "%SELF%" "%OUT%\INSTALAR-DEPENDENCIAS.bat" >nul
echo.
echo  Pronto. Pasta Remix completa em:
echo  %OUT%
echo  Mova essa pasta para onde quiser: tudo fica junto nela.

:pronto
echo.
echo  Libs ok em %TOOLS%
echo  Abra o %OUT%\Remix.exe: ele encontra as ferramentas sozinho, em assets\tools.
goto fim

:semcurl
echo  Este Windows nao tem o curl e o tar (eles vieram no Windows 10 versao 1803).
echo  Use a Microsoft Store (Instalador de Aplicativo) ou instale a mao.
echo.
choice /c 123 /n /m " 1-Abrir a Microsoft Store  2-Abrir as paginas oficiais + a pasta de libs  3-Sair: "
if errorlevel 3 goto cancelado
if errorlevel 2 goto manual
goto loja

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
echo  Coloque yt-dlp.exe, ffmpeg.exe e deno.exe na pasta que abriu.
goto fim

:cancelado
echo  Nada foi feito.
:fim
echo.
if not defined AUTO pause
exit /b 0

rem ======================= subrotinas =======================

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

:baixar_cloudflared
echo.
echo  cloudflared (tunel do Host: ouvir as musicas no celular pela internet)
"%CURL%" -fL --retry 3 -o "%TOOLS%\cloudflared.exe.part" "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"
if errorlevel 1 goto baixar_erro
move /y "%TOOLS%\cloudflared.exe.part" "%TOOLS%\cloudflared.exe" >nul
exit /b 0

:baixar_erro
echo    [erro] nao consegui baixar ou extrair. Confira a internet e rode de novo.
exit /b 1

rem ---- separador de stems (opcional): Demucs num Python isolado dentro da pasta Remix ----
:stems_talvez
set "STEMS=%TOOLS%\stems"
if exist "%STEMS%\venv\Lib\site-packages\demucs" (
    echo    [ok]     separador de stems ^(Demucs^)
    exit /b 0
)
if defined QUERSTEMS goto stems_instalar
if defined AUTO exit /b 0
echo.
echo  Opcional: separador de stems ^(so vocal, so musica, bateria, baixo...^).
echo  Baixa cerca de 1 GB e, no processador, leva mais ou menos metade da
echo  duracao de cada musica na primeira vez ^(depois fica guardado^).
choice /c SN /n /m " Instalar o separador de stems tambem? [S/N] "
if errorlevel 2 exit /b 0
:stems_instalar
echo.
echo  Separador de stems: uv + Python 3.12 + PyTorch ^(CPU^) + Demucs em
echo  %STEMS%
if not exist "%STEMS%" mkdir "%STEMS%"
"%CURL%" -fL --retry 3 -o "%TMPD%\uv.zip" "https://github.com/astral-sh/uv/releases/latest/download/uv-x86_64-pc-windows-msvc.zip"
if errorlevel 1 goto stems_erro
"%TAR%" -xf "%TMPD%\uv.zip" -C "%STEMS%"
if errorlevel 1 goto stems_erro
set "UV_PYTHON_INSTALL_DIR=%STEMS%\python"
set "UV_CACHE_DIR=%TMPD%\uvcache"
set "UV_LINK_MODE=copy"
"%STEMS%\uv.exe" venv --allow-existing --python 3.12 "%STEMS%\venv"
if errorlevel 1 goto stems_erro
"%STEMS%\uv.exe" pip install --python "%STEMS%\venv\Scripts\python.exe" torch==2.5.1 torchaudio==2.5.1 --index-url https://download.pytorch.org/whl/cpu
if errorlevel 1 goto stems_erro
"%STEMS%\uv.exe" pip install --python "%STEMS%\venv\Scripts\python.exe" demucs==4.0.1 soundfile
if errorlevel 1 goto stems_erro
echo    baixando o modelo htdemucs ^(~80 MB^)
set "TORCH_HOME=%STEMS%\torch"
"%STEMS%\venv\Scripts\python.exe" -c "from demucs.pretrained import get_model; get_model('htdemucs')"
if errorlevel 1 goto stems_erro
echo    [ok]     separador de stems instalado
exit /b 0
:stems_erro
echo    [aviso] o separador de stems nao foi instalado ^(o resto do Remix funciona igual^).
exit /b 0

:conferir_tudo
set "QUIETO=%~1"
if not defined QUIETO echo ==============================================================
if not defined QUIETO echo  Conferindo (so na pasta de libs do Remix)
if not defined QUIETO echo ==============================================================
set "FALTA="
call :conferir yt-dlp
call :conferir ffmpeg
call :conferir deno
call :conferir cloudflared
if not defined QUIETO echo.
exit /b 0

:conferir
set "ACHOU="
set "FALTA_%~1="
if not "%~1"=="" if exist "%TOOLS%\%~1.exe" set "ACHOU=%TOOLS%\%~1.exe"
if defined ACHOU (
    if not defined QUIETO echo    [ok]     %~1   !ACHOU!
) else (
    if not defined QUIETO echo    [falta]  %~1
    set "FALTA=!FALTA! %~1"
    set "FALTA_%~1=1"
)
exit /b 0