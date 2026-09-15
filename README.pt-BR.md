# Remix Player

Player C++ com nucleo compartilhado entre Windows (WinAPI + GDI+) e Linux (raylib), audio
miniaudio nos dois sistemas.

English version: [README.md](README.md). Programa pronto para baixar: [Releases](https://github.com/EchoGroupStudio/Remix/releases).

## Créditos

<p align="center">
  <a href="https://github.com/Nero-2077"><img src="https://raw.githubusercontent.com/EchoGroupStudio/Remix/main/docs/creditos/nero-2077.pt-BR.svg" alt="Nero-2077: autor do Remix, ideia original e versão Windows" width="48%"></a>
  <a href="https://github.com/NinjaZinS2"><img src="https://raw.githubusercontent.com/EchoGroupStudio/Remix/main/docs/creditos/sodre.pt-BR.svg" alt="Sodre (NinjaZinS2): co-desenvolvedor, playlists, streaming e versão Linux" width="48%"></a>
</p>

**[Nero-2077](https://github.com/Nero-2077)** criou o Remix: a ideia original e a versão Windows.
**Sodre** ([NinjaZinS2](https://github.com/NinjaZinS2)) entrou depois como co-desenvolvedor: participou da versão Windows,
sugeriu e desenvolveu as playlists e a música online (streaming e downloads) e levou o Remix para o Linux.

## Pastas

| Pasta | O que tem |
|---|---|
| `windows\` | versao Windows: `main.cpp`, `win_*.h`, icone/manifesto/versao (`app.rc`), **`COMPILAR.bat`** (clique 2x: compila com MinGW-w64), `RODAR.bat`, `BUILD-TUDO.ps1` |
| `linux/` | versao Linux: `main_linux.cpp` + headers, `RODAR.sh`, `build.sh`, `build-windows.sh`, `build-appimage.sh`, `packaging/` (deb/rpm), `patches/` (correcao do GLFW aplicada no build) |
| `comum/` | codigo igual nos dois: `app_core.h` (logica), `app_layout.h`, `app_input.h`, `app_keys.h` (atalhos), `app_playlists.h`, `config.h`, `playlist.h`, `player_ma.h` + `audio_backend.c` (miniaudio), `cover_art.h` (capas embutidas), `app_proc.h` (processos), `online_resolve.h` / `online_play.h` / `app_online_*.h` (musica online) |
| `assets/` | imagens, fontes e temas |
| raiz | `config.ini`, `covers.ini`, `artists.ini`, `playlists/`, `Musica/` = a "casa" do app (criados na primeira execucao, fora do git) quando roda de `windows\` ou `linux/` |
| `dist/` | pacotes gerados pelos scripts (zips portateis, .deb, .rpm, AppImage); os prontos ficam nas [Releases](https://github.com/EchoGroupStudio/Remix/releases) |
| `docs/`, `build/`, `third_party/` | documentacao extra e telas; saida de build e compiladores/bibliotecas baixados pelos scripts (esses dois ficam fora do git) |

Os zips de `dist/` continuam autossuficientes (exe/binario + assets + config na mesma pasta).

## Interface

- Modo **normal** redesenhado seguindo a referencia enviada: painel superior com Quadrado/CD e temas, seguido por cards de musica em grade.
- Modo **vertical** minimalista, focado somente na musica, com CD/capa menor, onda, seek, controles e engrenagem.
- O modo vertical pode voltar para **Quadrado** ou **CD** em Configuracoes.
- A janela continua redimensionavel pelas bordas/cantos.
- Escala geral, titulo, autor e vertical continuam independentes.
- Controle do LED permanece separado nas configuracoes.
- Cabecalho (1.1/1.2): Quadrado/CD, **AUTO** (autoplay), **ORDEM** da playlist, **PASTA**
  (trocar de pasta na hora, com pastas recentes), lista/grade, engrenagem e, no Windows,
  os botoes minimizar/fechar da janela sem borda.
- Volume com icone de alto-falante (clique = mudo) e porcentagem ao lado.
- Clique direito numa musica: tocar, trocar capa, renomear artista, **renomear o arquivo no
  disco**, abrir a pasta, **excluir (lixeira)** com confirmacao.
- Atalhos, com duas teclas para não disparar sem querer: Ctrl+Espaço (tocar/pausar), Ctrl+←/→
  (anterior/próxima), Ctrl+↑/↓ (volume), Ctrl+M (mudo), Ctrl+S (aleatório), Ctrl+R (repetir),
  Ctrl+0 (reiniciar a faixa), Ctrl+Del (excluir), Alt+↑/↓ (mover na ordem manual), Ctrl+F (buscar),
  F2 (renomear arquivo), Esc (fecha menus). Todos mudam em Configuracoes > ATALHOS; quem nunca
  mexeu nos atalhos recebe esse padrão novo sozinho.
- **MODO LEVE** em Configuracoes > EFEITOS: menos particulas/LED e 30 fps para PCs fracos.
- **Busca** (caixa acima da lista ou Ctrl+F): filtra por nome/artista/arquivo; Enter toca a primeira.
- **Playlists** (aba PLAYLISTS acima da lista): cada playlist e uma pasta em `playlists/`
  dentro da pasta de config, com um `playlist.json` que guarda so o caminho das musicas
  (nada e copiado nem apagado). Na playlist aberta, **+ ADICIONAR**: marcar musicas da
  biblioteca (clique nos cards e CONCLUIR), escolher arquivos, adicionar todas de uma pasta,
  **vincular uma pasta** (a playlist mostra sempre o conteudo atual dela), colar link ou buscar
  online. Com uma playlist aberta, o botao **PASTA DA PLAYLIST** do cabecalho mexe so nela (a
  biblioteca nao muda). **+ NOVA PLAYLIST**: vazia, de uma pasta, de um link ou da busca. No
  card, TOCAR / ALEATORIO, clique abre a lista, botao direito: pasta vinculada, sincronizar com
  o link, baixar as musicas online, modo online, renomear/excluir. Arquivo que mudou de pasta e
  reencontrado pelo nome e tamanho; se nao, o app avisa.
- **Atalhos configuraveis** (Configuracoes > ATALHOS): clique na tecla e pressione a nova
  combinacao; cada atalho pode ser FOCO (so com a janela ativa, padrao, nao atrapalha jogos)
  ou GLOBAL (funciona com outro programa na frente / em segundo plano; no Windows via
  RegisterHotKey). `Remix.exe --cmd next` manda um comando para a instancia aberta.
- **Aleatorio** e uma fila: a lista fica na sua ordem (inclusive manual); so a ordem de
  reproducao muda, cada faixa uma vez por ciclo.
- **Segundo plano**: fechar a janela com musica tocando esconde o player e ele continua
  tocando (Windows: icone na bandeja com menu; Linux: controles de midia do desktop).
  Abrir o Remix de novo traz a janela de volta; Ctrl+Q ou "SAIR DO REMIX" encerram.
  Teclas de midia do teclado funcionam. Tudo ajustavel em Configuracoes > REPRODUCAO
  (vem ligado; pode desligar).

## Seek e onda

- A onda usa a analise real do audio (decodificacao com miniaudio em thread, nos dois sistemas).
- O progresso possui knob visivel e e arrastavel.
- No modo normal, cada card tem sua propria barra de seek clicavel/arrastavel.
- No modo vertical, a barra principal e clicavel/arrastavel.

## Capas personalizadas

Cada musica possui um pequeno botao de foto/camera no card e no modo vertical.
Ao clicar:

1. abre o seletor de imagens;
2. voce escolhe JPG/PNG/BMP;
3. o programa **redimensiona** a imagem para ate 512 px e grava em `assets/covers/` (JPG q88 se opaca, PNG se tiver transparencia);
4. o player passa a carregar a copia interna;
5. o mapeamento fica salvo em `covers.ini`.

Assim, a capa personalizada nao depende do arquivo original continuar no mesmo local — e uma foto de celular de 4 MB vira ~100 KB em disco, sem diferenca visivel (a maior exibicao do player e ~300 px).

### Migracao automatica

Na inicializacao, capas antigas maiores que 512 px sao reencodadas no lugar
(mesmo nome/extensao, entao `covers.ini` continua valido). O original fica de
backup em `assets/covers/_originais/` — apague essa pasta quando quiser
liberar o espaco.

## Biblioteca padrao

Quando `MusicFolder=` fica vazio no `config.ini`, o player entra no modo **PADRAO** e procura
MP3/WAV/FLAC/OGG (e os formatos extras, se houver ffmpeg) nas **pastas do usuario**: Musicas,
Downloads, Documentos, Area de trabalho e pendrives/discos removiveis. Ele nao entra em pastas
de jogos e programas (Steam, AppData, node_modules, .git...), entao a varredura e rapida e nao
enche a lista de efeitos sonoros de jogo. Essa busca roda em segundo plano para nao travar a interface.

Em Configuracoes e possivel escolher uma pasta especifica. Ao fazer isso, o modo deixa de ser
padrao. O botao **PADRAO (PASTAS DO USUARIO)** restaura a busca automatica. O botao **PASTA**
do cabecalho faz a mesma troca sem abrir as configuracoes (pastas recentes, escolher outra
ou voltar ao padrao), e a pasta escolhida e monitorada: arquivos novos/removidos aparecem sozinhos.

## Musica online (streaming e download)

Aba **ONLINE** (acima da lista) ou, numa playlist, **+ ADICIONAR > Buscar online / Colar link**.
Ctrl+V com um link em qualquer lugar do app tambem abre a busca.

- **Busca** por nome no YouTube Music, YouTube ou SoundCloud (animacao de carregando enquanto o
  yt-dlp procura; os resultados aparecem conforme chegam). Cada resultado tem ▶ (tocar), ↓ (baixar)
  e + (colocar numa playlist); "ADICIONAR TODAS" salva a lista inteira.
- **Links** de musica, album ou playlist: YouTube / YouTube Music e SoundCloud tocam direto;
  **Spotify** (pagina publica "embed": sem conta, sem chave, ~1 s; o spotdl fica so de reserva,
  porque a API oficial do Spotify ficou restrita e lenta), **Deezer** (API publica) e **Apple Music**
  (iTunes lookup) viram titulo + artista + duracao, e cada musica e procurada no YouTube Music na hora
  de tocar (o link tocavel achado fica salvo na playlist). Nao ha quebra de DRM: o audio vem sempre
  do YouTube/SoundCloud.
- **Streaming so na memoria**: o yt-dlp acha o endereco do audio e o ffmpeg decodifica para PCM
  direto na RAM (no maximo 6 min decodificados a frente e ~10 min no total; o que ja tocou e
  descartado). Fechar o app no meio nao deixa arquivo nenhum. Avancar/voltar (seek) funciona.
- **Fila de streaming**: a musica atual e as **proximas 2** (na ordem da lista ou do aleatorio) ficam
  cada uma no seu canal (yt-dlp + ffmpeg + buffer na memoria). As da fila comecam em cascata (cada
  uma quando a anterior ja achou o audio), guardam ~75 s e esperam a vez com o ffmpeg parado; ao
  pular ou quando a musica acaba, a proxima sai **na hora** e o canal continua de onde parou. Na lista
  aparece TOCANDO / FILA: PRONTA / FILA: CARREGANDO com a barra do quanto ja carregou. Memoria: ~12 MB
  por musica da fila. Uma musica que falhou na fila e pulada sem esperar de novo.
- **Download**: fila em segundo plano (pilula no canto inferior direito; clique = abrir a pasta ou
  cancelar). O arquivo e montado numa pasta temporaria do cache e so vai para a pasta final
  (`<Musicas>/Remix Online/<playlist>/`, configuravel) quando termina. MP3, M4A ou formato original,
  com titulo/artista/capa. Terminou: a entrada da playlist passa a apontar para o arquivo.
- **Streaming ou download**: Configuracoes > ONLINE ("AO TOCAR: STREAMING / BAIXAR") vale para tudo;
  cada playlist pode ter o proprio modo (pilula "ONLINE: ..." na barra da playlist ou botao direito no
  card); botao direito numa musica online tem "Baixar". No modo baixar ela ja toca por streaming
  enquanto baixa.
- **Diario** (`remix/online/estado-XXXX.json` no cache: `~/.cache` no Linux, `%TEMP%\remix-cache` no
  Windows): gravado no maximo 1x por segundo e so quando algo muda, com o que esta tocando, como
  (URL direta ou pipe do yt-dlp), onde (memoria), minuto, % do buffer, cada canal da fila (tocando ou
  fila, estado, ate onde recebeu) e cada download
  (status, %, pasta temporaria, destino). Ao abrir, o app le o diario, apaga downloads que ficaram
  pela metade (sem mexer nos de outro Remix aberto) e lembra a ultima musica online: ela aparece
  selecionada e o streaming recomeca do inicio ao apertar play.

Precisa de **yt-dlp**, **ffmpeg** e de um "JavaScript runtime" que o YouTube passou a exigir (**Deno 2.3+**
ou **Node.js 22+**). As versoes portateis trazem um instalador:

- **Windows:** clique 2x em `INSTALAR-DEPENDENCIAS.bat` (na pasta do Remix). Ele baixa os arquivos oficiais do
  yt-dlp, do FFmpeg e do Deno direto para a pasta `assets\tools` **dentro do Remix**, com o `curl` e o `tar` que ja
  vem no Windows 10 1803+ e no 11 — nada e instalado no sistema (nem pip, nem winget, nem PATH). O Remix so carrega
  essas ferramentas da propria pasta `assets\tools`. Sem `curl`/`tar` (Windows 10 antigo, LTSC, PC de empresa) ele
  abre a Microsoft Store no "Instalador de Aplicativo" ou as paginas oficiais.
- **Linux:** `bash instalar-dependencias.sh` (na pasta portatil). Reconhece a distro e usa o gerenciador dela (apt no
  Debian/Ubuntu/Mint, dnf no Fedora/Nobara/RHEL, pacman no Arch/Manjaro/CachyOS, zypper no openSUSE, xbps no Void,
  eopkg no Solus). Quando o yt-dlp ou o JavaScript runtime da distro sao antigos, em sistema imutavel (Bazzite,
  Silverblue, SteamOS) ou sem sudo, baixa as versoes oficiais do yt-dlp, do Deno e do ffmpeg para `~/.local/bin` e
  `~/.deno/bin`. No NixOS mostra o comando do `nix`; `--mostrar` so mostra o que faria.

Depois e so abrir o Remix: ele acha os programas sozinho, sem reiniciar. Configuracoes > ONLINE mostra o que foi
encontrado. Rode o instalador de novo de vez em quando: o YouTube muda e o yt-dlp precisa estar em dia.

### Se o Remix fechar sozinho (Windows)

O Remix grava o `remix-log.txt` ao lado do `Remix.exe` (ou em `%LOCALAPPDATA%\Remix`) com cada etapa da abertura,
a versao do Windows, o dispositivo de som e o erro, e mostra um aviso em vez de sumir. A abertura seguinte entra em
**modo seguro** (sem som de abertura, efeitos, bandeja e atalhos globais; para forcar: `Remix.exe --seguro`). Mande
esse arquivo. Confira tambem se a pasta inteira foi extraida (nao abra o `Remix.exe` de dentro do `.zip`) e se o
antivirus nao colocou o exe em quarentena.

## Capas dos proprios arquivos

A capa gravada dentro da musica (MP3/ID3, M4A/MP4, FLAC, OGG/Opus, WAV/AIFF) aparece sozinha: e
extraida em segundo plano para `remix/art/` no cache (reduzida a 512 px). Prioridade: capa escolhida
por voce > capa do arquivo > imagem da pasta (`cover.jpg` etc.). M4A tambem mostra titulo e artista.

## Integracao futura com a IA

O executavel aceita:

```bat
Player.exe --play-file "D:\Musicas\Minha Musica.mp3"
```

Se o player ja estiver aberto, a chamada reutiliza a instancia existente. O `--play-file` so abre arquivos que ja existem no computador (musica online: aba ONLINE).

Flags de teste (Windows e Linux): `--home <pasta>` (config/biblioteca em outra pasta),
`--no-splash`, `--after <ms>:<acao>` (ex.: `--after 1500:vertical --after 3000:shot:C:\tela.png`)
e `--exit-after <ms>`.

## Arquivos gerados

- `config.ini` — modo, biblioteca, tema, volume, escalas, LED, autoplay, ordem, EQ, modo leve, pastas recentes.
- `covers.ini` — caminho interno das capas personalizadas.
- `artists.ini` — artistas editados a mao.
- `order.ini` — ordem manual da playlist.
- `assets/covers/` — copias das capas escolhidas pelo usuario.
- Conversoes do ffmpeg (formatos extras) viram WAV temporarios numa pasta de cache, apagados sozinhos depois de 72 h.

## Compilacao

Baixe o codigo com `git clone https://github.com/EchoGroupStudio/Remix.git`.

Windows: clique 2x em `windows\COMPILAR.bat`. Ele compila com o **MinGW-w64 (GCC)** do MSYS2
(ou WinLibs/scoop/choco). Se nao achar nenhum, explica como instalar e, se voce confirmar,
instala o MSYS2 + g++ pelo winget. Nada e baixado escondido e o script nao usa PowerShell.
O exe leva icone e informacoes de versao (`windows/app.rc`) e um manifesto (`windows/app.manifest`:
`asInvoker`, controles modernos, caminhos longos, Windows 7-11). O manifesto e compilado como
`default-manifest.o`: o GCC do MinGW ja linka um arquivo com esse nome sozinho, e o `-B<pasta windows, caminho absoluto>` faz ele
usar o nosso no lugar do padrao (sem isso o exe ficaria com dois manifestos). Os `.o` ja compilados
acompanham a pasta, para quem nao tiver o `windres`.

```text
cd windows
windres -O coff app.rc -o app_res.o
windres -O coff manifest.rc -o default-manifest.o
gcc -O2 -w -c -I../comum ../comum/audio_backend.c -o audio_backend.o
g++ -std=gnu++20 -O2 -w -municode -mwindows -static -s -B"$(cygpath -m "$PWD")/" -I../comum -I. main.cpp audio_backend.o app_res.o -o Remix.exe -lgdiplus -lshell32 -lcomdlg32 -lole32 -luuid -lwinmm -lwinhttp -ldwmapi
```

Para tocar M4A/AAC/Opus/WMA etc. no Windows, coloque um `ffmpeg.exe` na pasta `assets\tools` do Remix
(para baixar, rode o `INSTALAR-DEPENDENCIAS.bat`). Sem ele, MP3/WAV/FLAC/OGG continuam funcionando.

## Compilar tudo (Windows + Linux) com um script

- No Windows: `windows\BUILD-TUDO.ps1` (PowerShell) compila o `Remix.exe` com o MinGW-w64 e o
  binario Linux com o zig (pergunta antes de baixar o zig), mais os zips portateis.
- No Linux/WSL: `bash linux/build-all.sh` gera tudo, incluindo `.deb`, `.rpm`, AppImage e o `Remix.exe`
  (cross-compilado com MinGW-w64; `linux/fetch-mingw.sh` baixa o compilador sem root se faltar).

### Antivirus

O `Remix.exe` e compilado com MinGW-w64 (GCC), o toolchain padrao de programas open source no
Windows. As versoes antigas usavam o zig, cujos executaveis sao alvo frequente de falso
positivo ("Trojan:Win32/Wacatac!ml" e parecidos). O exe leva manifesto e informacoes de versao,
nao pede administrador e nao carrega DLL escondida (so as do sistema). O aviso azul do
SmartScreen ("aplicativo nao reconhecido") aparece para qualquer exe sem assinatura digital
baixado da internet; so some com certificado de assinatura de codigo. Falso positivo do
Defender: https://www.microsoft.com/wdsi/filesubmission

Novidades 1.1: autoplay (botao AUTO), ordem da playlist (inclusive manual, salva),
equalizador de 8 bandas, FLAC/OGG nativos e mais formatos via ffmpeg, audio sem
resampling, e correcoes (play que virava seek; rolagem ao trocar de modo). Os dois
bugs tambem foram corrigidos no `main.cpp` do Windows.

Novidades 1.2 (sugestoes dos testadores, nos dois sistemas): nucleo unico compartilhado
(`app_core.h`, `app_layout.h`, `app_input.h`), Windows tambem com miniaudio (FLAC/OGG nativos,
EQ, ffmpeg opcional), icone de volume com mudo, atalhos de teclado, botoes fechar/minimizar
no Windows, menu PASTA no cabecalho, clique direito com renomear arquivo/excluir/abrir pasta,
MODO LEVE, varredura padrao so nas pastas do usuario. Correcoes: nomes de arquivo com acento
nao abriam no Windows (fopen ANSI), travamento ao abrir a janela em sessao Wayland (GLFW),
menu PASTA e lapis do modo vertical desalinhados. Depois: modo segundo plano (fechar
continua tocando, bandeja/MPRIS/teclas de midia, ajustavel), cabecalho do vertical sem
sobreposicao em qualquer largura, e AppImage para Linux. Depois ainda: busca, playlists
(pasta + JSON com caminhos), atalhos configuraveis com escopo FOCO/GLOBAL, fila do
aleatorio sem mexer na ordem da lista, X das configuracoes apos rolar, LED no painel em
todos os modos, layout refeito no tamanho real da janela ao trocar de modo. Depois: musica
online (busca, links do Spotify/Deezer/Apple Music/YouTube/SoundCloud, streaming na memoria,
downloads com diario), playlists com + ADICIONAR / pasta vinculada / marcar musicas, capas
embutidas nos arquivos. Correcoes: clicar em certas faixas (a 11a, por exemplo) trocava a pasta
da biblioteca ou mexia em escala/LED/modo (IDs de clique repetidos); PASTA com uma playlist
aberta trocava a biblioteca inteira; aviso de lista vazia por cima da barra; trocar de pasta
durante uma varredura podia deixar a lista vazia; `--home` abre instancia separada.

## Linux (portátil, .deb, .rpm e AppImage)

A versao Linux vive em `linux/` (raylib) e compartilha com o Windows o nucleo inteiro que
esta em `comum/`. A casca Windows e `windows/` (GDI+/WinHTTP). Guia completo em
[linux/README-LINUX.md](linux/README-LINUX.md).

Rodando de `linux/` (ou `windows\`), o app usa a raiz do projeto como "casa" (`config.ini`,
`assets/`, `Musica/`). `bash linux/RODAR.sh` compila sozinho se o codigo mudou e abre.

```bash
linux/fetch-deps.sh            # raylib 5.5 (+ patch do GLFW) + zig + headers X11/Wayland, tudo em third_party/ (sem root)
linux/build.sh                 # -> build/remix e ./remix
linux/packaging/build-packages.sh   # -> dist/*.deb, dist/*.rpm, dist/*-portable.zip e dist/Remix-*.AppImage
linux/build-appimage.sh        # so o AppImage (um arquivo que roda em qualquer distro)
```

O binario exige so glibc >= 2.27 (Ubuntu 18.04+, Debian 10+, Fedora, etc.).
Instalado pelo pacote, usa `~/.config/remix/` para config/capas; com um
`config.ini` ao lado do binario ele roda em modo portatil.
