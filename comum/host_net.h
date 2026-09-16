#pragma once
// Rede do Host (servidor HTTP embutido, docs/HOST.md): sockets IPv4 bloqueantes com
// timeout, Windows (Winsock2) e POSIX. So o necessario: ouvir, aceitar, ler, escrever,
// listar os IPs da rede local e o nome do PC. Nada de IPv6 (decisao do projeto).
#include "platform.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#ifdef _WIN32
    // winsock2.h/ws2tcpip.h vem por platform.h (antes de windows.h)
    #include <iphlpapi.h>
    typedef SOCKET hsock_t;
    #define HSOCK_BAD INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <netdb.h>
    #include <errno.h>
    typedef int hsock_t;
    #define HSOCK_BAD (-1)
#endif

namespace hostnet {

inline bool Init() {
#ifdef _WIN32
    static bool done = false, ok = false;
    if (!done) { done = true; WSADATA w; ok = WSAStartup(MAKEWORD(2, 2), &w) == 0; }
    return ok;
#else
    return true;
#endif
}
inline void Close(hsock_t s) {
    if (s == HSOCK_BAD) return;
#ifdef _WIN32
    closesocket(s);
#else
    close(s);
#endif
}
// Acorda um accept() parado em outra thread (Linux precisa do shutdown antes do close).
inline void Wake(hsock_t s) {
    if (s == HSOCK_BAD) return;
#ifdef _WIN32
    shutdown(s, SD_BOTH);
#else
    shutdown(s, SHUT_RDWR);
#endif
}
inline void SetTimeout(hsock_t s, int ms) {
#ifdef _WIN32
    DWORD t = (DWORD)ms;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof t);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&t, sizeof t);
#else
    timeval tv; tv.tv_sec = ms / 1000; tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);
#endif
}
// Escuta na porta (IPv4). lanOk=false -> so 127.0.0.1 (o tunel ainda funciona).
inline hsock_t Listen(int port, bool lanOk, std::string& err) {
    err.clear();
    if (!Init()) { err = "rede indisponivel"; return HSOCK_BAD; }
    // Sem heranca: processos filhos (cloudflared, yt-dlp, ffmpeg) NAO podem levar o socket
    // junto, senao ao fechar o Remix eles seguram a porta e a proxima abertura falha.
#ifdef _WIN32
    hsock_t s = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT);
#else
    hsock_t s = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
#endif
    if (s == HSOCK_BAD) { err = "socket"; return HSOCK_BAD; }
    int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof one);
    sockaddr_in a; std::memset(&a, 0, sizeof a);
    a.sin_family = AF_INET; a.sin_port = htons((uint16_t)port);
    a.sin_addr.s_addr = lanOk ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
    if (bind(s, (sockaddr*)&a, sizeof a) != 0) { err = "porta " + std::to_string(port) + " em uso (ou sem permissao)"; Close(s); return HSOCK_BAD; }
    if (listen(s, 32) != 0) { err = "listen"; Close(s); return HSOCK_BAD; }
    return s;
}
inline hsock_t Accept(hsock_t ls, std::string& ip) {
    sockaddr_in a; socklen_t n = sizeof a;
#ifdef _WIN32
    hsock_t c = accept(ls, (sockaddr*)&a, &n);
    if (c == HSOCK_BAD) return c;
    SetHandleInformation((HANDLE)c, HANDLE_FLAG_INHERIT, 0);
#else
    hsock_t c = accept4(ls, (sockaddr*)&a, &n, SOCK_CLOEXEC);
    if (c == HSOCK_BAD) return c;
#endif
    char buf[64]; const char* p = inet_ntop(AF_INET, &a.sin_addr, buf, sizeof buf);
    ip = p ? p : "?";
    int one = 1; setsockopt(c, IPPROTO_TCP, TCP_NODELAY, (const char*)&one, sizeof one);
    SetTimeout(c, 10000);
    return c;
}
inline long Recv(hsock_t s, char* b, size_t n) {
#ifdef _WIN32
    return (long)recv(s, b, (int)n, 0);
#else
    return (long)recv(s, b, n, 0);
#endif
}
inline bool SendAll(hsock_t s, const char* b, size_t n) {
    while (n) {
#ifdef _WIN32
        int r = send(s, b, (int)n, 0);
#else
        long r = send(s, b, n, MSG_NOSIGNAL);
#endif
        if (r <= 0) return false;
        b += r; n -= (size_t)r;
    }
    return true;
}
inline bool SendAll(hsock_t s, const std::string& d) { return SendAll(s, d.data(), d.size()); }

