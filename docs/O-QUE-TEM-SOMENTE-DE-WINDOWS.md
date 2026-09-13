# O que tem somente de Windows — Remix v1.0

> Inventário completo das dependências exclusivas do Windows no código-fonte.
> Base: `remix v1.0` (main.cpp, player.h, playlist.h, config.h, theme.h).
> Complemento: veja `PLANO-PARA-LINUX.md` para o caminho de portabilidade.

---

## Resumo executivo

| Camada | Tecnologia Windows | Reaproveitável no Linux? |
|---|---|---|
| Áudio | Media Foundation + COM (`player.h` inteiro) | ❌ Reescrever |
| Renderização 2D | GDI+ (~45% do main.cpp) | ❌ Reescrever |
| Janela/eventos | Win32 USER32 (WndProc, WM_TIMER...) | ❌ Reescrever |
| HTTP | WinHTTP | ❌ Trocar (libcurl) |
| Diálogos/pastas | shell32 + comdlg32 | ❌ Trocar |
| Watch de pasta | ReadDirectoryChangesW | ❌ Trocar (inotify) |
| Instância única/IPC | Mutex + WM_COPYDATA | ❌ Trocar (socket Unix) |
| Codificação de texto | MultiByteToWideChar etc. | ❌ Trocar (UTF-8 nativo) |
| Lógica INI/scan/ID3/temas/efeitos | C++ padrão | ✅ **~55–60% do código** |

**Conclusão:** o que é "carne" do app (biblioteca, tags ID3, temas, efeitos matemáticos, estado do player) já é C++ portátil. O que é casca (janela, som, desenho) é 100% Win32/GDI+/MF.

---

## 1. Áudio — Media Foundation (100% do `player.h`, 247 linhas)

Toda a engine de som é Windows-only:

- `MFStartup` / `MFShutdown` — player.h:108-109
- `MFCreateSourceReaderFromURL` — player.h:60, 202 (decodificação MP3/WAV)
- `IMFPMediaPlayer` + `IMFPMediaPlayerCallback` — player.h:19, 37 (playback via COM)
- `MFCreateMediaType` — player.h:204
- `IMFSimpleAudioVolume` / `IMFAudioStreamVolume` (volume)
- Linkagem: `-lmfplat -lmfplay -lmfreadwrite -lmfuuid -lole32`

**Uso no app:** Open/Play/Pause/SeekMs/GetPositionMs/GetLengthMs + análise PCM para a onda/espectro FFT (`AnalyzeCurrentWave`, `SpecThreadLoop`).

## 2. Renderização — GDI+ (~45% do main.cpp)

Todo o visual é desenhado com GDI+:

- `Graphics`, `Bitmap`, `Image`, `SolidBrush`, `Pen`, `RectF`, `Color`
- `DrawRoundRect`, `DrawParticles`, `DrawRunnerRect/Circle`, `DrawCoverCircle`, `DrawMarqueeText` (main.cpp:165-405)
- Cache de fundo em bitmap: `g_bgCache`, `InvalidateBgCache` (main.cpp:771-773), blur da capa via GDI+
- Thumbnails: `g_thumbCache` (map caminho → `Gdiplus::Image*`)
- Splash: `Image` PNG (main.cpp:1594)
- Capas otimizadas v1.0: `SaveResized`/`ShrinkCoverInto` usam `Gdiplus::Bitmap::Save` + encoders JPEG/PNG (playlist.h)
- **Fontes:** `FontFamily(L"Segoe UI")` (main.cpp:1109) e `FontFamily(L"Segoe UI Symbol")` (main.cpp:753) — fontes que não existem fora do Windows; os ícones ⚙ ✎ ⇄ ⟳ ✕ dependem delas

## 3. Janela, eventos e loop — Win32 USER32 (~86 pontos no main.cpp)

- `RegisterClassW("MusicPlayerRemixWnd")`, `CreateWindowExW`, `DefWindowProcW`
- `WndProc`: `WM_PAINT`, `WM_TIMER` (animação a ~16 ms), `WM_KEYDOWN` (`VK_SPACE/VK_RIGHT/VK_LEFT/R`), `WM_ERASEBKGND`, `WM_SIZE/SIZE`, `WM_APP+7..12`
- `BeginPaint`/`EndPaint`, `InvalidateRect`, `SetTimer`/`KillTimer`
- Redimensionamento pelas bordas e clamp na área de trabalho

