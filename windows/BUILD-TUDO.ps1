# BUILD-TUDO.ps1 - Remix Player: compila tudo a partir do Windows.
#
#   1) Remix.exe (Windows x64) com MinGW-w64/GCC (MSYS2) -> windows\Remix.exe, dist\windows\Remix.exe,
#                                                           dist\remix-<ver>-windows-x64-portable.zip
#   2) remix (Linux x86_64, glibc >= 2.27) com zig        -> linux\remix, dist\remix-<ver>-linux-x86_64-portable.zip
#   3) .deb / .rpm / AppImage (so se houver WSL com dpkg-deb / rpmbuild; senao avisa)
#
# O Remix.exe e compilado com o MinGW-w64 (o mesmo do COMPILAR.bat e do MSYS2),
# que e o compilador padrao de programas open source no Windows. O zig so entra
# para gerar o binario LINUX (antivirus do Windows nao analisa binario Linux).
# Nada e baixado sem perguntar antes.
#
# Como rodar (PowerShell, dentro da pasta windows\):
#   Set-ExecutionPolicy -Scope Process Bypass
#   .\BUILD-TUDO.ps1              # tudo
#   .\BUILD-TUDO.ps1 -SoWindows   # so o Remix.exe
#   .\BUILD-TUDO.ps1 -SoLinux     # so o binario Linux + zip
param(
  [switch]$SoWindows,
  [switch]$SoLinux,
  [string]$Versao = '1.5.1'
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)   # raiz do projeto (pasta de cima)
Set-Location $Root
$TP = Join-Path $Root 'third_party'
foreach ($d in @($TP, 'build', 'build\raylib-zig\wl', 'build\win', 'dist', 'dist\windows')) { New-Item -ItemType Directory -Force -Path (Join-Path $Root $d) | Out-Null }

function Passo($msg) { Write-Host ""; Write-Host "==> $msg" -ForegroundColor Cyan }
function Perguntar($msg) { $r = Read-Host "$msg (S/N)"; return ($r -match '^[sSyY]') }

# ---------------------------------------------------------- MinGW-w64 -----
function Achar-MinGW {
  $cands = @()
  $c = Get-Command g++.exe -ErrorAction SilentlyContinue
  if ($c) { $cands += $c.Source }
  $cands += 'C:\msys64\ucrt64\bin\g++.exe', 'C:\msys64\mingw64\bin\g++.exe', 'C:\mingw64\bin\g++.exe',
            (Join-Path $env:USERPROFILE 'scoop\apps\mingw\current\bin\g++.exe'),
            'C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin\g++.exe'
  foreach ($g in $cands) {
    if ($g -and (Test-Path $g)) {
      $bin = Split-Path $g; $old = $env:PATH; $env:PATH = "$bin;$old"
      $m = (& $g -dumpmachine 2>$null)
      $env:PATH = $old
      if ($m -like 'x86_64*') { return $g }
    }
  }
  return $null
}

