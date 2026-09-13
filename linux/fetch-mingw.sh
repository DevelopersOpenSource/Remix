#!/usr/bin/env bash
# Baixa o MinGW-w64 (GCC + binutils + headers/CRT) para compilar o Remix.exe a
# partir do Linux, SEM instalar nada no sistema: tudo fica em third_party/mingw/.
# So precisa disto se o sistema nao tiver o MinGW instalado.
#   Fedora/RHEL/Nobara: dnf download dos pacotes ucrt64-*  (alvo UCRT, igual ao MSYS2 UCRT64)
#   Debian/Ubuntu/Mint: apt-get download dos pacotes mingw-w64 (posix threads)
#   (ou instale de vez: sudo dnf install ucrt64-gcc-c++  /  sudo apt install g++-mingw-w64-x86-64-posix)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TP="$ROOT/third_party"; OUT="$TP/mingw"; PK="$TP/mingw-pkgs"
if [ -x "$OUT/usr/bin/x86_64-w64-mingw32ucrt-g++" ] || [ -x "$OUT/usr/bin/x86_64-w64-mingw32-g++" ]; then
  echo "[mingw] ja esta em third_party/mingw"; exit 0
fi
rm -rf "$PK"; mkdir -p "$PK" "$OUT"
if command -v dnf >/dev/null; then
  echo "[mingw] baixando MinGW-w64 UCRT64 (Fedora, ~100 MB, sem instalar)..."
  (cd "$PK" && dnf download --resolve --destdir . ucrt64-gcc-c++ ucrt64-gcc ucrt64-cpp ucrt64-binutils \
     ucrt64-headers ucrt64-crt ucrt64-winpthreads ucrt64-winpthreads-static ucrt64-filesystem >/dev/null)
  for f in "$PK"/*.rpm; do bsdtar -C "$OUT" -xf "$f"; done
elif command -v apt-get >/dev/null; then
  echo "[mingw] baixando MinGW-w64 (Debian/Ubuntu, sem instalar)..."
  (cd "$PK" && apt-get download g++-mingw-w64-x86-64-posix gcc-mingw-w64-x86-64-posix gcc-mingw-w64-base \
     binutils-mingw-w64-x86-64 mingw-w64-x86-64-dev mingw-w64-common >/dev/null)
  for f in "$PK"/*.deb; do dpkg -x "$f" "$OUT"; done
  # no Debian os nomes vem com sufixo -posix (update-alternatives nao roda aqui)
  for t in gcc g++ cpp; do
    [ -e "$OUT/usr/bin/x86_64-w64-mingw32-$t" ] || ln -sf "x86_64-w64-mingw32-$t-posix" "$OUT/usr/bin/x86_64-w64-mingw32-$t"
  done
else
  echo "[mingw] instale o MinGW-w64 pelo gerenciador de pacotes da sua distro e rode de novo."; exit 1
fi
rm -rf "$PK"
G="$(ls "$OUT"/usr/bin/x86_64-w64-mingw32*-g++ 2>/dev/null | head -1)"
[ -n "$G" ] && "$G" --version | head -1 && echo "[mingw] ok -> third_party/mingw" || { echo "[mingw] falhou (instale pelo gerenciador de pacotes)"; exit 1; }
