#!/usr/bin/env bash
# Remix Player - instala o que o Remix usa alem do proprio programa:
#   yt-dlp, ffmpeg e um JavaScript runtime (Deno 2.3+ ou Node.js 22+)  ->  buscar, tocar e baixar musica online
#   zenity (ou kdialog no KDE)                                        ->  janelas de escolher pasta e arquivo
#   libcurl                                                           ->  capas e links do Spotify, Deezer e Apple Music
# Descobre a distro pelo /etc/os-release e usa o caminho certo para cada uma:
#   Debian, Ubuntu, Mint, Pop!_OS, Zorin...   apt      yt-dlp e Deno oficiais (os do repositorio sao antigos)
#   Fedora, Nobara, RHEL, Rocky, Alma...      dnf      ffmpeg ou ffmpeg-free; sem nenhum, o ffmpeg estatico oficial
#   Arch, Manjaro, EndeavourOS, CachyOS...    pacman
#   openSUSE Tumbleweed e Leap                zypper
#   Void Linux e Solus                        xbps-install e eopkg
#   Bazzite, Silverblue, Kinoite, SteamOS e outras imutaveis, ou sem sudo: tudo na sua pasta
#     pessoal (~/.local/bin e ~/.deno/bin), sem mexer no sistema
#   Gentoo: yt-dlp, Deno e ffmpeg na pasta pessoal (sem compilar); NixOS: mostra o comando do nix
# O Remix procura as ferramentas nessas pastas sozinho.
#
#   bash instalar-dependencias.sh             mostra o que falta e pergunta antes de instalar
#   bash instalar-dependencias.sh --sim       instala sem perguntar
#   bash instalar-dependencias.sh --mostrar   so mostra o que faria (nao instala nada)
set -u
AUTO=0; SHOW=0
for a in "$@"; do
  case "$a" in
    --sim|-y) AUTO=1 ;;
    --mostrar|--dry-run) SHOW=1 ;;
    -h|--help) sed -n '2,19p' "$0"; exit 0 ;;
  esac
done
export PATH="$HOME/.local/bin:$HOME/.deno/bin:$PATH"
# Testes: REMIX_DEPS_FINGIR_FALTA=1 finge que nada esta instalado; REMIX_DEPS_OS_RELEASE=arquivo no lugar
# do /etc/os-release; REMIX_DEPS_PM=apt-get|dnf|pacman|zypper|xbps-install|eopkg|nenhum; REMIX_DEPS_IMUTAVEL=1
FAKE="${REMIX_DEPS_FINGIR_FALTA:-0}"

have() { [ "$FAKE" = 1 ] && return 1; command -v "$1" >/dev/null 2>&1; }
run() { if [ "$SHOW" = 1 ]; then echo "   [faria] $*"; return 0; fi; echo "   \$ $*"; "$@"; }
vnum() { "$@" 2>/dev/null | head -n1 | grep -oE '[0-9]+(\.[0-9]+)+' | head -n1; }
vge() { [ -n "$1" ] && [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]; }

ok_ytdlp() { have yt-dlp && vge "$(vnum yt-dlp --version)" 2025.11.0; }
ok_js() {
  if have deno && vge "$(vnum deno --version)" 2.3.0; then return 0; fi
  if have node && vge "$(vnum node --version)" 22.0.0; then return 0; fi
  if have bun && vge "$(vnum bun --version)" 1.2.11; then return 0; fi
  return 1
}
ok_ffmpeg() { have ffmpeg; }
ok_dialog() { have zenity || have kdialog; }
ok_curl() {
  [ "$FAKE" = 1 ] && return 1
  { ldconfig -p 2>/dev/null || /sbin/ldconfig -p 2>/dev/null || /usr/sbin/ldconfig -p 2>/dev/null; } | grep -qE 'libcurl(-gnutls)?\.so\.4'
}
line() { if "$2"; then printf '   [ok]     %s\n' "$1"; else printf '   [falta]  %s\n' "$1"; MISSING=1; fi; }
report() {
  MISSING=0
  local y="" j=""
  have yt-dlp && y="$(vnum yt-dlp --version)"
  have deno && j="$j deno $(vnum deno --version)"
  have node && j="$j node $(vnum node --version)"
  line "yt-dlp 2025.11 ou mais novo${y:+ (tem $y)}" ok_ytdlp
  line "ffmpeg" ok_ffmpeg
  line "Deno 2.3+ ou Node.js 22+${j:+ (tem$j)}" ok_js
  line "zenity ou kdialog (janelas de escolher pasta)" ok_dialog
  line "libcurl (capas e links)" ok_curl
}

