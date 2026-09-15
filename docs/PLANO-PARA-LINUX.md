# Plano para versão Linux — Remix

> Estratégia pensada a partir do inventário em `O-QUE-TEM-SOMENTE-DE-WINDOWS.md`.
> Objetivo: mesma estética e mesmos recursos, rodando nativo no Linux, reaproveitando o máximo do código atual.

---

## 1. Estratégia escolhida: camadas de plataforma

O código já está naturalmente dividido entre **lógica portável** (~55–60%) e **casca Windows**. Em vez de reescrever tudo num framework novo, o caminho é cercar a casca com 4 interfaces C++ e implementá-las por sistema:

```
┌──────────────────────────────────────────────┐
│ Núcleo (portável — já existe hoje)           │
│ config.h · playlist.h · theme.h ·            │
│ estado do player · efeitos (matemática)      │
└──────────────┬───────────────────────────────┘
               │ usa apenas ↓
   ┌───────────┼─────────────┬──────────────┐
IAudioEngine  IGraphics   IPlatform     INetClient
(áudio/PCM)   (desenho)   (janela/eventos/ (HTTP)
                           diálogos/watch/
                           instância única)
```

Regra de ouro da refatoração: **nenhum `#include <windows.h>` acima da linha das interfaces.**

---

## 2. Mapa de substituição por componente

| Função | Hoje (Windows) | Linux recomendado | Alternativas |
|---|---|---|---|
| Playback MP3/WAV + seek + volume | Media Foundation | **miniaudio** (+dr_mp3 embutido) — header-only, sem dependências | GStreamer (pesado mas completo), SDL2_audio |
| Decodificação PCM p/ onda/FFT | MF Source Reader | **miniaudio** decodificando p/ buffer float | libmpg123 + libsndfile |
| Desenho 2D (cards, partículas, runner, blur) | GDI+ | **raylib** (imediato, estilo igual ao Draw*) | SDL2 + SDL2_image, Cairo |
| Janela, teclado, timer de animação | USER32 + WM_TIMER | a própria raylib/SDL traz janela+eventos+timestep | GTK/GLFW |
| Texto e ícones ⚙ ✎ ⇄ ⟳ ✕ | Segoe UI / Segoe UI Symbol | **empacotar Noto Sans Symbols 2 + DejaVu Sans** (TTF na pasta assets/fonts) | fontconfig p/ achar fonte do sistema |
| HTTP (busca/download capa) | WinHTTP | **libcurl** (`libcurl4` presente em toda distro) | cpp-httplib (header-only) |
| Watch de pasta | ReadDirectoryChangesW | **inotify** (API do kernel, sem libs) | fanotify |
| Diálogo escolher pasta/imagem | SHBrowseForFolder / GetOpenFileName | **zenity/kdialog** (fallback simples) → depois xdg-desktop-portal | GTK nativo |
| Instância única | Mutex nomeado | **lockfile** em `$XDG_RUNTIME_DIR/remix.lock` (flock) | D-Bus |
| IPC `--play-file` | WM_COPYDATA + FindWindow | **Unix domain socket** em `$XDG_RUNTIME_DIR/remix.sock` | D-Bus |
| Temp | GetTempPathW | `/tmp` ou `$TMPDIR` | — |
| ExeDir | GetModuleFileNameW | ler link `/proc/self/exe` | argv[0] |
| Enumerar drives | GetLogicalDriveStringsW | varrer `/media/$USER`, `/run/media/$USER`, `/mnt` + filtrar `/proc`,`/sys`,`/dev` | udisks2 |
| Junction/reparse skip | FILE_ATTRIBUTE_REPARSE_POINT | `fs::is_symlink` / `status().type()` | — |
| Ícone do app | app.rc + windres | `.desktop` + ícone em `~/.local/share/icons` | empacotado |
| Codificação ID3 UTF-8↔UTF-16 | MultiByteToWideChar | ir para **UTF-8 nativo** (`char`) — decode UTF-16 manual já existe no parser ID3 | iconv |

