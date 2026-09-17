# Remix Player

A lightweight music player for **Windows and Linux** (C++, one shared core, ~3 MB). Play your local
library, organize playlists and stream or download music from **YouTube, YouTube Music, SoundCloud,
Spotify, Deezer and Apple Music** links.

> The interface is currently in Brazilian Portuguese. Documentação em português: [README.pt-BR.md](README.pt-BR.md).

<p align="center">
  <img src="docs/screenshots/biblioteca.png" alt="Library grid (Linux)" width="32%">
  <img src="docs/screenshots/busca-online.png" alt="Online search (Linux)" width="32%">
  <img src="docs/screenshots/windows-lista.png" alt="List view (Windows)" width="32%">
</p>

## Credits

<p align="center">
  <a href="https://github.com/Nero-2077"><img src="https://raw.githubusercontent.com/EchoGroupStudio/Remix/main/docs/creditos/nero-2077.en.svg" alt="Nero-2077: author of Remix, original idea and Windows version" width="48%"></a>
  <a href="https://github.com/NinjaZinS2"><img src="https://raw.githubusercontent.com/EchoGroupStudio/Remix/main/docs/creditos/sodre.en.svg" alt="Sodre (NinjaZinS2): co-developer, playlists, streaming, Linux version and Host for iOS" width="48%"></a>
</p>