# ---- qual distro ----
OSR="${REMIX_DEPS_OS_RELEASE:-/etc/os-release}"
OS_ID=""; OS_LIKE=""; OS_NAME=""; OS_VARIANT=""
if [ -r "$OSR" ]; then
  while IFS='=' read -r k v; do
    v="${v%\"}"; v="${v#\"}"
    case "$k" in ID) OS_ID="$v" ;; ID_LIKE) OS_LIKE="$v" ;; PRETTY_NAME) OS_NAME="$v" ;; VARIANT_ID) OS_VARIANT="$v" ;; esac
  done < "$OSR"
fi
FAMILY=""
for x in $OS_ID $OS_LIKE; do
  case "$x" in
    debian|ubuntu|linuxmint|pop|elementary|zorin|kali|raspbian|neon|deepin|pureos|mx) FAMILY=apt ;;
    fedora|rhel|centos|rocky|almalinux|nobara|ultramarine|ol|amzn) FAMILY=dnf ;;
    arch|manjaro|endeavouros|cachyos|garuda|artix|arcolinux|steamos) FAMILY=pacman ;;
    opensuse*|suse|sles|sled) FAMILY=zypper ;;
    void) FAMILY=xbps ;;
    solus) FAMILY=eopkg ;;
    gentoo) FAMILY=gentoo ;;
    nixos) FAMILY=nixos ;;
    alpine) FAMILY=alpine ;;
  esac
  [ -n "$FAMILY" ] && break
done
IMUTAVEL=0
[ "${REMIX_DEPS_IMUTAVEL:-0}" = 1 ] && IMUTAVEL=1
[ -z "${REMIX_DEPS_OS_RELEASE:-}" ] && [ -e /run/ostree-booted ] && IMUTAVEL=1
case "$OS_ID" in steamos|bazzite|aurora|bluefin|endless) IMUTAVEL=1 ;; esac
case "$OS_VARIANT" in silverblue|kinoite|sericea|onyx|*-atomic) IMUTAVEL=1 ;; esac

PM=""
case "$FAMILY" in apt) PM=apt-get ;; dnf) PM=dnf ;; pacman) PM=pacman ;; zypper) PM=zypper ;; xbps) PM=xbps-install ;; eopkg) PM=eopkg ;; esac
if [ -z "$FAMILY" ]; then for p in apt-get dnf pacman zypper xbps-install eopkg; do command -v "$p" >/dev/null 2>&1 && { PM="$p"; break; }; done; fi
if [ -n "${REMIX_DEPS_PM:-}" ]; then PM="$REMIX_DEPS_PM"; [ "$PM" = nenhum ] && PM=""
elif [ -n "$PM" ] && ! command -v "$PM" >/dev/null 2>&1; then PM=""; fi
[ "$IMUTAVEL" = 1 ] && PM=""
SUDO=""; SEMSUDO=0
if [ -n "$PM" ] && [ "$(id -u)" -ne 0 ]; then
  if command -v sudo >/dev/null 2>&1; then SUDO="sudo"; elif command -v doas >/dev/null 2>&1; then SUDO="doas"; else PM=""; SEMSUDO=1; fi
fi
ARCH="$(uname -m)"

