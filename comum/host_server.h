#pragma once
// Host: o Remix do PC vira um servidor para o celular (docs/HOST.md).
//
//   - Servidor HTTP/1.1 proprio (host_net.h), IPv4, uma thread por conexao,
//     limite de conexoes, timeouts, limite de tamanho e de taxa por IP.
//   - Pareamento: PIN (4-12 digitos) -> pedido -> o PC aceita na tela ->
//     token aleatorio em cookie HttpOnly/SameSite=Strict. Erros de PIN travam o
//     IP (5 erros = 60 s, dobrando).
//   - O celular so recebe ids opacos (hash do caminho): nenhum caminho de arquivo
//     sai do PC e nenhum caminho entra vindo do celular. Nada de SQL, nada de shell.
//   - As threads do servidor NUNCA tocam g_tracks/g_playlists: leem uma copia
//     (Publish) feita pela thread da UI. A UI chama as funcoes Host* daqui; o
//     servidor avisa a UI por eventos (EV_HOST_PEDIDO, EV_HOST_STATUS).
//   - Tunel Cloudflare (cloudflared, "quick tunnel"): HTTPS ate o celular sem abrir
//     porta, atravessa CGNAT e nao entrega IP nem localizacao. Na rede local o
//     acesso e HTTP direto (mesmo roteador).
#include "host_net.h"
#include "host_web.h"
#include "app_proc.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace host {