# ------------------------------------------------------------ Windows -----
if (-not $SoLinux) {
  $Gpp = Achar-MinGW
  if (-not $Gpp) { throw "MinGW-w64 (g++) nao encontrado. Rode windows\COMPILAR.bat (ele ajuda a instalar o MSYS2) ou veja windows\LEIA-ME.txt." }
  $Bin = Split-Path $Gpp
  $env:PATH = "$Bin;$env:PATH"
  Passo "compilando Remix.exe (Windows x64) com MinGW-w64 $(& $Gpp -dumpversion)"
  # recursos: icone + versao (app_res.o) e manifesto (default-manifest.o; -Bwindows/ faz o GCC usar o nosso)
  Push-Location (Join-Path $Root 'windows')
  $wr = Join-Path $Bin 'windres.exe'
  & $wr -O coff app.rc -o app_res_novo.o; $ok1 = ($LASTEXITCODE -eq 0)
  & $wr -O coff manifest.rc -o default-manifest_novo.o; $ok2 = ($LASTEXITCODE -eq 0)
  if ($ok1 -and $ok2) { Move-Item -Force app_res_novo.o app_res.o; Move-Item -Force default-manifest_novo.o default-manifest.o }
  else { Remove-Item -ErrorAction SilentlyContinue app_res_novo.o, default-manifest_novo.o; Write-Warning "windres falhou: usando os recursos ja compilados da pasta" }
  Pop-Location
  & (Join-Path $Bin 'gcc.exe') -O2 -w -c "-I$Root\comum" 'comum\audio_backend.c' -o 'build\win\audio_backend.o'
  if ($LASTEXITCODE -ne 0) { throw "falhou audio_backend.c (Windows)" }
  & $Gpp -std=gnu++20 -O2 -w -municode -mwindows -static -s ("-B" + ($Root -replace '\\','/') + '/windows/') "-I$Root\comum" "-I$Root\windows" `
      'windows\main.cpp' 'build\win\audio_backend.o' 'windows\app_res.o' -o 'build\win\Remix.exe' `
      -lgdiplus -lshell32 -lcomdlg32 -lole32 -luuid -lwinmm -lwinhttp -ldwmapi -lws2_32 -liphlpapi
  if ($LASTEXITCODE -ne 0) { throw "falhou a compilacao do Remix.exe (se o Remix estiver aberto, feche-o, inclusive pela bandeja, e rode de novo)" }
  Copy-Item 'build\win\Remix.exe' 'dist\windows\Remix.exe' -Force
  Copy-Item 'build\win\Remix.exe' 'windows\Remix.exe' -Force   # exe pronto na pasta windows\
  # zip portatil Windows: exe + assets + config + Musica
  $pw = Join-Path $Root "build\portable-win\remix-$Versao-windows-x64"
  if (Test-Path (Split-Path $pw)) { Remove-Item -Recurse -Force (Split-Path $pw) }
  New-Item -ItemType Directory -Force -Path "$pw\Musica", "$pw\assets\covers" | Out-Null
  Copy-Item 'build\win\Remix.exe' "$pw\Remix.exe"
  Copy-Item -Recurse 'assets\branding', 'assets\themes' "$pw\assets\"
  Copy-Item 'Musica\LEIA-ME.txt' "$pw\Musica\" -ErrorAction SilentlyContinue
  Copy-Item 'README.md' "$pw\" -ErrorAction SilentlyContinue
  Copy-Item 'README.pt-BR.md' "$pw\" -ErrorAction SilentlyContinue
  Copy-Item 'windows\INSTALAR-DEPENDENCIAS.bat' "$pw\"
  Copy-Item 'windows\LEIA-ME-PORTATIL.txt' "$pw\LEIA-ME.txt"
  New-Item -ItemType Directory -Force -Path "$pw\assets\tools" | Out-Null
  "Opcional: coloque aqui yt-dlp.exe, ffmpeg.exe e deno.exe se preferir instalar a mao.`r`nO Remix procura nesta pasta.`r`n" | Set-Content -Path "$pw\assets\tools\LEIA-ME.txt" -Encoding ASCII
  "[General]`r`nMusicFolder=Musica`r`n" | Set-Content -Path "$pw\config.ini" -Encoding ASCII
  $zipW = Join-Path $Root "dist\remix-$Versao-windows-x64-portable.zip"
  if (Test-Path $zipW) { Remove-Item $zipW }
  Compress-Archive -Path $pw -DestinationPath $zipW
  Write-Host "ok -> windows\Remix.exe, dist\windows\Remix.exe e $zipW" -ForegroundColor Green
}