**[Nero-2077](https://github.com/Nero-2077)** created Remix: the original idea and the Windows version.
**Sodre** ([NinjaZinS2](https://github.com/NinjaZinS2)) joined later as co-developer: worked on the Windows version,
proposed and built playlists and online music (streaming and downloads), brought Remix to Linux, and came up with **Host** —
the PC acting as a server for the phone — so Remix reaches the **iPhone (iOS) and other mobile devices** while Nero builds the Android version.

## Download

Portable builds are on the [Releases page](https://github.com/EchoGroupStudio/Remix/releases), nothing to install:

| System | File |
|---|---|
| Windows 10/11 (x64) | `remix-<version>-windows-x64-portable.zip`: extract the whole folder and run `Remix.exe` |
| Linux (x86_64, any distro) | `remix-<version>-linux-x86_64-portable.zip`: extract and run `./RODAR.sh` |

Linux builds only need glibc 2.27 or newer (Ubuntu 18.04+, Debian 10+, Fedora...). A `.deb`, a `.rpm` and an
AppImage can be built with `bash linux/packaging/build-packages.sh` (see [Building from source](#building-from-source)).

### Online music requirements

Streaming, downloads and online search use free external tools that are **not bundled**: **yt-dlp**, **FFmpeg**
and a JavaScript runtime, which YouTube now requires (**Deno 2.3+** or **Node.js 22+**). The portable versions
include an installer:

- **Windows:** double-click `INSTALAR-DEPENDENCIAS.bat` in the Remix folder. It downloads the official yt-dlp,
  FFmpeg and Deno builds straight into `assets\tools` **inside** Remix, using the `curl` and `tar` built into
  Windows 10 1803+ and 11 — nothing is installed globally (no pip, no winget, no PATH). Remix only loads these
  tools from its own `assets\tools` folder. On Windows without `curl`/`tar` it opens App Installer in the
  Microsoft Store or the official download pages.
- **Linux:** `bash instalar-dependencias.sh` in the portable folder. It detects the distribution and uses its package
  manager (apt on Debian/Ubuntu/Mint, dnf on Fedora/Nobara/RHEL, pacman on Arch/Manjaro/CachyOS, zypper on openSUSE,
  xbps on Void, eopkg on Solus). When the packaged yt-dlp or JavaScript runtime is too old, on immutable systems
  (Bazzite, Silverblue, SteamOS) or without sudo, it downloads the official yt-dlp, Deno and static FFmpeg builds into
  `~/.local/bin` and `~/.deno/bin`. On NixOS it prints the `nix` command instead; `--mostrar` only shows the plan.

Then just open Remix: it finds the tools by itself, no restart needed. *Settings › ONLINE* shows what was found.
Run the installer again from time to time: YouTube changes often and yt-dlp must stay up to date.

**Optional — stem separation:** the installers also offer the **STEMS** option (`/stems` on Windows, `--stems` on
Linux, about 1 GB): an isolated Python with CPU PyTorch and [Demucs](https://github.com/facebookresearch/demucs)
(Meta, open source), only for Remix (`assets\tools\stems` on Windows, `~/.local/share/remix/stems` on Linux).

### If Remix closes by itself (Windows)

Remix writes `remix-log.txt` next to `Remix.exe` (or in `%LOCALAPPDATA%\Remix`) with every startup step, the
Windows version, the audio device and the error, and shows a message instead of just disappearing. The next launch
starts in **safe mode** (no splash sound, effects, tray icon or global hotkeys; force it with `Remix.exe --seguro`).
Please open an issue with that file. Also check that the whole folder was extracted (do not run `Remix.exe` from
inside the `.zip`) and that your antivirus did not quarantine it.

## Features

**Library**
- Scans your Music, Downloads, Documents and Desktop folders (skipping game and app folders) or a folder you choose, and watches it for changes.
- MP3, WAV, FLAC and OGG built in; M4A, AAC, Opus, WMA and more through ffmpeg.
- Tags and **embedded cover art** (MP3, M4A/MP4, FLAC, OGG/Opus, WAV/AIFF); custom covers from a file or a web image search.
- Search, sorting (title, artist, file, date or manual), rename or delete files, edit artist names.

**Playlists**
- Each playlist is `playlists/<name>/playlist.json` with file paths only: nothing is copied.
- Add songs from the library, pick files, add a whole folder, **link a folder** (always in sync), paste a link or search online.
- Files that moved are found again by name and size. Shuffle is a play queue, so your custom order stays intact.

**Online music**
- Search YouTube Music, YouTube or SoundCloud, or paste links to tracks, albums and playlists from YouTube / YouTube Music, SoundCloud, **Spotify, Deezer and Apple Music**.
- Spotify, Deezer and Apple Music links provide title, artist and duration (Spotify through its public embed page: no account or API key), and each song is matched on YouTube Music. No DRM is circumvented: audio always comes from YouTube or SoundCloud.
- **In-memory streaming:** yt-dlp finds the audio URL and ffmpeg decodes straight to RAM. Nothing is written to disk and seeking works.
- **Streaming queue:** the current song and the next two each get their own channel and preload in the background, so skipping (or reaching the end of a song) starts the next one instantly.
- **Fast start:** yt-dlp's audio lookup (~3 s) is cached for 25 minutes and shared by the player, the phone (Host) and downloads, and the first search results are looked up in the background — playing one of them starts in about half a second.
- **Downloads** (MP3, M4A or the original format, with tags and cover) run **several at once** (half your CPU cores, 2 to 6) and reuse the lookup already done by streaming or search; files are assembled in the cache and moved to `Music/Remix Online/<playlist>` only when complete. In download mode a song starts playing by streaming right away while it downloads.
- Streaming or download can be chosen globally or per playlist. A small journal in the cache lets the app clean up interrupted downloads on the next start.

**Sound: safe volume, effects, stems and a wave that follows the beat**
- **Safe volume:** perceptual (cubic) volume curve, a master stage that rises at most 40 dB per second and a limiter at
  −0.3 dBFS. Before, the slider was linear (3% was already −30 dB) and jumping to 100% could blast your headphones;
  bass boost or the equalizer could also clip.
- **Effects (header › EFFECTS):** Slow, Speed, Reverb, Bass and 8D, each with 3 levels (click cycles 1 → 2 → 3 → off).
  Slow/Speed change tempo and pitch together ("slowed"/"sped up").
- **Stems:** Full, Vocals only, Music only, Drums, Bass and Other, separated in the background with Demucs (optional,
  see above). On a CPU the first separation takes about half of the song's length; the full song keeps playing, Remix
  switches to the chosen stem at the same position when it is ready, and the stems are cached (switching is instant
  afterwards). With a stem mode on, the next songs in the queue are separated ahead.
- **Rhythm wave:** the wave follows the real audio — fine 25 ms energy, beats detected from the spectrum and the output
  latency compensated (before it read 50 ms ahead and drifted up to 4% over a song).

**Look and feel**
- Three interface styles (Settings > INTERFACE STYLE): **Classic** (the original look: theme-colored outlines, LED glow, transport on every card), **Clean** (neutral grays, flat surfaces, theme color only on what is playing, no LED) and **Spotify + LED** (Clean with the LED glow and light runner on). The choice is saved as `Style=` in config.ini.
- Square, CD or compact vertical layout; grid or list; themes; LED glow, particles and glitch effects (with a light mode for slower PCs); 8-band equalizer.

**Desktop integration**
- Keeps playing in the background when the window is closed (optional).
- Windows: tray icon and media keys. Linux: MPRIS (desktop media applet, media keys, `playerctl`).
- Configurable hotkeys, each one FOCUS (only while Remix is active) or GLOBAL; `remix --cmd next` for desktop shortcuts on Wayland.
- Single instance: opening a file hands it to the running player.

## Building from source

```bash
git clone https://github.com/EchoGroupStudio/Remix.git
cd Remix
```

### Linux

Nothing is installed system-wide: dependencies are downloaded into `third_party/`.

```bash
bash linux/fetch-deps.sh                 # raylib 5.5 (+ GLFW patch), zig 0.13, X11/Wayland headers
bash linux/build.sh                      # -> build/remix and linux/remix
bash linux/RODAR.sh                      # builds if needed, then runs
bash linux/packaging/build-packages.sh   # .deb, .rpm, portable zip and AppImage in dist/
bash linux/build-windows.sh              # cross-compiles windows/Remix.exe with MinGW-w64
```

### Windows

Double-click `windows\COMPILAR.bat`. It uses MinGW-w64 GCC (MSYS2 UCRT64, WinLibs, Scoop or Chocolatey)
and explains how to install it when missing. Manual commands: [windows/LEIA-ME.txt](windows/LEIA-ME.txt).

## Host: your PC library on the phone (iPhone and Android)

Sodre's idea to bring Remix to the **iPhone (iOS)** — and any phone — with no app store, while the native Android version
is in progress: the PC app becomes a **server** and the phone uses it from the browser ("Add to Home Screen" works).

- **Link a phone:** Settings > HOST (or the **HOST** button in the header) > turn it on. On the phone, **scan the QR code**
  and type a name — done. Without the QR, open the link, enter the **PIN** and accept the device on the PC. QR codes are
  single-use and expire in 10 minutes (optionally also require approval on the PC).
- **You decide what each device gets:** a newly linked device sees **nothing**. Share the whole **library** per device,
  host playlists to all or some devices, and approve playlists a phone asks to share. Each phone keeps its **own playlists**,
  isolated from other devices.
- **A real music-app UI on the phone:** Home, Search, Your Library, playlist pages with back navigation, full-screen
  **Now Playing** and lock-screen controls, **effects and stems** applied by the PC (the phone just plays the result, so
  the iPhone lock screen keeps working) and a **wave that follows the beat** computed by the PC. Tuned for **iPhone Safari**: every button gives touch feedback, the seek bar
  works by tapping or dragging anywhere on it, sheets open the keyboard and stay above it, playback recovers after an
  error, and the WhatsApp connect page works even in the iPhone preview (no JavaScript needed).
- **Online on the phone:** search and play YouTube Music, YouTube and SoundCloud from the phone — the **PC** runs yt-dlp and
  ffmpeg and sends only the audio; hosted playlists with online tracks stream too. The phone never talks to those sites.
- **LAN and internet:** same router (Wi-Fi or cable) or a **Cloudflare tunnel** (HTTPS, no port forwarding, CGNAT-friendly,
  your IP stays hidden). The tunnel link is **shown only after it is verified** (avoids DNS `NXDOMAIN` caching) and has a
  **COPY LINK** button.
- **Security:** only local or tunnel connections are accepted (even on a public IP), per-device/per-track authorization
  (revoking cuts playback immediately), devices linked without "Remember" are temporary, PIN/QR brute-force lockout (per
  address and global), CSRF/XSS and DNS-rebinding protection, slow-request and DoS limits, optional LAN-only IPv6.
  Default port **49875**.
  Details, security model and API in [docs/HOST.md](docs/HOST.md) (Portuguese).

## File safety (planned)

A plan to protect users from malicious or corrupted music files — play only the decodable audio and the cover, sanitized in an isolated process, without breaking the app — is in [docs/SEGURANCA-DE-ARQUIVOS.md](docs/SEGURANCA-DE-ARQUIVOS.md) (Portuguese). Not implemented yet.

## Android (work in progress)

The plan for the Android port (reusing the raylib shell, pinned toolchain under `third_party/android`, APK built without Gradle) is in [docs/ANDROID.md](docs/ANDROID.md) (Portuguese); `docs/android-exemplo.zip` holds the example `android/` folder.

## Project layout

| Folder | Contents |
|---|---|
| `comum/` | shared core: player (miniaudio), library, playlists, layout, input, online streaming and downloads |
| `linux/` | Linux shell (raylib), MPRIS, X11 hotkeys, build and packaging scripts |
| `windows/` | Windows shell (Win32 + GDI+), icon/manifest resources, `COMPILAR.bat` |
| `assets/` | branding, fonts and themes |
| `docs/` | extra documentation (Portuguese) and screenshots |

Run from the project folder (or a portable zip), Remix keeps `config.ini`, `covers.ini`, `artists.ini` and
`playlists/` next to it. Installed packages and the AppImage use `~/.config/remix`. Caches live in
`~/.cache/remix` (Linux) or `%TEMP%\remix-cache` (Windows).

## Command line

```text
remix [file]                 play a file (reuses the running instance)
remix --cmd <action>         playpause, next, prev, volup, voldown, mute, shuffle, repeat, show, hide, quit
remix --home <folder>        use another folder for settings and library (portable mode)
```

## License

Remix Player is released under the [Apache License 2.0](LICENSE). Bundled third-party code keeps its own
license: raylib and GLFW (zlib), miniaudio and stb (public domain / MIT), the QR encoder derived from Project Nayuki's
QR Code generator (MIT), DejaVu fonts (Bitstream Vera) and
Droid Sans Japanese (Apache 2.0). See [linux/packaging/copyright](linux/packaging/copyright). yt-dlp and ffmpeg
are separate programs and are not distributed with Remix, and neither is cloudflared (Apache 2.0, used by Host).

Please respect each platform's terms of service and the copyright law of your country: download only what
you have the right to.