// IPs IPv4 da rede local (Wi-Fi e cabo), sem loopback e sem link-local (169.254.*).
inline std::vector<std::string> LanIPv4() {
    std::vector<std::string> out;
    Init();
#ifdef _WIN32
    ULONG sz = 16 * 1024; std::vector<char> buf(sz);
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    if (GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buf.data(), &sz) == ERROR_BUFFER_OVERFLOW) { buf.resize(sz); }
    if (GetAdaptersAddresses(AF_INET, flags, NULL, (IP_ADAPTER_ADDRESSES*)buf.data(), &sz) != NO_ERROR) return out;
    for (IP_ADAPTER_ADDRESSES* ad = (IP_ADAPTER_ADDRESSES*)buf.data(); ad; ad = ad->Next) {
        if (ad->OperStatus != IfOperStatusUp || ad->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        if (ad->IfType != IF_TYPE_ETHERNET_CSMACD && ad->IfType != IF_TYPE_IEEE80211) continue;   // so cabo e Wi-Fi
        { std::wstring fn = ad->FriendlyName ? ad->FriendlyName : L""; for (auto& c : fn) c = (wchar_t)towlower(c);
          if (fn.find(L"virtual") != std::wstring::npos || fn.find(L"vethernet") != std::wstring::npos || fn.find(L"vmware") != std::wstring::npos || fn.find(L"hyper-v") != std::wstring::npos || fn.find(L"tailscale") != std::wstring::npos) continue; }
        for (IP_ADAPTER_UNICAST_ADDRESS* u = ad->FirstUnicastAddress; u; u = u->Next) {
            if (u->Address.lpSockaddr->sa_family != AF_INET) continue;
            char b[64]; sockaddr_in* si = (sockaddr_in*)u->Address.lpSockaddr;
            if (!inet_ntop(AF_INET, &si->sin_addr, b, sizeof b)) continue;
            std::string ip = b;
            if (ip.rfind("127.", 0) == 0 || ip.rfind("169.254.", 0) == 0) continue;
            out.push_back(ip);
        }
    }
#else
    ifaddrs* ifa = nullptr;
    if (getifaddrs(&ifa) != 0) return out;
    for (ifaddrs* p = ifa; p; p = p->ifa_next) {
        if (!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
        if (!(p->ifa_flags & IFF_UP) || (p->ifa_flags & IFF_LOOPBACK)) continue;
        { std::string nm = p->ifa_name ? p->ifa_name : "";   // pula redes virtuais (docker, VMs, VPN): o celular nao chega nelas
          static const char* skip[] = { "docker", "br-", "veth", "virbr", "tun", "tap", "vmnet", "vboxnet", "tailscale", "wg", "zt" };
          bool v = false; for (const char* k : skip) if (nm.rfind(k, 0) == 0) v = true; if (v) continue; }
        char b[64]; sockaddr_in* si = (sockaddr_in*)p->ifa_addr;
        if (!inet_ntop(AF_INET, &si->sin_addr, b, sizeof b)) continue;
        std::string ip = b;
        if (ip.rfind("127.", 0) == 0 || ip.rfind("169.254.", 0) == 0) continue;
        out.push_back(ip);
    }
    freeifaddrs(ifa);
#endif
    return out;
}
inline std::string HostName() {
    Init();
    char b[256] = { 0 };
    if (gethostname(b, sizeof b - 1) != 0 || !b[0]) return "Remix";
    return b;
}

} // namespace hostnet