# -------------------------------------------------------------- Linux -----
if (-not $SoWindows) {
  $ZigVer = '0.13.0'
  $Zig = Join-Path $TP 'zig-win\zig.exe'
  $RL = Join-Path $TP 'raylib-5.5\src'
  $Sysroot = Join-Path $TP 'sysroot\usr\include'
  $podeLinux = $true
  if (-not (Test-Path (Join-Path $Sysroot 'X11\Xlib.h'))) {
    Write-Warning "third_party\sysroot (headers X11/Wayland) nao encontrado: nao da pra compilar o binario Linux daqui. Use a pasta third_party completa do projeto ou rode linux/fetch-deps.sh num Linux."
    $podeLinux = $false
  }
  if ($podeLinux -and -not (Test-Path $Zig)) {
    if (Perguntar "O binario Linux e compilado com o zig $ZigVer (~80 MB, de ziglang.org). Baixar agora para third_party\zig-win?") {
      $zipZ = Join-Path $TP 'zig-win.zip'
      Invoke-WebRequest -Uri "https://ziglang.org/download/$ZigVer/zig-windows-x86_64-$ZigVer.zip" -OutFile $zipZ
      Expand-Archive -Path $zipZ -DestinationPath $TP -Force
      Rename-Item -Path (Join-Path $TP "zig-windows-x86_64-$ZigVer") -NewName 'zig-win'
      Remove-Item $zipZ
    } else { Write-Warning "sem zig: pulando o binario Linux"; $podeLinux = $false }
  }
  if ($podeLinux -and -not (Test-Path $RL)) {
    if (Perguntar "Falta o codigo do raylib 5.5 (~20 MB, github.com/raysan5/raylib). Baixar agora?") {
      $tgz = Join-Path $TP 'raylib-5.5.tar.gz'
      Invoke-WebRequest -Uri 'https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz' -OutFile $tgz
      tar -xzf $tgz -C $TP
      Remove-Item $tgz
    } else { Write-Warning "sem raylib: pulando o binario Linux"; $podeLinux = $false }
  }
  if ($podeLinux) {
    $T = 'x86_64-linux-gnu.2.27'
    Passo "compilando raylib 5.5 para Linux (zig, alvo $T)"
    Copy-Item 'linux\wl-protocols\*.h' 'build\raylib-zig\wl\' -Force
    $defs = @('-DPLATFORM_DESKTOP', '-DPLATFORM_DESKTOP_GLFW', '-DGRAPHICS_API_OPENGL_33', '-D_GLFW_X11', '-D_GLFW_WAYLAND',
              '-DSUPPORT_FILEFORMAT_JPG=1', '-DSUPPORT_FILEFORMAT_BMP=1', '-D_GNU_SOURCE')
    $inc = @("-I$RL", "-I$RL\external\glfw\include", '-Ibuild\raylib-zig\wl', "-isystem", $Sysroot)
    $objs = @()
    foreach ($f in @('rcore', 'rshapes', 'rtextures', 'rtext', 'utils', 'rglfw')) {
      $o = "build\raylib-zig\$f.o"
      & $Zig cc -target $T -fno-sanitize=undefined -O2 -DNDEBUG -std=gnu99 -fPIC -w @defs @inc -c "$RL\$f.c" -o $o
      if ($LASTEXITCODE -ne 0) { throw "falhou raylib: $f.c" }
      $objs += $o
    }
    & $Zig ar rcs 'build\raylib-zig\libraylib.a' @objs
    Passo "compilando o Remix para Linux (glibc >= 2.27)"
    & $Zig cc -target $T -fno-sanitize=undefined -O2 -w -c "-I$Root\comum" 'comum\audio_backend.c' -o 'build\audio_backend.o'
    if ($LASTEXITCODE -ne 0) { throw "falhou audio_backend.c" }
    & $Zig c++ -target $T -fno-sanitize=undefined -std=gnu++20 -O2 -w "-I$Root\comum" "-I$Root\linux" "-I$RL" "-I$RL\external" `
        -o 'build\remix' 'linux\main_linux.cpp' 'build\audio_backend.o' 'build\raylib-zig\libraylib.a' -lm -lpthread -ldl -Wl,--strip-all
    if ($LASTEXITCODE -ne 0) { throw "falhou a compilacao do remix (Linux)" }
    Copy-Item 'build\remix' 'linux\remix' -Force
    # zip portatil Linux (o zip do Windows nao guarda permissao de execucao:
    # no Linux, rode "bash RODAR.sh" ou "chmod +x remix RODAR.sh" uma vez)
    $pl = Join-Path $Root "build\portable-linux\remix-$Versao-linux-x86_64"
    if (Test-Path (Split-Path $pl)) { Remove-Item -Recurse -Force (Split-Path $pl) }
    New-Item -ItemType Directory -Force -Path "$pl\Musica", "$pl\assets\covers" | Out-Null
    Copy-Item 'build\remix' "$pl\remix"
    Copy-Item -Recurse 'assets\branding', 'assets\fonts', 'assets\themes' "$pl\assets\"
    Remove-Item "$pl\assets\branding\app.ico" -ErrorAction SilentlyContinue
    Copy-Item 'linux\RODAR.sh' "$pl\RODAR.sh"
    Copy-Item 'Musica\LEIA-ME.txt' "$pl\Musica\" -ErrorAction SilentlyContinue
    Copy-Item 'linux\README-LINUX.md' "$pl\README-LINUX.md"
    Copy-Item 'linux\packaging\copyright' "$pl\LICENCAS.txt"
    "[General]`nMusicFolder=Musica`n" | Set-Content -Path "$pl\config.ini" -Encoding ASCII -NoNewline
    $zipL = Join-Path $Root "dist\remix-$Versao-linux-x86_64-portable.zip"
    if (Test-Path $zipL) { Remove-Item $zipL }
    Compress-Archive -Path $pl -DestinationPath $zipL
    Write-Host "ok -> linux\remix e $zipL" -ForegroundColor Green

    # .deb / .rpm / AppImage precisam de dpkg-deb / rpmbuild: tenta pelo WSL, se existir
    $wsl = Get-Command wsl.exe -ErrorAction SilentlyContinue
    if ($wsl) {
      Passo "tentando gerar .deb/.rpm/AppImage pelo WSL"
      $wslRoot = (& wsl.exe wslpath -a ($Root -replace '\\', '/')) 2>$null
      if ($wslRoot) {
        & wsl.exe bash -lc "cd '$wslRoot' && chmod +x linux/*.sh linux/packaging/*.sh linux/remix && REMIX_VERSION=$Versao linux/packaging/build-packages.sh"
        if ($LASTEXITCODE -ne 0) { Write-Warning "linux/packaging/build-packages.sh falhou no WSL (instale 'dpkg' e 'rpm' la dentro: sudo apt install dpkg rpm)" }
      }
    } else {
      Write-Host "(.deb/.rpm/AppImage: rode linux/packaging/build-packages.sh num Linux ou no WSL; o binario ja esta em linux\remix)" -ForegroundColor Yellow
    }
  }
}

Passo "pronto"
Get-ChildItem dist -Recurse -File | ForEach-Object { "{0,10:N0} KB  {1}" -f ($_.Length / 1KB), $_.FullName.Substring($Root.Length + 1) }
