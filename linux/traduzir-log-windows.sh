#!/usr/bin/env bash
# Traduz os enderecos de um remix-log.txt do Windows ("Remix.exe+0x..." e "pilha (offsets no Remix.exe): ...")
# para funcao, arquivo e linha do codigo, com o Remix-sym.exe do MESMO build. O linux/build-windows.sh guarda
# uma copia por build em build/win/simbolos/Remix-sym-<id>.exe; o <id> e o da linha "build:" do log.
# So as 8 ultimas ficam: guarde as de uma release numa subpasta (ex.: build/win/simbolos/v1.2.0/), que nao e apagada.
#   bash linux/traduzir-log-windows.sh remix-log.txt [Remix-sym.exe]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
LOG="${1:-}"
if [ -z "$LOG" ] || [ ! -f "$LOG" ]; then echo "uso: bash linux/traduzir-log-windows.sh remix-log.txt [Remix-sym.exe]"; exit 1; fi

A2L=""
for c in x86_64-w64-mingw32ucrt-addr2line x86_64-w64-mingw32-addr2line \
         "$ROOT/third_party/mingw/usr/bin/x86_64-w64-mingw32ucrt-addr2line" "$ROOT/third_party/mingw/usr/bin/x86_64-w64-mingw32-addr2line"; do
  if command -v "$c" >/dev/null 2>&1; then A2L="$(command -v "$c")"; break; fi
done
[ -n "$A2L" ] || { echo "addr2line do MinGW-w64 nao encontrado: instale o binutils do MinGW ou rode linux/fetch-mingw.sh"; exit 1; }
NM="${A2L%addr2line}nm"

u4() { od -An -tu4 -j"$2" -N4 "$1" | tr -d ' '; }
pe_id() { od -An -tx4 -j$(( $(u4 "$1" 60) + 8 )) -N4 "$1" | tr -d ' ' | tr a-f A-F; }

LOGID="$(tr -d '\r' < "$LOG" | sed -n -E 's/^\[[^]]*\] build: ([0-9A-Fa-f]{8}).*/\1/p' | head -n1 | tr a-f A-F)"
SYM="${2:-}"
if [ -z "$SYM" ]; then
  SYM="$ROOT/build/win/Remix-sym.exe"
  if [ -n "$LOGID" ]; then   # tambem nas subpastas de release
    F="$(find "$ROOT/build/win/simbolos" -name "Remix-sym-$LOGID.exe" 2>/dev/null | head -n1)"
    if [ -n "$F" ]; then SYM="$F"; fi
  fi
fi
[ -f "$SYM" ] || { echo "nao achei o Remix-sym.exe ($SYM): compile com bash linux/build-windows.sh"; exit 1; }
SYMID="$(pe_id "$SYM")"
BASE=$(( 0x$(od -An -tx8 -j$(( $(u4 "$SYM" 60) + 48 )) -N8 "$SYM" | tr -d ' ') ))
echo "log:      $LOG (build ${LOGID:-sem id})"
echo "simbolos: ${SYM#"$ROOT"/} (build $SYMID)"
if [ -z "$LOGID" ]; then echo "AVISO: log sem a linha \"build:\" (Remix antigo): confira se o exe e o destes simbolos."
elif [ "$LOGID" != "$SYMID" ]; then echo "AVISO: o log e de outro build e nao achei build/win/simbolos/Remix-sym-$LOGID.exe: os nomes podem sair errados."; fi
echo "(\"Remix.exe+0x...\" e o ponto exato do erro; a \"pilha\" e aproximada: sobras antigas da memoria tambem aparecem)"
echo

NMCACHE=""
trap 'if [ -n "$NMCACHE" ]; then rm -f "$NMCACHE"; fi' EXIT
short() {
  sed -E -e "s#$ROOT/##g" \
    -e 's/std::__cxx11::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t> >/std::wstring/g' \
    -e 's/std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >/std::string/g'
}
nearest() {   # sem linha de debug (codigo de biblioteca): simbolo mais proximo pelo nm
  if [ -z "$NMCACHE" ]; then NMCACHE="$(mktemp)"; "$NM" -C --defined-only --numeric-sort "$SYM" 2>/dev/null | awk '$2 ~ /^[tT]$/' > "$NMCACHE"; fi
  local hex s; hex=$(printf '%016x' "$1")
  s=$(awk -v a="$hex" '($1 "") <= (a "") { b = $0; next } { exit } END { print b }' "$NMCACHE")
  [ -n "$s" ] || { echo "??"; return 0; }
  printf '%s+0x%X\n' "$(printf '%s' "$s" | sed -E 's/^[0-9a-f]+ [tT] //')" $(( $1 - 0x${s%% *} ))
}
translate() {   # $1 = offset em hex, sem 0x
  local addr=$(( BASE + 0x$1 )) out
  out=$("$A2L" -f -C -i -p -e "$SYM" "$(printf '0x%x' "$addr")" 2>/dev/null | short || true)
  case "$out" in ""|"??"*) out="$(nearest "$addr" | short) (sem linha)" ;; esac
  printf '%s\n' "$out" | sed -e "1s/^/      0x$1  /" -e '2,$s/^/            /'
}

while IFS= read -r line; do
  printf '%s\n' "$line"
  case "$line" in
    *"pilha (offsets no Remix.exe):"*)
      for off in $(printf '%s\n' "${line##*Remix.exe):}" | grep -o -E '0x[0-9A-Fa-f]+' || true); do translate "${off#0x}"; done ;;
    *" Remix.exe+0x"*)
      translate "$(printf '%s\n' "$line" | sed -E 's/.* Remix\.exe\+0x([0-9A-Fa-f]+).*/\1/')" ;;
  esac
done < <(tr -d '\r' < "$LOG")