// ------------------------------------------------------------ utilidades --
inline long long NowSec() { return (long long)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
inline bool RandomBytes(unsigned char* b, size_t n) {
#ifdef _WIN32
    typedef BOOLEAN(WINAPI* Fn)(PVOID, ULONG);
    static Fn fn = nullptr;
    if (!fn) { HMODULE h = LoadLibraryW(L"advapi32.dll"); if (h) fn = (Fn)GetProcAddress(h, "SystemFunction036"); }
    if (fn && fn(b, (ULONG)n)) return true;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) { size_t got = 0; while (got < n) { ssize_t r = read(fd, b + got, n - got); if (r <= 0) break; got += (size_t)r; } close(fd); if (got == n) return true; }
#endif
    // ultimo recurso (nao deveria acontecer): mistura relogio + endereco
    unsigned long long x = (unsigned long long)std::chrono::steady_clock::now().time_since_epoch().count() ^ (unsigned long long)(uintptr_t)b;
    for (size_t i = 0; i < n; i++) { x ^= x << 13; x ^= x >> 7; x ^= x << 17; b[i] = (unsigned char)(x & 0xFF); }
    return false;
}
inline std::string RandomHex(size_t bytes) {
    std::vector<unsigned char> b(bytes); RandomBytes(b.data(), bytes);
    static const char* hx = "0123456789abcdef"; std::string s; s.reserve(bytes * 2);
    for (unsigned char c : b) { s.push_back(hx[c >> 4]); s.push_back(hx[c & 15]); }
    return s;
}
inline std::string IdFor(const std::wstring& path) {   // FNV-1a 64 do caminho em UTF-8 -> 16 hex
    std::string u = WideToUtf8(path);
    unsigned long long h = 1469598103934665603ULL;
    for (unsigned char c : u) { h ^= c; h *= 1099511628211ULL; }
    char b[24]; snprintf(b, sizeof b, "%016llx", h); return b;
}
inline bool IsId(const std::string& s) { if (s.size() != 16) return false; for (char c : s) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false; return true; }
inline bool IsHex(const std::string& s, size_t n) { if (s.size() != n) return false; for (char c : s) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false; return true; }
inline bool DigitsOnly(const std::string& s, size_t mn, size_t mx) { if (s.size() < mn || s.size() > mx) return false; for (char c : s) if (c < '0' || c > '9') return false; return true; }
inline bool ConstEq(const std::string& a, const std::string& b) { if (a.size() != b.size()) return false; unsigned d = 0; for (size_t i = 0; i < a.size(); i++) d |= (unsigned)(a[i] ^ b[i]); return d == 0; }
inline std::string JsonEsc(const std::string& s) {   // tambem escapa < > & (a pagina nunca usa innerHTML, mas custa nada)
    std::string o; o.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
        case '"': o += "\\\""; break; case '\\': o += "\\\\"; break; case '\n': o += "\\n"; break; case '\r': o += "\\r"; break; case '\t': o += "\\t"; break;
        case '<': o += "\\u003c"; break; case '>': o += "\\u003e"; break; case '&': o += "\\u0026"; break;
        default: if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; } else o.push_back((char)c);
        }
    }
    return o;
}
inline std::string JStrA(const std::string& utf8) { return "\"" + JsonEsc(utf8) + "\""; }
inline std::string JStr(const std::wstring& w) { return JStrA(WideToUtf8(w)); }
// Leitor de JSON "raso": {"chave":"valor","n":123,"b":true}. Devolve o valor como texto
// (string sem aspas e sem escapes basicos). Suficiente para os pedidos do celular.
inline std::string JGet(const std::string& j, const std::string& key) {
    std::string k = "\"" + key + "\"";
    size_t p = j.find(k); if (p == std::string::npos) return "";
    p = j.find(':', p + k.size()); if (p == std::string::npos) return "";
    p++; while (p < j.size() && (j[p] == ' ' || j[p] == '\t')) p++;
    if (p >= j.size()) return "";
    if (j[p] == '"') {
        std::string o; p++;
        while (p < j.size() && j[p] != '"') {
            if (j[p] == '\\' && p + 1 < j.size()) {
                char e = j[p + 1];
                if (e == 'u' && p + 5 < j.size()) {   // \uXXXX -> UTF-8
                    unsigned cp = (unsigned)strtoul(j.substr(p + 2, 4).c_str(), nullptr, 16);
                    if (cp < 0x80) o.push_back((char)cp); else if (cp < 0x800) { o.push_back((char)(0xC0 | (cp >> 6))); o.push_back((char)(0x80 | (cp & 0x3F))); }
                    else { o.push_back((char)(0xE0 | (cp >> 12))); o.push_back((char)(0x80 | ((cp >> 6) & 0x3F))); o.push_back((char)(0x80 | (cp & 0x3F))); }
                    p += 6; continue;
                }
                o.push_back(e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r' : e); p += 2; continue;
            }
            o.push_back(j[p++]);
        }
        return o;
    }
    size_t e = p; while (e < j.size() && j[e] != ',' && j[e] != '}' && j[e] != ' ') e++;
    return j.substr(p, e - p);
}
inline std::string UrlDecode(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size() && isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) { o.push_back((char)strtoul(s.substr(i + 1, 2).c_str(), nullptr, 16)); i += 2; }
        else if (s[i] == '+') o.push_back(' ');
        else o.push_back(s[i]);
    }
    return o;
}
// Nome vindo do celular: sem controle, sem excesso, aparado.
inline std::string CleanName(const std::string& in, size_t maxLen) {
    std::string o;
    for (unsigned char c : in) { if (c < 0x20 || c == 0x7F) continue; o.push_back((char)c); if (o.size() >= maxLen) break; }
    while (!o.empty() && o.back() == ' ') o.pop_back();
    size_t b = 0; while (b < o.size() && o[b] == ' ') b++;
    return o.substr(b);
}
inline std::string Esc(const std::string& s) { std::string o; for (char c : s) { if (c == '%') o += "%25"; else if (c == '|') o += "%7C"; else if (c == '\n') o += "%0A"; else if (c == '\r') continue; else o.push_back(c); } return o; }
inline std::string Unesc(const std::string& s) { return UrlDecode(s); }
inline std::string ContentTypeFor(const std::wstring& path) {
    std::wstring e = std::filesystem::path(path).extension().wstring(); for (auto& c : e) c = (wchar_t)towlower(c);
    if (e == L".mp3") return "audio/mpeg"; if (e == L".m4a" || e == L".mp4" || e == L".aac") return "audio/mp4";
    if (e == L".ogg" || e == L".oga" || e == L".opus") return "audio/ogg"; if (e == L".flac") return "audio/flac";
    if (e == L".wav") return "audio/wav"; if (e == L".wma") return "audio/x-ms-wma"; if (e == L".webm") return "audio/webm";
    if (e == L".jpg" || e == L".jpeg") return "image/jpeg"; if (e == L".png") return "image/png"; if (e == L".webp") return "image/webp";
    return "application/octet-stream";
}
inline std::string HtmlEsc(const std::string& s) { std::string o; for (char c : s) { if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;"; else if (c == '&') o += "&amp;"; else if (c == '"') o += "&quot;"; else o.push_back(c); } return o; }

// ------------------------------------------------------------ estado ------
struct HTrack { std::string id; std::wstring title, artist, path, coverPath; int dur = 0; };
struct HPlaylist { std::wstring slug, name; std::vector<std::string> ids; };
struct Device { std::string id, name, token, ip; long long created = 0, lastSeen = 0; };
struct PairReq { std::string id, name, ip; long long created = 0; int estado = 0; std::string token, devId; bool viaTunnel = false; };   // estado: 0 pendente, 1 aceito, 2 recusado
struct DevPlaylist { std::string dev, slug, name; std::vector<std::string> ids; };
struct RateInfo { long long winStart = 0; int count = 0; int fails = 0; long long lockUntil = 0; };
struct State {
    std::mutex m;                                   // protege tudo abaixo (menos os atomics)
    bool running = false; int port = 0; bool lanOk = true; std::string pin; std::wstring hostName; std::string accent = "#1e90ff";
    hsock_t ls = HSOCK_BAD; std::thread th; std::atomic<bool> stop{ false }; std::atomic<int> conns{ 0 };
    std::vector<HTrack> tracks; std::map<std::string, size_t> byId; std::vector<HPlaylist> pls;
    std::map<std::wstring, std::string> targets;    // slug -> "" (nao hosteada) | "ALL" | "id1,id2"
    std::vector<Device> devs; std::vector<PairReq> reqs; std::vector<DevPlaylist> dpls;
    std::map<std::string, RateInfo> rate;
    std::vector<std::string> lanUrls; std::string lastError;
    std::thread tunTh; std::atomic<bool> tunCancel{ false }; std::atomic<bool> tunRunning{ false }; std::string tunUrl, tunStatus = "desligado";
    std::atomic<unsigned> version{ 0 };             // sobe a cada mudanca (a UI olha para redesenhar)
};
inline State& St() { static State* s = new State(); return *s; }
inline void Bump() { St().version++; }

// ------------------------------------------------------------ host.ini ----
inline std::wstring IniPath() { return Config::Join(Config::BaseDir(), L"host.ini"); }
inline void SaveIni() {   // chamar com St().m travado
    State& s = St(); std::string o = "# Remix Host: dispositivos pareados e playlists hosteadas. Nao compartilhe (tem os tokens).\n[Dispositivos]\n";
    for (auto& d : s.devs) o += "dev=" + d.id + "|" + d.token + "|" + std::to_string(d.created) + "|" + Esc(d.name) + "|" + Esc(d.ip) + "\n";
    o += "[Playlists]\n";
    for (auto& kv : s.targets) if (!kv.second.empty()) o += Esc(WideToUtf8(kv.first)) + "=" + kv.second + "\n";
    for (auto& p : s.dpls) { o += "[Celular:" + p.dev + ":" + p.slug + "]\nnome=" + Esc(p.name) + "\nids="; for (size_t i = 0; i < p.ids.size(); i++) { if (i) o += ","; o += p.ids[i]; } o += "\n"; }
    std::wstring path = IniPath(), tmp = path + L".tmp";
#ifdef _WIN32
    FILE* f = _wfopen(tmp.c_str(), L"wb");
#else
    FILE* f = fopen(WideToUtf8(tmp).c_str(), "wb");
#endif
    if (!f) return;
    fwrite(o.data(), 1, o.size(), f); fclose(f);
    std::error_code ec; std::filesystem::remove(std::filesystem::path(path), ec); std::filesystem::rename(std::filesystem::path(tmp), std::filesystem::path(path), ec);
}
inline void LoadIni() {
    State& s = St(); std::lock_guard<std::mutex> lk(s.m);
    s.devs.clear(); s.targets.clear(); s.dpls.clear();
#ifdef _WIN32
    FILE* f = _wfopen(IniPath().c_str(), L"rb");
#else
    FILE* f = fopen(WideToUtf8(IniPath()).c_str(), "rb");
#endif
    if (!f) return;
    std::string all; char b[4096]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) all.append(b, n); fclose(f);
    std::string sect; DevPlaylist* cur = nullptr; size_t pos = 0;
    while (pos <= all.size()) {
        size_t e = all.find('\n', pos); std::string ln = all.substr(pos, e == std::string::npos ? std::string::npos : e - pos); pos = (e == std::string::npos) ? all.size() + 1 : e + 1;
        while (!ln.empty() && (ln.back() == '\r' || ln.back() == ' ')) ln.pop_back();
        if (ln.empty() || ln[0] == '#') continue;
        if (ln[0] == '[') {
            sect = ln.substr(1, ln.size() - 2); cur = nullptr;
            if (sect.rfind("Celular:", 0) == 0) { size_t c = sect.find(':', 8); if (c != std::string::npos) { DevPlaylist p; p.dev = sect.substr(8, c - 8); p.slug = sect.substr(c + 1); s.dpls.push_back(p); cur = &s.dpls.back(); } }
            continue;
        }
        size_t eq = ln.find('='); if (eq == std::string::npos) continue;
        std::string k = ln.substr(0, eq), v = ln.substr(eq + 1);
        if (sect == "Dispositivos" && k == "dev") {
            std::vector<std::string> f; size_t p2 = 0; while (true) { size_t c = v.find('|', p2); f.push_back(v.substr(p2, c == std::string::npos ? std::string::npos : c - p2)); if (c == std::string::npos) break; p2 = c + 1; }
            if (f.size() >= 4 && IsHex(f[0], 12) && IsHex(f[1], 64)) { Device d; d.id = f[0]; d.token = f[1]; d.created = atoll(f[2].c_str()); d.name = Unesc(f[3]); if (f.size() > 4) d.ip = Unesc(f[4]); s.devs.push_back(d); }
        } else if (sect == "Playlists") { s.targets[Utf8ToWide(Unesc(k))] = v; }
        else if (cur) { if (k == "nome") cur->name = Unesc(v); else if (k == "ids") { size_t p2 = 0; while (p2 <= v.size()) { size_t c = v.find(',', p2); std::string id = v.substr(p2, c == std::string::npos ? std::string::npos : c - p2); if (IsId(id)) cur->ids.push_back(id); if (c == std::string::npos) break; p2 = c + 1; } } }
    }
    // playlists de dispositivos que nao existem mais
    s.dpls.erase(std::remove_if(s.dpls.begin(), s.dpls.end(), [&](const DevPlaylist& p) { for (auto& d : s.devs) if (d.id == p.dev) return false; return true; }), s.dpls.end());
}

// ------------------------------------------------------------ snapshot ----
// Thread da UI: copia a biblioteca e as playlists do PC para o servidor.
inline void Publish(const std::vector<Track>& lib, const std::vector<Playlist>& pls) {
    std::vector<HTrack> tr; std::map<std::string, size_t> by;
    auto add = [&](const std::wstring& path, const std::wstring& title, const std::wstring& artist, const std::wstring& cover, int dur) {
        if (path.empty()) return std::string();
        std::string id = IdFor(path);
        if (by.count(id)) return id;
        HTrack t; t.id = id; t.path = path; t.title = title.empty() ? std::filesystem::path(path).stem().wstring() : title; t.artist = artist; t.coverPath = cover; t.dur = dur;
        by[id] = tr.size(); tr.push_back(t); return id;
    };
    for (auto& t : lib) if (!IsOnlineTrack(t)) add(t.path, t.title, t.artist, t.coverPath, t.durSec);
    std::vector<HPlaylist> hp;
    for (auto& p : pls) {
        HPlaylist h; h.slug = p.slug; h.name = p.name;
        for (auto& e : p.entries) { if (!e.url.empty() || e.path.empty()) continue; std::string id = add(e.path, e.title, e.artist, L"", e.dur); if (!id.empty()) h.ids.push_back(id); }
        hp.push_back(h);
    }
    State& s = St(); std::lock_guard<std::mutex> lk(s.m);
    s.tracks.swap(tr); s.byId.swap(by); s.pls.swap(hp);
}

// ------------------------------------------------------------ limites -----
inline bool RateOk(const std::string& ip) {   // com St().m travado. 60 pedidos / 10 s por IP
    State& s = St(); long long now = NowSec();
    if (s.rate.size() > 4000) s.rate.clear();
    RateInfo& r = s.rate[ip];
    if (now - r.winStart >= 10) { r.winStart = now; r.count = 0; }
    return ++r.count <= 60;
}
inline bool PairLocked(const std::string& ip) { State& s = St(); auto it = s.rate.find(ip); return it != s.rate.end() && NowSec() < it->second.lockUntil; }
inline void PairFail(const std::string& ip) { State& s = St(); RateInfo& r = s.rate[ip]; r.fails++; if (r.fails >= 5) { int k = r.fails - 5; if (k > 4) k = 4; r.lockUntil = NowSec() + 60LL * (1 << k); } }
inline void PairOk(const std::string& ip) { State& s = St(); s.rate[ip].fails = 0; s.rate[ip].lockUntil = 0; }
inline void ExpireReqs() {   // com St().m travado
    State& s = St(); long long now = NowSec();
    s.reqs.erase(std::remove_if(s.reqs.begin(), s.reqs.end(), [&](const PairReq& r) { return now - r.created > 180 && r.estado != 1; }), s.reqs.end());
    s.reqs.erase(std::remove_if(s.reqs.begin(), s.reqs.end(), [&](const PairReq& r) { return r.estado == 1 && now - r.created > 900; }), s.reqs.end());
}

// ------------------------------------------------------------ HTTP --------
struct Req { std::string method, path, query, body, ip, cookieDev; bool viaTunnel = false; std::map<std::string, std::string> h; long long r0 = -1, r1 = -1; bool hasRange = false; };
struct Resp { int status = 200; std::string ctype = "application/json; charset=utf-8"; std::string body; std::vector<std::string> extra; bool cache = false; };
inline const char* StatusText(int c) {
    switch (c) { case 200: return "OK"; case 204: return "No Content"; case 206: return "Partial Content"; case 400: return "Bad Request"; case 401: return "Unauthorized"; case 403: return "Forbidden";
    case 404: return "Not Found"; case 405: return "Method Not Allowed"; case 413: return "Payload Too Large"; case 416: return "Range Not Satisfiable"; case 429: return "Too Many Requests"; case 503: return "Service Unavailable"; default: return "Error"; }
}
inline std::string Head(const Resp& r, long long len) {
    std::string o = "HTTP/1.1 " + std::to_string(r.status) + " " + StatusText(r.status) + "\r\n";
    o += "Content-Type: " + r.ctype + "\r\nContent-Length: " + std::to_string(len) + "\r\n";
    o += "Connection: close\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer\r\nX-Frame-Options: DENY\r\n";
    o += r.cache ? "Cache-Control: public, max-age=86400\r\n" : "Cache-Control: no-store\r\n";
    if (r.ctype.rfind("text/html", 0) == 0) o += "Content-Security-Policy: default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; media-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'none'\r\n";
    for (auto& e : r.extra) o += e + "\r\n";
    return o + "\r\n";
}
inline void SendResp(hsock_t s, const Resp& r) { std::string h = Head(r, (long long)r.body.size()); hostnet::SendAll(s, h); if (!r.body.empty()) hostnet::SendAll(s, r.body); }
inline void SendJson(hsock_t s, int status, const std::string& json, const std::vector<std::string>& extra = {}) { Resp r; r.status = status; r.body = json; r.extra = extra; SendResp(s, r); }
inline void SendErr(hsock_t s, int status, const char* erro) { SendJson(s, status, std::string("{\"erro\":\"") + erro + "\"}"); }

// Le o pedido inteiro (cabecalho ate 16 KB, corpo ate 64 KB). false = pedido invalido (status em err).
inline bool ReadReq(hsock_t s, Req& r, int& err) {
    std::string buf; char b[4096]; err = 400;
    while (buf.find("\r\n\r\n") == std::string::npos) {
        if (buf.size() > 16384) { err = 431; return false; }
        long n = hostnet::Recv(s, b, sizeof b); if (n <= 0) return false; buf.append(b, (size_t)n);
    }
    size_t he = buf.find("\r\n\r\n"); std::string head = buf.substr(0, he); std::string rest = buf.substr(he + 4);
    size_t le = head.find("\r\n"); std::string line = head.substr(0, le == std::string::npos ? head.size() : le);
    size_t s1 = line.find(' '); if (s1 == std::string::npos) return false; size_t s2 = line.find(' ', s1 + 1); if (s2 == std::string::npos) return false;
    r.method = line.substr(0, s1); std::string target = line.substr(s1 + 1, s2 - s1 - 1);
    if (target.empty() || target[0] != '/' || target.size() > 2048) return false;
    size_t q = target.find('?'); r.path = target.substr(0, q); if (q != std::string::npos) r.query = target.substr(q + 1);
    if (r.path.find("..") != std::string::npos) return false;
    size_t p = (le == std::string::npos) ? head.size() : le + 2; int nh = 0;
    while (p < head.size()) {
        size_t e = head.find("\r\n", p); std::string hl = head.substr(p, e == std::string::npos ? std::string::npos : e - p); p = (e == std::string::npos) ? head.size() : e + 2;
        size_t c = hl.find(':'); if (c == std::string::npos) continue;
        std::string k = hl.substr(0, c), v = hl.substr(c + 1); for (auto& ch : k) ch = (char)tolower((unsigned char)ch);
        while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) v.erase(0, 1); while (!v.empty() && (v.back() == ' ' || v.back() == '\t')) v.pop_back();
        if (++nh > 64) { err = 431; return false; }
        r.h[k] = v;
    }
    long long cl = 0; auto it = r.h.find("content-length"); if (it != r.h.end()) cl = atoll(it->second.c_str());
    if (cl < 0 || cl > 65536) { err = 413; return false; }
    if (r.h.count("transfer-encoding")) { err = 413; return false; }
    r.body = rest;
    while ((long long)r.body.size() < cl) { long n = hostnet::Recv(s, b, sizeof b); if (n <= 0) return false; r.body.append(b, (size_t)n); }
    r.body.resize((size_t)cl);
    // Range: bytes=a-b | bytes=a-
    auto rg = r.h.find("range");
    if (rg != r.h.end() && rg->second.rfind("bytes=", 0) == 0) {
        std::string v = rg->second.substr(6); size_t d = v.find('-');
        if (d != std::string::npos && v.find(',') == std::string::npos) { r.hasRange = true; r.r0 = d ? atoll(v.substr(0, d).c_str()) : -1; r.r1 = (d + 1 < v.size()) ? atoll(v.substr(d + 1).c_str()) : -1; }
    }
    // cookie do dispositivo
    auto ck = r.h.find("cookie");
    if (ck != r.h.end()) { size_t p2 = ck->second.find("remix_dev="); if (p2 != std::string::npos) { size_t e = ck->second.find(';', p2); r.cookieDev = ck->second.substr(p2 + 10, e == std::string::npos ? std::string::npos : e - p2 - 10); } }
    // atras do tunel: o cloudflared chega de 127.0.0.1 e manda o IP real
    if (r.ip == "127.0.0.1") { auto cf = r.h.find("cf-connecting-ip"); if (cf != r.h.end() && !cf->second.empty()) { r.ip = cf->second; r.viaTunnel = true; } }
    auto xp = r.h.find("x-forwarded-proto"); if (xp != r.h.end() && xp->second == "https") r.viaTunnel = true;
    return true;
}
// Arquivo com Range (o <audio> do navegador pede pedacos para avancar/voltar).
inline void SendFile(hsock_t s, const Req& r, const std::wstring& path, const std::string& ctype, bool cache) {
    std::error_code ec; unsigned long long sz = std::filesystem::file_size(std::filesystem::path(path), ec);
    if (ec) { SendErr(s, 404, "arquivo"); return; }
#ifdef _WIN32
    FILE* f = _wfopen(path.c_str(), L"rb");
#else
    FILE* f = fopen(WideToUtf8(path).c_str(), "rb");
#endif
    if (!f) { SendErr(s, 404, "arquivo"); return; }
    long long a = 0, bnd = (long long)sz - 1; int status = 200;
    if (r.hasRange && sz > 0) {
        if (r.r0 < 0) { long long n = r.r1; if (n <= 0) n = 1; if (n > (long long)sz) n = (long long)sz; a = (long long)sz - n; }
        else { a = r.r0; if (r.r1 >= 0 && r.r1 < bnd) bnd = r.r1; }
        if (a > bnd || a >= (long long)sz) { fclose(f); Resp e; e.status = 416; e.body = "{}"; e.extra.push_back("Content-Range: bytes */" + std::to_string(sz)); SendResp(s, e); return; }
        status = 206;
    }
    Resp h; h.status = status; h.ctype = ctype; h.cache = cache; h.extra.push_back("Accept-Ranges: bytes");
    if (status == 206) h.extra.push_back("Content-Range: bytes " + std::to_string(a) + "-" + std::to_string(bnd) + "/" + std::to_string(sz));
    long long len = bnd - a + 1; if (sz == 0) len = 0;
    hostnet::SendAll(s, Head(h, len));
    if (r.method == "HEAD" || len == 0) { fclose(f); return; }
#ifdef _WIN32
    _fseeki64(f, a, SEEK_SET);
#else
    fseeko(f, (off_t)a, SEEK_SET);
#endif
    hostnet::SetTimeout(s, 30000);
    std::vector<char> buf(65536); long long left = len;
    while (left > 0) { size_t want = (size_t)std::min<long long>((long long)buf.size(), left); size_t n = fread(buf.data(), 1, want, f); if (!n) break; if (!hostnet::SendAll(s, buf.data(), n)) break; left -= (long long)n; }
    fclose(f);
}
inline std::string ReplaceAll(std::string s, const std::string& a, const std::string& b) { size_t p = 0; while ((p = s.find(a, p)) != std::string::npos) { s.replace(p, a.size(), b); p += b.size(); } return s; }
inline std::string DevSlug(const std::string& name) {
    std::string o; for (unsigned char c : name) { if (isalnum(c)) o.push_back((char)tolower(c)); else if (c == ' ' || c == '-' || c == '_') { if (!o.empty() && o.back() != '-') o.push_back('-'); } if (o.size() >= 24) break; }
    while (!o.empty() && o.back() == '-') o.pop_back(); if (o.empty()) o = "playlist";
    return o + "-" + RandomHex(3);
}

