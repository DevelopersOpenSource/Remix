# Remix Player — pacote RPM (Fedora / Nobara / RHEL-like)
#
# Empacota a arvore ja instalada gerada por packaging/build-packages.sh
# (binario compilado por linux/build.sh + assets). O binario embute a libc++ e
# so depende de glibc (>= 2.27 quando compilado com o zig) e libm; X11/Wayland/
# OpenGL, libcurl e PulseAudio/ALSA sao carregados em tempo de execucao (dlopen).
%global debug_package %{nil}
%global _build_id_links none
%global __brp_check_rpaths %{nil}

Name:           remix
Version:        %{?remix_version}%{!?remix_version:1.5.0}
Release:        %{?remix_release}%{!?remix_release:1}%{?dist}
Summary:        Remix Player — player de música MP3/WAV com visual neon
License:        Apache-2.0 AND zlib AND MIT AND Bitstream-Vera
Source0:        remix-stage.tar.gz
BuildArch:      x86_64
ExclusiveArch:  x86_64

Requires:       hicolor-icon-theme
Recommends:     libcurl
Recommends:     (zenity or kdialog)
Recommends:     pipewire-pulseaudio
Recommends:     yt-dlp
Recommends:     (ffmpeg or ffmpeg-free)
Recommends:     (deno or nodejs)

%description
Player de música para desktop com biblioteca por pasta (ou varredura
automática), capas personalizadas (arquivo local ou busca na internet),
temas, efeitos visuais (partículas, LED, glitch, linha corredora), onda de
áudio real e espectro por frequência. Modos quadrado, CD e vertical.
Porte Linux (raylib + miniaudio) da versão Windows do Remix.

Configuração e capas ficam em ~/.config/remix (ou ao lado do binário, se
existir um config.ini lá — modo portátil).

%prep
%setup -q -c -T
tar -xzf %{SOURCE0}

%build
# nada: binario ja compilado por linux/build.sh

%install
mkdir -p %{buildroot}
cp -a usr %{buildroot}/

%files
%{_bindir}/remix
%{_datadir}/remix/
%{_datadir}/applications/remix.desktop
%{_datadir}/icons/hicolor/*/apps/remix.png
%doc %{_docdir}/%{name}/

%changelog
* Tue Sep 16 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.5.0-1
- Host: corrige o conflito de botoes com o estilo, QR code para vincular, cada aparelho so ve o que o PC liberar, online no celular, link do tunel testado antes de aparecer e botao de copiar

* Tue Sep 16 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.4.0-1
- Host: o Remix vira servidor para o celular (PIN + aceite no PC, tunel Cloudflare, rede local, playlists por aparelho)

* Tue Sep 16 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.3.1-1
- Configuracoes afinadas nos estilos novos: chaves liga/desliga, secoes na medida, textos sem vazar, versao no titulo

* Tue Sep 16 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.3.0-1
- Estilo da interface: Classico (original), Limpo e Spotify + LED, escolhido nas configuracoes

* Mon Sep 15 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.2.1-1
- Onda e espectro corretos na musica online (streaming)
- Pacotes montados sem CRLF; creditos no README
* Sat Sep 05 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.2.0-1
- Nucleo compartilhado Windows/Linux (mesma logica nos dois sistemas)
- Icone de volume com mudo (clique ou M), atalhos de teclado (espaco, setas, +/-, R, Del, F2)
- Menu PASTA no cabecalho (recentes + escolher), menu de contexto (renomear/apagar/abrir pasta)
- Modo leve, varredura padrao so em Musica/Downloads/Documentos/Area de trabalho
- Botoes fechar/minimizar no cabecalho (Windows)

* Sat Sep 05 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.1.0-1
- Autoplay, ordem da playlist (manual salva), equalizador 8 bandas, FLAC/OGG nativos e
  m4a/aac/opus/wma via ffmpeg, engine na taxa nativa do arquivo, tamanho de janela salvo,
  correcoes: play que virava seek, rolagem ao trocar de modo
* Fri Sep 04 2026 EchoGroupStudio <103298328+NinjaZinS2@users.noreply.github.com> - 1.0.0-1
- Primeira versão Linux (raylib + miniaudio), mesma interface da versão Windows