pm_install() {
  case "$PM" in
    dnf) run $SUDO dnf install -y "$@" ;;
    apt-get) run $SUDO apt-get install -y "$@" ;;
    pacman) run $SUDO pacman -S --needed --noconfirm "$@" ;;
    zypper) run $SUDO zypper --non-interactive install "$@" ;;
    xbps-install) run $SUDO xbps-install -Sy "$@" ;;
    eopkg) run $SUDO eopkg install -y "$@" ;;
    *) return 1 ;;
  esac
}
install_one() {   # tenta os pacotes na ordem e para no primeiro que instalar
  local p
  for p in "$@"; do
    if [ "$PM" = apt-get ] && command -v apt-cache >/dev/null 2>&1 && ! apt-cache show "$p" >/dev/null 2>&1; then continue; fi
    pm_install "$p" && return 0
  done
  return 1
}
fetch() {   # url destino
  if command -v curl >/dev/null 2>&1; then run curl -fL --retry 3 -o "$2" "$1"
  elif command -v wget >/dev/null 2>&1; then run wget -O "$2" "$1"
  else echo "   [erro] precisa do curl ou do wget para baixar"; return 1; fi
}
arch_ok() {
  case "$ARCH" in x86_64|aarch64) return 0 ;; esac
  echo "   [erro] processador $ARCH: nao ha versao oficial pronta, instale $1 pela sua distro"; return 1
}
# ---- versoes oficiais na pasta pessoal (sem sudo) ----
user_ytdlp() {
  arch_ok yt-dlp || return 1
  echo "yt-dlp: versao oficial em ~/.local/bin"
  run mkdir -p "$HOME/.local/bin"
  if command -v python3 >/dev/null 2>&1 && python3 -c 'import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)'; then
    fetch "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp" "$HOME/.local/bin/yt-dlp" || return 1
  else
    local b="yt-dlp_linux"; [ "$ARCH" = aarch64 ] && b="yt-dlp_linux_aarch64"
    fetch "https://github.com/yt-dlp/yt-dlp/releases/latest/download/$b" "$HOME/.local/bin/yt-dlp" || return 1
  fi
  run chmod +x "$HOME/.local/bin/yt-dlp"
}
user_deno() {
  arch_ok Deno || return 1
  echo "Deno: versao oficial em ~/.deno/bin"
  local z="deno-x86_64-unknown-linux-gnu.zip" tmp
  [ "$ARCH" = aarch64 ] && z="deno-aarch64-unknown-linux-gnu.zip"
  tmp="$(mktemp -d)"
  fetch "https://github.com/denoland/deno/releases/latest/download/$z" "$tmp/deno.zip" || { rm -rf "$tmp"; return 1; }
  run mkdir -p "$HOME/.deno/bin"
  if command -v unzip >/dev/null 2>&1; then run unzip -o -q "$tmp/deno.zip" -d "$HOME/.deno/bin"; else run python3 -m zipfile -e "$tmp/deno.zip" "$HOME/.deno/bin"; fi
  run chmod +x "$HOME/.deno/bin/deno"
  rm -rf "$tmp"
}
user_ffmpeg() {
  arch_ok ffmpeg || return 1
  echo "ffmpeg: versao estatica oficial (BtbN, cerca de 100 MB) em ~/.local/bin"
  local a="linux64" tmp
  [ "$ARCH" = aarch64 ] && a="linuxarm64"
  tmp="$(mktemp -d)"
  fetch "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-$a-gpl.tar.xz" "$tmp/ff.tar.xz" || { rm -rf "$tmp"; return 1; }
  run tar -xJf "$tmp/ff.tar.xz" -C "$tmp"
  run mkdir -p "$HOME/.local/bin"
  run cp "$tmp/ffmpeg-master-latest-$a-gpl/bin/ffmpeg" "$HOME/.local/bin/ffmpeg"
  run chmod +x "$HOME/.local/bin/ffmpeg"
  rm -rf "$tmp"
}

echo "=============================================================="
echo "  Remix Player - dependencias"
echo "=============================================================="
if [ "$FAMILY" = nixos ]; then MODO="NixOS: instala pelo nix"
elif [ "$IMUTAVEL" = 1 ]; then MODO="distro imutavel: tudo na sua pasta pessoal"
elif [ -n "$PM" ]; then MODO="usa o $PM"
elif [ "$SEMSUDO" = 1 ]; then MODO="sem sudo: tudo na sua pasta pessoal"
else MODO="gerenciador de pacotes nao reconhecido: tudo na sua pasta pessoal"; fi
echo "  Sistema: ${OS_NAME:-${OS_ID:-desconhecido}}   ($MODO)"
if [ "$FAMILY" = alpine ] || { [ -z "${REMIX_DEPS_OS_RELEASE:-}" ] && ldd --version 2>&1 | grep -qi musl; }; then
  echo "  Aviso: esta distro usa a musl (como o Alpine); o Remix e feito para glibc e nao roda aqui."