### Por que miniaudio + raylib como recomendação padrão?

1. **Zero dependências externas** — filosofia idêntica ao projeto atual ("sem libs externas" no ID3). Ambos são single-file/header-only e compilam com o mesmo g++.
2. raylib desenha do mesmo jeito que o código atual pensa: "por frame, desenhe tudo" — os `Draw*` traduzem quase 1:1 (`DrawRoundRect`→`DrawRectangleRounded`, `Image`→`Texture2D`, marquee→clip mode).
3. O binário continua pequeno e estático (AppImage fácil).

---

## 3. Mudanças estruturais necessárias no núcleo

1. **UTF-8 em vez de UTF-16**: trocar `std::wstring`→`std::string` nas assinaturas do núcleo (ou criar `typedef` por plataforma). É o refactor mais invasivo; fazer UMA vez, cedo.
2. **Notificações thread→UI**: hoje são `PostMessageW(WM_APP+n)`; viram uma fila mutex-protégida que o loop principal drena a cada frame (`g_events.push({MIGRATION_DONE})` etc.). Os WM_APP+7..12 mapeiam direto pra eventos.
3. **Timer**: `WM_TIMER` vira timestep do loop principal (raylib: `GetFrameTime()`).
4. **Caminhos XDG**: config vai para `~/.config/remix/` (config.ini, covers.ini, artists.ini), capas em `~/.local/share/remix/covers`. **Manter o modo portátil**: se existir `config.ini` ao lado do binário, usar ele (comportamento atual preservado).
5. `Config::ExeDir()` → `/proc/self/exe`.

## 4. Roadmap em fases

| Fase | Entrega | Esforço relativo |
|---|---|---|
| **0** | Inventário Windows-only (este par de docs) | feito |
| **1** | Refactor p/ camadas: extrair `IAudioEngine/IGraphics/IPlatform`; UTF-8; fila de eventos | ~40% do trabalho |
| **2** | Núcleo compilando no Linux SEM UI + testes de scan/INI/ID3 via CLI | pequeno |
| **3** | Backend áudio (miniaudio): play/pause/seek/volume + PCM p/ onda | médio |
| **4** | Backend gráfico (raylib): janela, cards normal/vertical, splash, partículas/glitch/runner, settings | **maior fatia** |
| **5** | Web (curl), watch (inotify), instância única (socket), diálogos (zenity/portal) | pequeno |
| **6** | Empacotamento: AppImage + .deb, `.desktop`, ícone, teste Wayland/X11 e HiDPI | médio |

## 5. Riscos e detalhes que costumam morder

- **Fontes/símbolos**: ⚙ ✎ ⇄ ⟳ ✕ precisam da Noto Symbols; sem ela viram quadrados. Empacotar o TTF resolve em qualquer distro.
- **HiDPI/Wayland**: escalas manuais do app (uiScale etc.) ajudam; raylib/SDL2 lidam bem se configurados; testar fração de escala.
- **Blur da capa**: GDI+ não existe — reimplementar downscale+stackblur no backend gráfico (barato: é cache de bitmap, roda 1x).
- **AppImage + glibc**: compilar na distro base mais antiga suportada (ex.: Ubuntu 20.04 container) para compatibilidade.
- **MP3 legal/histórico**: nada muda — decodificação local, sem streaming.

## 6. Estimativa de reaproveitamento

| Bloco | Linhas hoje | Destino |
|---|---|---|
| playlist.h + config.h + theme.h | ~770 | ✅ ~90% direto (após UTF-8) |
| Lógica de estado/UI-math no main.cpp | ~900 | ✅ ~80% (eventos/timer adaptados) |
| player.h (MF) | 247 | ♻ interface mantida, backend novo |
| Draw*/layout GDI+ | ~1100 | 🔁 reescrito no backend raylib |
| WinHTTP/diálogos/watch/mutex | ~200 | 🔁 reescritos (peças pequenas) |

