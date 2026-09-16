# Host — ouvir as músicas do PC no celular

Desde a 1.4.0 o Remix do PC pode virar um **servidor** para o celular: o telefone
abre um site, digita um PIN, o PC aceita o aparelho na tela e pronto — dá para ouvir
a biblioteca do PC, as playlists que o PC "hosteou" e ter playlists próprias no
celular. Funciona pela **rede local** (mesmo roteador, Wi-Fi ou cabo) e pela
**internet** através de um túnel Cloudflare, sem abrir porta no roteador e sem
entregar o IP ou a localização de ninguém.

## Como usar (PC)

1. **Configurações > HOST** (ou o botão **HOST** no cabeçalho, que abre o painel):
   - **PIN**: de 4 a 12 números. Obrigatório. É o que o celular digita na primeira vez.
   - **Porta**: padrão **49875** (portas TCP vão só até 65535; essa fica fora do que os
     serviços comuns usam). Troque se quiser.
   - **Nome**: como o PC aparece no celular (padrão: nome do computador).
   - **Túnel Cloudflare**: ligado por padrão. Precisa do `cloudflared` (o instalador de
     dependências baixa em `assets/tools`, como o yt-dlp).
   - **Rede local**: ligado por padrão. Desligue para aceitar só pelo túnel.
2. **LIGAR**. O painel mostra o link da rede local (`http://192.168.x.y:49875`) e, quando o
   túnel sobe, o link `https://….trycloudflare.com`.
3. Mande o link para o celular — ou clique **HTML P/ WHATSAPP**: o Remix grava
   `Remix-conectar.html` na pasta do app; mande esse arquivo pelo WhatsApp. Abrindo no
   celular, ele procura o PC na rede local sozinho e, se não achar, oferece o link do túnel.
   O arquivo só tem links: **nenhum PIN, nenhum IP público**.
4. No celular: abra o link, digite um nome para o aparelho e o PIN. No PC aparece
   **"Novo dispositivo quer se conectar"** — ACEITAR ou RECUSAR. Aceitou, o celular entra.
5. **Playlists**: no PC, clique com o botão direito numa playlist > **Hostear no celular**
   (ou no painel HOST, HOST: NÃO/TODOS/ALGUNS). O padrão ao hostear é **todos** os
   aparelhos; no painel dá para escolher quais. A biblioteca inteira do PC sempre aparece
   para quem está pareado.
6. O celular cria as playlists dele na aba **MINHAS**. Elas ficam guardadas no PC
   (`host.ini`, e o painel HOST lista, só leitura), mas não entram nas playlists do PC.
7. **Remover** um aparelho no painel revoga o acesso na hora (ele precisa parear de novo).

Para "instalar" no celular como app: no Chrome/Safari, menu > **Adicionar à tela inicial**
(a página é uma PWA). Um app Android que é só um *wrapper* desse site também serve —
ver `docs/ANDROID.md`.

## O que o celular vê e faz

- Biblioteca do PC (nome, artista, duração, capa) e as playlists hosteadas para ele.
- Toca as músicas **no próprio celular** (o PC só serve o arquivo; áudio do PC não muda).
  Formatos que o navegador toca: mp3, m4a/aac, ogg/opus, flac, wav. Músicas *online*
  (streaming do YouTube etc.) não são servidas — só arquivos locais.
- Playlists próprias (criar, renomear, apagar, adicionar/remover faixas).
- Controles na tela de bloqueio (Media Session): tocar/pausar/próxima.

## Segurança — o que foi feito

| Risco | Como é tratado |
|---|---|
| Alguém adivinhar o PIN | 5 erros seguidos travam o IP por 60 s, dobrando a cada erro (até 16 min). PIN comparado em tempo constante. E mesmo com o PIN certo **o PC tem que aceitar** o aparelho. |
| Token roubado | Token aleatório de 256 bits (CSPRNG do sistema) em cookie `HttpOnly; SameSite=Strict` (`Secure` pelo túnel). Revogável no painel. Fica em `host.ini` no PC (não compartilhe o arquivo). |
| CSRF | Todo POST exige o cabeçalho `X-Remix: 1` (um site de fora não consegue mandá-lo) + cookie SameSite=Strict. |
| XSS | A página nunca usa `innerHTML` com dados; tudo entra por `textContent`. `Content-Security-Policy: script-src 'self'` (sem script inline), `X-Frame-Options: DENY`, `nosniff`. O JSON ainda escapa `< > &`. |
| SQL injection | Não existe SQL: os dados ficam em `host.ini` (texto) e os campos são escapados. Nomes vindos do celular são limpos (sem caracteres de controle, tamanho máximo). |
| Path traversal / vazamento de caminhos | O celular só recebe **ids opacos** (hash do caminho). Nenhum caminho de arquivo sai do PC nem entra vindo do celular; `..` na URL é rejeitado; só arquivos que estão no mapa da biblioteca são servidos. |
| DoS / DDoS | Limite de 60 pedidos por 10 s por IP (429), no máximo 24 conexões simultâneas (503), cabeçalho até 16 KB, corpo até 64 KB (413), timeouts de leitura/escrita, sem keep-alive. Pelo túnel, a Cloudflare absorve ataque volumétrico e o PC só recebe conexão de saída. |
| Privacidade | Túnel: o celular fala com `*.trycloudflare.com`; o PC abre uma conexão de saída para a Cloudflare — nada de IP público, porta aberta, IPv6 ou localização. O HTML de conectar só carrega links. Só IPv4. |
| CGNAT | O túnel é conexão de saída do PC, então funciona atrás de CGNAT/roteador sem redirecionar porta. |