// Uma conexao = um pedido. Tudo que depende de estado roda com St().m travado (curto).
inline void Serve(hsock_t c, std::string peer) {
    State& s = St();
    Req r; r.ip = peer; int err = 400;
    if (!ReadReq(c, r, err)) { if (err != 400 || true) { Resp e; e.status = err; e.body = "{\"erro\":\"pedido\"}"; SendResp(c, e); } return; }
    { std::lock_guard<std::mutex> lk(s.m); if (!RateOk(r.ip)) { Resp e; e.status = 429; e.body = "{\"erro\":\"calma\"}"; e.extra.push_back("Retry-After: 10"); SendResp(c, e); return; } }
    const std::string& P = r.path;
    // ---- estaticos (sem login)
    if (r.method == "GET" || r.method == "HEAD") {
        Resp o; o.cache = true;
        if (P == "/" || P == "/index.html") { std::string acc; { std::lock_guard<std::mutex> lk(s.m); acc = s.accent; } o.cache = false; o.ctype = "text/html; charset=utf-8"; o.body = ReplaceAll(ReplaceAll(hostweb::INDEX_HTML, "@ACCENT@", acc), "@VER@", WideToUtf8(REMIX_VERSAO)); SendResp(c, o); return; }
        if (P == "/app.js") { o.ctype = "text/javascript; charset=utf-8"; o.body = hostweb::APP_JS; SendResp(c, o); return; }
        if (P == "/app.css") { o.ctype = "text/css; charset=utf-8"; o.body = hostweb::APP_CSS; SendResp(c, o); return; }
        if (P == "/manifest.webmanifest") { o.ctype = "application/manifest+json"; o.body = hostweb::MANIFEST_JSON; SendResp(c, o); return; }
        if (P == "/sw.js") { o.cache = false; o.ctype = "text/javascript; charset=utf-8"; o.body = hostweb::SW_JS; SendResp(c, o); return; }
        if (P == "/icon.png") { SendFile(c, r, Config::Join(Config::Join(Config::AssetDir(), L"branding"), L"icon.png"), "image/png", true); return; }
    }
    if (P == "/api/ping") {   // tambem usado pela pagina "conectar" para achar o PC na LAN (CORS so aqui, so GET)
        std::vector<std::string> cors = { "Access-Control-Allow-Origin: *", "Access-Control-Allow-Methods: GET", "Access-Control-Allow-Private-Network: true", "Access-Control-Max-Age: 600" };
        if (r.method == "OPTIONS") { Resp o; o.status = 204; o.extra = cors; SendResp(c, o); return; }
        std::wstring nm; { std::lock_guard<std::mutex> lk(s.m); nm = s.hostName; }
        SendJson(c, 200, "{\"app\":\"remix\",\"v\":" + JStr(REMIX_VERSAO) + ",\"nome\":" + JStr(nm) + "}", cors); return;
    }
    // ---- pareamento
    if (P == "/api/parear") {
        if (r.method != "POST") { SendErr(c, 405, "metodo"); return; }
        if (r.h["x-remix"] != "1") { SendErr(c, 403, "origem"); return; }
        std::string pin = JGet(r.body, "pin"), nome = CleanName(JGet(r.body, "nome"), 40); if (nome.empty()) nome = "Celular";
        std::lock_guard<std::mutex> lk(s.m);
        if (PairLocked(r.ip)) { SendJson(c, 429, "{\"erro\":\"travado\"}", { "Retry-After: 60" }); return; }
        if (!DigitsOnly(pin, 4, 12) || !ConstEq(pin, s.pin)) { PairFail(r.ip); SendErr(c, 401, "pin"); return; }
        PairOk(r.ip); ExpireReqs();
        int mine = 0; for (auto& q : s.reqs) if (q.ip == r.ip && q.estado == 0) mine++;
        if (mine >= 3 || s.reqs.size() >= 20) { SendJson(c, 429, "{\"erro\":\"pedidos\"}"); return; }
        PairReq q; q.id = RandomHex(8); q.name = nome; q.ip = r.ip; q.created = NowSec(); q.viaTunnel = r.viaTunnel; s.reqs.push_back(q); Bump();
        AppPost(EV_HOST_PEDIDO, Utf8ToWide(q.id), 0);
        SendJson(c, 200, "{\"req\":\"" + q.id + "\"}"); return;
    }
    if (P == "/api/parear/estado") {
        std::string id; { size_t p = r.query.find("req="); if (p != std::string::npos) { size_t e = r.query.find('&', p); id = UrlDecode(r.query.substr(p + 4, e == std::string::npos ? std::string::npos : e - p - 4)); } }
        if (!IsHex(id, 16)) { SendErr(c, 400, "req"); return; }
        std::lock_guard<std::mutex> lk(s.m); ExpireReqs();
        for (auto& q : s.reqs) if (q.id == id && q.ip == r.ip) {
            if (q.estado == 1) { std::string ck = "Set-Cookie: remix_dev=" + q.token + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=31536000" + (r.viaTunnel ? "; Secure" : ""); std::string tok = q.token; q.token.clear(); SendJson(c, 200, "{\"estado\":\"aceito\"}", { ck }); return; }
            SendJson(c, 200, std::string("{\"estado\":\"") + (q.estado == 2 ? "recusado" : "pendente") + "\"}"); return;
        }
        SendJson(c, 200, "{\"estado\":\"expirado\"}"); return;
    }
    // ---- daqui para baixo so dispositivo pareado
    Device dev; bool auth = false;
    { std::lock_guard<std::mutex> lk(s.m); if (IsHex(r.cookieDev, 64)) for (auto& d : s.devs) if (ConstEq(d.token, r.cookieDev)) { d.lastSeen = NowSec(); if (d.ip != r.ip) d.ip = r.ip; dev = d; auth = true; break; } }
    if (!auth) { SendErr(c, 401, "pareie"); return; }
    if (r.method == "POST" && r.h["x-remix"] != "1") { SendErr(c, 403, "origem"); return; }
    if (P == "/api/estado") {
        std::lock_guard<std::mutex> lk(s.m);
        SendJson(c, 200, "{\"host\":" + JStr(s.hostName) + ",\"dispositivo\":" + JStrA(dev.name) + ",\"id\":\"" + dev.id + "\",\"v\":" + JStr(REMIX_VERSAO) + ",\"faixas\":" + std::to_string(s.tracks.size()) + "}"); return;
    }
    if (P == "/api/biblioteca") {
        std::string o = "{\"faixas\":["; { std::lock_guard<std::mutex> lk(s.m); bool first = true; for (auto& t : s.tracks) { if (!first) o += ","; first = false; o += "{\"id\":\"" + t.id + "\",\"t\":" + JStr(t.title) + ",\"a\":" + JStr(t.artist) + ",\"d\":" + std::to_string(t.dur) + ",\"c\":" + (t.coverPath.empty() ? "0" : "1") + "}"; } }
        SendJson(c, 200, o + "]}"); return;
    }
    if (P == "/api/playlists") {
        std::string o = "{\"pc\":["; std::lock_guard<std::mutex> lk(s.m); bool first = true;
        for (auto& p : s.pls) {
            auto it = s.targets.find(p.slug); if (it == s.targets.end() || it->second.empty()) continue;
            if (it->second != "ALL" && ("," + it->second + ",").find("," + dev.id + ",") == std::string::npos) continue;
            if (!first) o += ","; first = false; o += "{\"slug\":" + JStr(p.slug) + ",\"nome\":" + JStr(p.name) + ",\"ids\":["; for (size_t i = 0; i < p.ids.size(); i++) { if (i) o += ","; o += "\"" + p.ids[i] + "\""; } o += "]}";
        }
        o += "],\"minhas\":["; first = true;
        for (auto& p : s.dpls) { if (p.dev != dev.id) continue; if (!first) o += ","; first = false; o += "{\"slug\":" + JStrA(p.slug) + ",\"nome\":" + JStrA(p.name) + ",\"ids\":["; for (size_t i = 0; i < p.ids.size(); i++) { if (i) o += ","; o += "\"" + p.ids[i] + "\""; } o += "]}"; }
        SendJson(c, 200, o + "]}"); return;
    }
    if (P == "/api/minhas") {
        if (r.method != "POST") { SendErr(c, 405, "metodo"); return; }
        std::string acao = JGet(r.body, "acao"), slug = JGet(r.body, "slug"), nome = CleanName(JGet(r.body, "nome"), 60), id = JGet(r.body, "id");
        std::lock_guard<std::mutex> lk(s.m);
        if (acao == "criar") {
            int n = 0; for (auto& p : s.dpls) if (p.dev == dev.id) n++;
            if (n >= 50) { SendErr(c, 400, "limite"); return; }
            if (nome.empty()) nome = "Playlist";
            DevPlaylist p; p.dev = dev.id; p.name = nome; p.slug = DevSlug(nome); s.dpls.push_back(p); SaveIni(); Bump();
            SendJson(c, 200, "{\"ok\":1,\"slug\":" + JStrA(p.slug) + "}"); return;
        }
        DevPlaylist* pl = nullptr; for (auto& p : s.dpls) if (p.dev == dev.id && p.slug == slug) { pl = &p; break; }
        if (!pl) { SendErr(c, 404, "playlist"); return; }
        if (acao == "renomear") { if (nome.empty()) { SendErr(c, 400, "nome"); return; } pl->name = nome; }
        else if (acao == "apagar") { s.dpls.erase(std::remove_if(s.dpls.begin(), s.dpls.end(), [&](const DevPlaylist& p) { return p.dev == dev.id && p.slug == slug; }), s.dpls.end()); }
        else if (acao == "add") { if (!IsId(id) || !s.byId.count(id)) { SendErr(c, 404, "faixa"); return; } if (pl->ids.size() >= 5000) { SendErr(c, 400, "limite"); return; } if (std::find(pl->ids.begin(), pl->ids.end(), id) == pl->ids.end()) pl->ids.push_back(id); }
        else if (acao == "remover") { pl->ids.erase(std::remove(pl->ids.begin(), pl->ids.end(), id), pl->ids.end()); }
        else { SendErr(c, 400, "acao"); return; }
        SaveIni(); Bump(); SendJson(c, 200, "{\"ok\":1}"); return;
    }
    if (P == "/api/sair") {
        if (r.method != "POST") { SendErr(c, 405, "metodo"); return; }
        { std::lock_guard<std::mutex> lk(s.m); s.devs.erase(std::remove_if(s.devs.begin(), s.devs.end(), [&](const Device& d) { return d.id == dev.id; }), s.devs.end()); s.dpls.erase(std::remove_if(s.dpls.begin(), s.dpls.end(), [&](const DevPlaylist& p) { return p.dev == dev.id; }), s.dpls.end()); SaveIni(); Bump(); }
        SendJson(c, 200, "{\"ok\":1}", { "Set-Cookie: remix_dev=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0" }); return;
    }
    if (P.rfind("/api/faixa/", 0) == 0 || P.rfind("/api/capa/", 0) == 0) {
        bool capa = P[5] == 'c'; std::string id = P.substr(capa ? 10 : 11);
        if (!IsId(id)) { SendErr(c, 404, "id"); return; }
        std::wstring path, cover; { std::lock_guard<std::mutex> lk(s.m); auto it = s.byId.find(id); if (it == s.byId.end()) { SendErr(c, 404, "faixa"); return; } path = s.tracks[it->second].path; cover = s.tracks[it->second].coverPath; }
        if (capa) { if (cover.empty()) { SendErr(c, 404, "capa"); return; } SendFile(c, r, cover, ContentTypeFor(cover), true); return; }
        SendFile(c, r, path, ContentTypeFor(path), false); return;
    }
    SendErr(c, 404, "rota");
}
inline void AcceptLoop() {
    State& s = St();
    while (!s.stop.load()) {
        std::string ip; hsock_t c = hostnet::Accept(s.ls, ip);
        if (c == HSOCK_BAD) { if (s.stop.load()) break; std::this_thread::sleep_for(std::chrono::milliseconds(50)); continue; }
        if (s.conns.load() >= 24) { hostnet::SendAll(c, "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\nRetry-After: 2\r\n\r\n"); hostnet::Close(c); continue; }
        s.conns++;
        try { std::thread([c, ip]() { Serve(c, ip); hostnet::Close(c); St().conns--; }).detach(); }
        catch (...) { hostnet::Close(c); s.conns--; }
    }
}