## 4. Rede — WinHTTP

Busca e download das capas da web:

- `HttpGetBytes` + `WebSearchAsync`/`WebDownloadSelectedAsync` (main.cpp:1724-1832)
- 12+ funções `WinHttp*` (Open/Connect/OpenRequest/SendRequest/ReceiveResponse/ReadData...)
- Linkagem: `-lwinhttp`

## 5. Sistema de arquivos e diálogos

- Diálogo de pasta: `SHBrowseForFolderW` + `SHGetPathFromIDListW` (`PickFolder`, main.cpp:738)
- Diálogo de imagem: `GetOpenFileNameW` (`PickImageFile`, main.cpp:743)
- Temp: `GetTempPathW` (main.cpp:1754, 1801)
- **Watch de pasta:** `ReadDirectoryChangesW` + `CancelIoEx` (main.cpp:614-653)
- **Scan do PC:** `GetLogicalDriveStringsW` + `GetDriveTypeW` (`ScanComputerMusic`, playlist.h:391-397); exclusão de junctions via `FILE_ATTRIBUTE_REPARSE_POINT` (playlist.h:251-252)
- `CopyFileW` / `MoveFileW` / `DeleteFileW` / `SetFileAttributesW` (capas)

## 6. Texto e codificação (17 pontos)

- `MultiByteToWideChar` / `WideCharToMultiByte` — config.h:15-27 (`Utf8ToWide`/`WideToUtf8`) e playlist.h (`DecodeID3Text`)
- Strings em UTF-16 (`wchar_t`, `-municode`, `wWinMain`) — Linux usa UTF-8 nativo
- `MAX_PATH` em buffers fixos

## 7. Instância única + IPC (`--play-file`)

- Mutex nomeado: `CreateMutexW(L"RemixPlayer.SingleInstance")` (main.cpp:2279)
- Segunda instância localiza a primeira por `FindWindowW` e envia comando via `WM_COPYDATA`/`COPYDATASTRUCT` (`SendToExisting`, main.cpp:2275)

## 8. COM / OLE

- `CoInitializeEx(COINIT_APARTMENTTHREADED)` (main.cpp:2278), `CoTaskMemFree`
- `GlobalAlloc` + `CreateStreamOnHGlobal` (migração de capas, playlist.h) — decodificar imagem da memória
- `_bstr_t`/GUIDs: `__uuidof`, `IID_IUnknown`

## 9. Build e recursos

- `app.rc` + `windres` (ícone embutido) — Linux usa `.desktop` + tema de ícones freedesktop
- Flags Windows-only: `-municode`, `-mwindows`; libs: `gdiplus shell32 comdlg32 mfplat mfplay mfreadwrite mfuuid ole32 uuid winmm winhttp`
- Binário PE `.exe` → ELF sem extensão

---

## ✅ O que JÁ é portável (não precisa mexer)

| Componente | Onde | Por quê |
|---|---|---|
| Leitura/gravação INI UTF-8 | config.h | só `fstream` |
| Parser ID3v2/ID3v1 | playlist.h:23-136 | binário puro (menos conversão de encoding, item 6) |
| Scan de pastas/biblioteca | playlist.h (`WalkFiles`, `ScanFolder`) | `std::filesystem` |
| Sistema de temas | theme.h | só fstream/filesystem |
| Efeitos matemáticos | partículas, glitch, runner, onda, FFT | matemática pura — muda só quem desenha |
| Estado do player | fila, shuffle, repeat, escalas, splash timer | lógica pura |
| Threading | `std::thread/mutex/atomic` | padrão C++ |
| Hash/nomes de capas | `std::hash` + swprintf | padrão |

Detalhe único de plataforma dentro do "portável": `Config::ExeDir()` (config.h:89) usa `GetModuleFileNameW` → trocar por leitura de `/proc/self/exe`.
