#!/usr/bin/env bash
# Compila o Remix.exe (Windows x64) a partir do Linux.
# Compilador padrao: MinGW-w64 (GCC + binutils) - o MESMO do MSYS2 que o
# windows\COMPILAR.bat usa, e o toolchain comum de programas open source no
# Windows (bem menos alvo de falso positivo de antivirus do que o zig).
#   - usa o x86_64-w64-mingw32ucrt-g++ / x86_64-w64-mingw32-g++ do sistema, se tiver
#   - senao o MinGW em third_party/mingw (linux/fetch-mingw.sh baixa, sem root)
#   REMIX_WIN_TOOLCHAIN=zig bash linux/build-windows.sh  -> zig (antigo; antivirus pode acusar)
# Recursos: windows/app.rc (icone + versao) -> app_res.o e windows/manifest.rc ->
# default-manifest.o (manifesto; o GCC linka esse nome sozinho e o -B faz usar o nosso).
# Os dois .o ficam em windows/ ja compilados, para quem nao tem o windres.
# Fontes: comum/ + windows/. Saida: build/win/Remix.exe, windows/Remix.exe,
# dist/windows/Remix.exe e dist/remix-<ver>-windows-x64-portable.zip.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
VER="${REMIX_VERSION:-1.2.0}"
TC="${REMIX_WIN_TOOLCHAIN:-mingw}"
mkdir -p build/win dist/windows
LIBS=(-lgdiplus -lshell32 -lcomdlg32 -lole32 -luuid -lwinmm -lwinhttp -ldwmapi)

find_mingw() {
  for p in x86_64-w64-mingw32ucrt x86_64-w64-mingw32; do
    if command -v "$p-g++" >/dev/null 2>&1 && command -v "$p-windres" >/dev/null 2>&1; then
      local g; g="$(command -v "$p-g++")"; echo "${g%-g++}"; return 0
    fi
    if [ -x "$ROOT/third_party/mingw/usr/bin/$p-g++" ]; then echo "$ROOT/third_party/mingw/usr/bin/$p"; return 0; fi
  done
  return 1
}
# windres num temporario (so troca o .o se deu certo); caminhos do .rc sao relativos a windows/
winres() {
  local rc="$1" out="$2" tmp="$2.novo"
  if ! (cd windows && "$PFX-windres" -O coff "$rc" -o "$tmp" 2>/dev/null); then
    (cd windows && "$PFX-windres" --preprocessor="$PFX-gcc" --preprocessor-arg=-E --preprocessor-arg=-xc-header \
       --preprocessor-arg=-DRC_INVOKED -O coff "$rc" -o "$tmp")
  fi
  mv -f "windows/$tmp" "windows/$out"
}

if [ "$TC" = mingw ]; then
  PFX="$(find_mingw || true)"
  if [ -z "$PFX" ]; then bash linux/fetch-mingw.sh; PFX="$(find_mingw || true)"; fi
  [ -n "$PFX" ] || { echo "[win] MinGW-w64 nao encontrado: instale (Fedora: ucrt64-gcc-c++ / Debian: g++-mingw-w64-x86-64-posix) ou rode linux/fetch-mingw.sh"; exit 1; }
  echo "[win] compilando Remix.exe com MinGW-w64: $("$PFX-g++" --version | head -1)"
  winres app.rc app_res.o                     # icone + versao
  winres manifest.rc default-manifest.o       # manifesto
  "$PFX-gcc" -O2 -w -c -I"$ROOT/comum" comum/audio_backend.c -o build/win/audio_backend.o
  "$PFX-g++" -std=gnu++20 -O2 -w -municode -mwindows -static -s -B"$ROOT/windows/" -I"$ROOT/comum" -I"$ROOT/windows" \
    windows/main.cpp build/win/audio_backend.o windows/app_res.o -o build/win/Remix.exe "${LIBS[@]}"
else
  ZIG="$ROOT/third_party/zig/zig"
  [ -x "$ZIG" ] || { echo "zig nao encontrado: rode linux/fetch-deps.sh"; exit 1; }
  echo "[win] AVISO: compilando com zig. Exe feito com zig costuma levar falso positivo de antivirus; o padrao e MinGW."
  "$ZIG" cc -target x86_64-windows-gnu -O2 -w -c -I"$ROOT/comum" comum/audio_backend.c -o build/win/audio_backend.o
  "$ZIG" c++ -target x86_64-windows-gnu -std=gnu++20 -O2 -w -municode -DUNICODE -D_UNICODE -Wl,--subsystem,windows -I"$ROOT/comum" -I"$ROOT/windows" \
    windows/main.cpp build/win/audio_backend.o windows/app_res.o windows/default-manifest.o -o build/win/Remix.exe "${LIBS[@]}"
  rm -f build/win/Remix.pdb build/win/main.lib
fi
cp build/win/Remix.exe dist/windows/Remix.exe
cp build/win/Remix.exe windows/Remix.exe   # exe pronto na pasta windows/
# zip portatil Windows
PW="$ROOT/build/portable-win/remix-$VER-windows-x64"
rm -rf "$ROOT/build/portable-win"; mkdir -p "$PW/Musica" "$PW/assets/covers"
cp build/win/Remix.exe "$PW/"
cp -a assets/branding assets/themes "$PW/assets/"
cp Musica/LEIA-ME.txt "$PW/Musica/" 2>/dev/null || true
cp README.md "$PW/" 2>/dev/null || true
cp README.pt-BR.md "$PW/" 2>/dev/null || true
printf '[General]\r\nMusicFolder=Musica\r\n' > "$PW/config.ini"
OUT="dist/remix-$VER-windows-x64-portable.zip"; rm -f "$OUT"
(cd "$ROOT/build/portable-win" && zip -qr "$ROOT/$OUT" "remix-$VER-windows-x64")
echo "[win] ok -> dist/windows/Remix.exe, $OUT ($(du -h build/win/Remix.exe | cut -f1))"