**Criptografia:** pelo túnel a conexão é HTTPS de ponta a ponta (certificado válido da
Cloudflare; o trecho `cloudflared → 127.0.0.1` fica dentro do próprio PC). Na rede local
o acesso é HTTP direto — quem está no **mesmo roteador** consegue ver o tráfego. Se isso
importar, desligue "Rede local" e use só o túnel, mesmo em casa. (Um certificado
auto-assinado local dispararia aviso no celular; ficou de fora por isso.)

**O que ainda NÃO é:** controle remoto do player do PC (o celular toca localmente), e
músicas online (yt-dlp) pelo celular. Ambos são possíveis por cima desta base.

## Como funciona por dentro

- `comum/host_net.h` — sockets IPv4 (Winsock2 no Windows, POSIX no Linux), IPs da LAN
  (só cabo/Wi-Fi; docker/VM/VPN são pulados), nome do PC.
- `comum/host_server.h` — servidor HTTP/1.1 próprio (uma thread por conexão), rotas da
  API, pareamento, limites, `host.ini`, túnel (`cloudflared tunnel --url http://127.0.0.1:PORTA`,
  a URL é lida do log), página `Remix-conectar.html`, e o estado do painel (`host::PU()`).
  As threads do servidor só leem uma **cópia** da biblioteca (`host::Publish`, refeita
  pela UI quando algo muda); nunca tocam `g_tracks`/`g_playlists`.
- `comum/host_web.h` — a página do celular (HTML/CSS/JS embutidos), manifesto PWA,
  service worker e o modelo do HTML de conectar. Cor do tema entra em `@ACCENT@`.
- Núcleo: `Config` ganha `[Host]` (`HostOn`, `HostPort`, `HostPin`, `HostTunnel`,
  `HostLan`, `HostName`), eventos `EV_HOST_PEDIDO`/`EV_HOST_STATUS`, diálogo de
  confirmação `g_confirmKind==2`, editor de texto modos 6/7/8 (porta/PIN/nome),
  seção HOST nas configurações, pílula HOST no cabeçalho, painel HOST (layout em
  `LayoutHostPanel`, desenho em `DrawHostPanel` nas duas cascas), item "Hostear no
  celular" no menu da playlist.

### API (JSON)

| Rota | Auth | O que faz |
|---|---|---|
| `GET /api/ping` | não | `{app, v, nome}` — usado pelo HTML de conectar para achar o PC na LAN (CORS só aqui, só GET) |
| `POST /api/parear` `{pin, nome}` | não | cria um pedido; o PC recebe `EV_HOST_PEDIDO` |
| `GET /api/parear/estado?req=` | não | `pendente` / `aceito` (manda o cookie) / `recusado` / `expirado` (3 min) |
| `GET /api/estado` | cookie | nome do PC, do aparelho, versão, nº de faixas |
| `GET /api/biblioteca` | cookie | `{faixas:[{id,t,a,d,c}]}` |
| `GET /api/playlists` | cookie | `{pc:[…hosteadas para este aparelho], minhas:[…]}` |
| `POST /api/minhas` `{acao: criar\|renomear\|apagar\|add\|remover, slug, nome, id}` | cookie + `X-Remix` | playlists do aparelho (máx. 50, 5000 faixas cada) |
| `GET /api/faixa/<id>` | cookie | o arquivo, com `Range` (206) para o `<audio>` avançar/voltar |
| `GET /api/capa/<id>` | cookie | a capa (jpg/png) |
| `POST /api/sair` | cookie + `X-Remix` | o aparelho se desconecta (token revogado) |

### Testes que rodam sem celular

```bash
# Linux (Xephyr): liga o host com PIN 2468 e aceita o pedido sozinho 7 s depois
build/remix --home <pasta> --no-splash --after 500:hostpin:2468 --after 800:host:on --after 7000:hostok --exit-after 30000 &
curl -s http://127.0.0.1:49875/api/ping
curl -s -X POST http://127.0.0.1:49875/api/parear -H 'X-Remix: 1' -H 'Content-Type: application/json' -d '{"pin":"2468","nome":"teste"}'
```

Ações de teste: `hostpin:<pin>`, `hostport:<n>`, `host:on`, `host:off`, `hostok`, `hostno`,
`hostpanel`, `hosthtml`, `tunnel:on`, `tunnel:off`.

## Túnel Cloudflare

O Remix usa o **quick tunnel** do `cloudflared` (`tunnel --url …`): não precisa de conta,
domínio nem configuração; a URL `https://<palavras>.trycloudflare.com` muda a cada vez
que o túnel sobe (por isso o HTML de conectar é gerado de novo quando você quiser mandar
o link atual). Quem quiser um endereço fixo pode criar um túnel nomeado na conta
Cloudflare e apontar para `http://127.0.0.1:<porta>`; o app continua igual.

Onde o `cloudflared` fica: `assets/tools/cloudflared(.exe)` ao lado do app (é o que os
instaladores de dependências baixam), ou no PATH / `~/.local/bin`.