// ------------------------------------------------------------ tunel -------
inline std::wstring CloudflaredPath() {
    namespace fs = std::filesystem; std::error_code ec;
#ifdef _WIN32
    const wchar_t* exe = L"cloudflared.exe";
#else
    const wchar_t* exe = L"cloudflared";
#endif
    std::wstring t = Config::Join(Config::Join(Config::AssetDir(), L"tools"), exe);
    if (fs::exists(fs::path(t), ec)) return t;
#ifdef _WIN32
    wchar_t buf[MAX_PATH]; if (SearchPathW(NULL, L"cloudflared.exe", NULL, MAX_PATH, buf, NULL)) return buf;
#else
    const char* pathEnv = std::getenv("PATH"); std::string p = pathEnv ? pathEnv : ""; size_t pos = 0;
    while (pos <= p.size()) { size_t c = p.find(':', pos); std::string dir = p.substr(pos, c == std::string::npos ? std::string::npos : c - pos); if (!dir.empty()) { fs::path cand = fs::path(dir) / "cloudflared"; if (fs::exists(cand, ec) && access(cand.c_str(), X_OK) == 0) return cand.wstring(); } if (c == std::string::npos) break; pos = c + 1; }
    if (const char* home = std::getenv("HOME")) { fs::path c = fs::path(home) / ".local" / "bin" / "cloudflared"; if (fs::exists(c, ec)) return c.wstring(); }
    for (const char* d : { "/usr/local/bin/cloudflared", "/usr/bin/cloudflared", "/opt/homebrew/bin/cloudflared" }) if (fs::exists(fs::path(d), ec)) return Utf8ToWide(d);
#endif
    return L"";
}
inline bool HaveCloudflared() { return !CloudflaredPath().empty(); }
inline void TunnelStop() {
    State& s = St(); s.tunCancel = true;
    if (s.tunTh.joinable()) s.tunTh.join();
    std::lock_guard<std::mutex> lk(s.m); s.tunUrl.clear(); s.tunStatus = "desligado"; Bump();
}
inline void TunnelStart(int port) {
    State& s = St();
    if (s.tunRunning.load()) return;
    if (s.tunTh.joinable()) s.tunTh.join();
    s.tunCancel = false; s.tunRunning = true;
    { std::lock_guard<std::mutex> lk(s.m); s.tunUrl.clear(); s.tunStatus = "iniciando..."; }
    Bump();
    s.tunTh = std::thread([port]() {
        State& st = St();
        std::wstring exe = CloudflaredPath();
        if (exe.empty()) { { std::lock_guard<std::mutex> lk(st.m); st.tunStatus = "cloudflared nao encontrado: rode o instalador de dependencias"; } st.tunRunning = false; Bump(); AppPost(EV_HOST_STATUS, L"Túnel: cloudflared não encontrado (rode o instalador de dependências).", 0); return; }
        std::vector<std::wstring> args = { exe, L"tunnel", L"--url", L"http://127.0.0.1:" + std::to_wstring(port), L"--no-autoupdate" };
        CapResult r = RunCapture(args, 0, &st.tunCancel, [](const std::string& ln) {
            size_t p = ln.find("https://"); if (p == std::string::npos) return;
            size_t e = p; while (e < ln.size() && (isalnum((unsigned char)ln[e]) || ln[e] == '-' || ln[e] == '.' || ln[e] == ':' || ln[e] == '/')) e++;
            std::string url = ln.substr(p, e - p);
            if (url.find(".trycloudflare.com") == std::string::npos) return;
            bool novo = false; { std::lock_guard<std::mutex> lk(St().m); if (St().tunUrl != url) { St().tunUrl = url; St().tunStatus = "ligado"; novo = true; } }
            if (novo) { Bump(); AppPost(EV_HOST_STATUS, L"Túnel pronto: " + Utf8ToWide(url), 1); }
        });
        bool cancel = st.tunCancel.load();
        { std::lock_guard<std::mutex> lk(st.m); st.tunUrl.clear(); st.tunStatus = cancel ? "desligado" : (r.started ? "caiu (cloudflared fechou)" : "nao consegui iniciar o cloudflared"); }
        st.tunRunning = false; Bump();
        if (!cancel) AppPost(EV_HOST_STATUS, r.started ? L"O túnel caiu. Ligue de novo no painel HOST." : L"Não consegui iniciar o cloudflared.", 0);
    });
}