**Resumo:** ~55–60% de aproveitamento real; o que se reescreve é casca bem delimitada.

---

## 7. Status — porte concluído (2026-09-04, núcleo unificado em 2026-09-05)

O que foi feito, e onde o caminho real divergiu do plano:

| Item do plano | Resultado |
|---|---|
| Fases 1–6 | concluídas: núcleo portável, backend miniaudio, backend raylib, web/watch/instância única/diálogos, `.deb` + `.rpm` |
| UTF-16 → UTF-8 no núcleo | **não** foi feito: o núcleo continua em `std::wstring` (UTF-32 no Linux) com codec UTF-8 manual em `config.h` e `platform.h` fornecendo `BYTE/DWORD/COLORREF/RECT`. Evitou reescrever o `main.cpp` do Windows e manteve o diff pequeno |
| 4 interfaces (IAudioEngine/IGraphics/IPlatform/INetClient) | virou headers da casca Linux (`player_linux.h`, `gfx.h`, `sys_linux.h`) com a mesma "cara" de chamada do GDI+/Win32, em vez de interfaces abstratas usadas pelos dois lados |
| Espectro ao vivo (SpecThreadLoop) | substituído por espectrograma pré-calculado na mesma passagem que gera a onda (48 bandas por ~50 ms), consultado por posição |
| Diálogos via xdg-desktop-portal | ficou em `zenity`/`kdialog` em thread (portal exigiria D-Bus) |
| Empacotamento AppImage | não feito; `.deb` e `.rpm` prontos em `packaging/` |
| Compatibilidade glibc | build padrão com **zig** (`-target x86_64-linux-gnu.2.27`): binário exige só glibc ≥ 2.27 (Ubuntu 18.04+). No caminho g++, `linux/compat_glibc.cpp` segura em 2.35 |
| Modo portátil | `remix` + `RODAR.sh` na raiz do projeto e `dist/*-portable.zip`: extrair e rodar, config/capas na própria pasta |
| Toolchain | Linux: zig (`-target x86_64-linux-gnu.2.27`). Windows: MinGW-w64/GCC desde 2026-09-10 (o exe feito com zig levava falso positivo de antivírus); zig só como opção (`REMIX_WIN_TOOLCHAIN=zig`). Scripts: `windows/BUILD-TUDO.ps1` / `linux/build-all.sh` |
| 1.1 (2026-09-05) | autoplay, ordem da playlist com modo manual salvo, EQ 8 bandas, FLAC/OGG nativos + ffmpeg para o resto, engine na taxa nativa, janela lembrada; bugs de seek/scroll corrigidos nos dois sistemas |
| 1.2 (2026-09-05) | o que o plano chamava de "núcleo portável" virou realidade completa: `app_core.h` / `app_layout.h` / `app_input.h` são usados pelas DUAS cascas (Windows `main.cpp` + `win/`, Linux `linux/`), e o Windows migrou de Media Foundation para o mesmo miniaudio. Sugestões dos testadores implementadas nos dois lados (volume/mudo, atalhos, fechar/minimizar, menu PASTA, renomear/excluir, modo leve, varredura só nas pastas do usuário). O `Remix.exe` cruzado foi verificado em runtime no Wine (screenshots + reprodução). Em seguida: modo segundo plano (fechar continua tocando; bandeja no Windows, MPRIS via libdbus/dlopen no Linux; teclas de mídia; ajustável nas configurações), `FLAG_WINDOW_ALWAYS_RUN` (minimizado continua), e AppImage (`linux/build-appimage.sh`, appimagetool + runtime estático em `third_party/appimage/`). Por último: busca, playlists (`app_playlists.h`: pasta + JSON com caminhos, correção automática por nome/tamanho), atalhos configuráveis (`app_keys.h`, FOCO/GLOBAL; Windows RegisterHotKey, Linux XGrabKey + `remix --cmd`), fila do aleatório, e correções de X das configurações, LED e troca de modo |

Detalhes e instruções em `linux/README-LINUX.md`.