fi
echo
report
if [ "$MISSING" = 0 ]; then echo; echo "Tudo pronto: e so abrir o Remix."; exit 0; fi
echo
if [ "$FAMILY" = nixos ]; then
  echo "No NixOS programas baixados soltos nao rodam. Instale pelo nix:"
  echo "   nix-shell -p yt-dlp ffmpeg deno zenity        (so nesta sessao do terminal)"
  echo "   ou ponha yt-dlp, ffmpeg, deno e zenity em environment.systemPackages e rode: sudo nixos-rebuild switch"
  exit 1
fi
if [ -n "$PM" ]; then
  echo "Plano: o que a distro tem em dia vem pelo $PM (pede a senha do $SUDO); o que faltar ou for antigo"
  echo "vem da versao oficial para a sua pasta pessoal."
else
  echo "Plano: versoes oficiais do yt-dlp, do Deno e do ffmpeg em ~/.local/bin e ~/.deno/bin (sem sudo)."
  if [ "$FAMILY" = gentoo ]; then echo "  Gentoo, se preferir pelo sistema: sudo emerge --ask media-video/ffmpeg net-misc/yt-dlp dev-lang/deno-bin gnome-extra/zenity"; fi
fi
if [ "$SHOW" = 0 ] && [ "$AUTO" = 0 ]; then
  printf 'Instalar o que falta agora? [S/n] '
  read -r resp || resp=n
  case "${resp:-s}" in s|S|sim|Sim|SIM|y|Y|yes) ;; *) echo "Nada foi instalado."; exit 0 ;; esac
fi

if [ -n "$PM" ]; then
  [ "$PM" = apt-get ] && run $SUDO apt-get update
  if ! ok_dialog; then case "${XDG_CURRENT_DESKTOP:-}" in *KDE*) install_one kdialog zenity ;; *) install_one zenity kdialog ;; esac; fi
  if ! ok_curl; then case "$PM" in dnf|xbps-install) install_one libcurl ;; apt-get) install_one libcurl4t64 libcurl4 ;; pacman|eopkg) install_one curl ;; zypper) install_one libcurl4 ;; esac; fi
  if ! ok_ffmpeg; then case "$PM" in dnf) install_one ffmpeg ffmpeg-free ;; zypper) install_one ffmpeg-7 ffmpeg-6 ffmpeg ;; *) install_one ffmpeg ;; esac; fi
  # yt-dlp e runtime JavaScript do repositorio so onde costumam estar em dia (no apt sao antigos)
  if [ "$PM" != apt-get ] && ! ok_ytdlp; then install_one yt-dlp; fi
  if ! ok_js; then case "$PM" in pacman|xbps-install) install_one deno nodejs ;; dnf) install_one nodejs deno ;; zypper) install_one nodejs22 ;; esac; fi
  hash -r 2>/dev/null
  if [ "$SHOW" = 1 ]; then echo "   Se ainda faltar algo depois do $PM:"; fi
else
  if ! ok_dialog; then echo "   Aviso: sem zenity ou kdialog o Remix nao abre a janela de escolher pasta; instale pela loja de apps da distro."; fi
  if ! ok_curl; then echo "   Aviso: falta a libcurl (capas e links); quase toda distro ja traz, instale pela loja de apps se precisar."; fi
fi
ok_ytdlp || user_ytdlp
ok_js || user_deno
ok_ffmpeg || user_ffmpeg
echo
if [ "$SHOW" = 1 ]; then echo "(--mostrar: nada foi instalado)"; exit 0; fi
echo "Conferindo:"
report
echo
if [ "$MISSING" = 0 ]; then echo "Tudo pronto: e so abrir o Remix."; else echo "Ainda falta algo acima. Veja as mensagens e rode de novo."; exit 1; fi