// ------------------------------------------------------------ API da UI ---
inline bool Running() { return St().running; }
inline std::string Start(int port, bool lanOk, const std::string& pin, const std::wstring& name, const std::string& accent) {
    State& s = St();
    if (s.running) return "";
    if (!DigitsOnly(pin, 4, 12)) return "Defina um PIN de 4 a 12 numeros nas configuracoes.";
    if (port < 1024 || port > 65535) return "Porta invalida (1024 a 65535).";
    std::string err; hsock_t ls = hostnet::Listen(port, lanOk, err);
    if (ls == HSOCK_BAD) { std::lock_guard<std::mutex> lk(s.m); s.lastError = err; return err; }
    LoadIni();
    { std::lock_guard<std::mutex> lk(s.m); s.ls = ls; s.port = port; s.lanOk = lanOk; s.pin = pin; s.hostName = name.empty() ? Utf8ToWide(hostnet::HostName()) : name; s.accent = accent; s.lastError.clear(); s.lanUrls.clear(); if (lanOk) for (auto& ip : hostnet::LanIPv4()) s.lanUrls.push_back("http://" + ip + ":" + std::to_string(port)); }
    s.stop = false; s.running = true;
    s.th = std::thread(AcceptLoop);
    Bump();
    return "";
}
inline void Stop() {
    State& s = St();
    if (!s.running) return;
    s.stop = true; hostnet::Wake(s.ls); hostnet::Close(s.ls);
    if (s.th.joinable()) s.th.join();
    { std::lock_guard<std::mutex> lk(s.m); s.ls = HSOCK_BAD; s.reqs.clear(); }
    s.running = false;
    TunnelStop();
    Bump();
}
inline void Approve(const std::string& reqId, bool ok) {
    State& s = St(); std::lock_guard<std::mutex> lk(s.m);
    for (auto& q : s.reqs) if (q.id == reqId && q.estado == 0) {
        if (!ok) { q.estado = 2; Bump(); return; }
        Device d; d.id = RandomHex(6); d.name = q.name; d.token = RandomHex(32); d.ip = q.ip; d.created = d.lastSeen = NowSec();
        s.devs.push_back(d); q.estado = 1; q.token = d.token; q.devId = d.id; SaveIni(); Bump(); return;
    }
}
inline void Revoke(const std::string& devId) {
    State& s = St(); std::lock_guard<std::mutex> lk(s.m);
    s.devs.erase(std::remove_if(s.devs.begin(), s.devs.end(), [&](const Device& d) { return d.id == devId; }), s.devs.end());
    s.dpls.erase(std::remove_if(s.dpls.begin(), s.dpls.end(), [&](const DevPlaylist& p) { return p.dev == devId; }), s.dpls.end());
    SaveIni(); Bump();
}
inline std::string Targets(const std::wstring& slug) { State& s = St(); std::lock_guard<std::mutex> lk(s.m); auto it = s.targets.find(slug); return it == s.targets.end() ? "" : it->second; }
inline void SetTargets(const std::wstring& slug, const std::string& t) { State& s = St(); std::lock_guard<std::mutex> lk(s.m); s.targets[slug] = t; SaveIni(); Bump(); }
// Liga/desliga um dispositivo na lista de uma playlist (ALL -> vira lista sem ele; lista -> alterna)
inline void ToggleTargetDevice(const std::wstring& slug, const std::string& devId) {
    State& s = St(); std::lock_guard<std::mutex> lk(s.m);
    std::string t = s.targets[slug]; std::vector<std::string> ids;
    if (t == "ALL") { for (auto& d : s.devs) if (d.id != devId) ids.push_back(d.id); }
    else { size_t p = 0; bool had = false; while (p <= t.size()) { size_t c = t.find(',', p); std::string id = t.substr(p, c == std::string::npos ? std::string::npos : c - p); if (!id.empty()) { if (id == devId) had = true; else ids.push_back(id); } if (c == std::string::npos) break; p = c + 1; } if (!had) ids.push_back(devId); }
    std::string o; for (size_t i = 0; i < ids.size(); i++) { if (i) o += ","; o += ids[i]; }
    if (ids.size() == s.devs.size() && !s.devs.empty()) o = "ALL";
    s.targets[slug] = o; SaveIni(); Bump();
}
// Copia do estado para a UI desenhar (sem segurar o mutex enquanto desenha).
struct View { bool running = false; int port = 0; bool lanOk = true; std::wstring hostName; std::string tunUrl, tunStatus, lastError; bool tunRunning = false; std::vector<Device> devs; std::vector<PairReq> pending; std::vector<DevPlaylist> dpls; std::vector<std::string> lanUrls; unsigned version = 0; };
inline View GetView() {
    State& s = St(); View v; std::lock_guard<std::mutex> lk(s.m);
    v.running = s.running; v.port = s.port; v.lanOk = s.lanOk; v.hostName = s.hostName; v.tunUrl = s.tunUrl; v.tunStatus = s.tunStatus; v.lastError = s.lastError; v.tunRunning = s.tunRunning.load();
    v.devs = s.devs; for (auto& q : s.reqs) if (q.estado == 0) v.pending.push_back(q); v.dpls = s.dpls; v.lanUrls = s.lanUrls; v.version = s.version.load();
    return v;
}
inline PairReq FindReq(const std::string& id) { State& s = St(); std::lock_guard<std::mutex> lk(s.m); for (auto& q : s.reqs) if (q.id == id) return q; return PairReq(); }
// Pagina "conectar" (para mandar pelo WhatsApp): so links, sem PIN, sem IP publico.
inline std::wstring WriteConnectHtml() {
    State& s = St(); std::string tun, nome; std::vector<std::string> lans;
    { std::lock_guard<std::mutex> lk(s.m); tun = s.tunUrl; nome = WideToUtf8(s.hostName); lans = s.lanUrls; }
    std::string ok; for (char c : tun) if (isalnum((unsigned char)c) || c == '-' || c == '.' || c == ':' || c == '/') ok.push_back(c); tun = ok;
    std::string lj = "["; for (size_t i = 0; i < lans.size(); i++) { if (i) lj += ","; lj += JStrA(lans[i]); } lj += "]";
    std::string html = hostweb::CONNECT_HTML;
    html = ReplaceAll(html, "@NOME@", HtmlEsc(nome)); html = ReplaceAll(html, "@TUNEL@", tun); html = ReplaceAll(html, "@LANS@", lj); html = ReplaceAll(html, "#ACCENT", s.accent);
    std::wstring path = Config::Join(Config::BaseDir(), L"Remix-conectar.html");
#ifdef _WIN32
    FILE* f = _wfopen(path.c_str(), L"wb");
#else
    FILE* f = fopen(WideToUtf8(path).c_str(), "wb");
#endif
    if (!f) return L"";
    fwrite(html.data(), 1, html.size(), f); fclose(f);
    return path;
}

// ------------------------------------------------------------ painel (UI) --
struct PanelUI {
    bool open = false; int scroll = 0, contentH = 0;
    RECT box{ 0,0,0,0 }, btnClose{ 0,0,0,0 }, btnToggle{ 0,0,0,0 }, btnTunnel{ 0,0,0,0 }, btnHtml{ 0,0,0,0 }, btnPasta{ 0,0,0,0 }, btnPort{ 0,0,0,0 }, btnPin{ 0,0,0,0 }, btnName{ 0,0,0,0 }, btnLan{ 0,0,0,0 }, list{ 0,0,0,0 };
    std::vector<RECT> accept, deny, revoke, plHost; std::vector<std::vector<RECT>> plDev;
    View v;   // copia usada pelo layout e pelo desenho (atualizada quando version muda)
};
inline PanelUI& PU() { static PanelUI* p = new PanelUI(); return *p; }

} // namespace host
