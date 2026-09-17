#!/usr/bin/env bash
# build-all.sh - Remix Player: gera TODOS os binarios e pacotes a partir de um
# Linux (ou WSL), sem instalar nada no sistema:
#   build/remix + linux/remix        Linux x86_64 (glibc >= 2.27)
#   dist/*.deb, dist/*.rpm           pacotes (dpkg-deb / rpmbuild)
#   dist/*-linux-x86_64-portable.zip pasta portatil Linux
#   dist/Remix-*-x86_64.AppImage     AppImage (um arquivo, qualquer distro)
#   dist/windows/Remix.exe + windows/Remix.exe + dist/*-windows-x64-portable.zip   (cross-compilado com MinGW-w64)
# Uso: bash linux/build-all.sh   (REMIX_VERSION=1.5.6 bash linux/build-all.sh para mudar a versao)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
chmod +x linux/*.sh linux/packaging/*.sh 2>/dev/null || true
bash linux/fetch-deps.sh
bash linux/build.sh
bash linux/packaging/build-packages.sh
bash linux/build-windows.sh
echo
echo "== dist/ =="
ls -la dist/ dist/windows/ 2>/dev/null
