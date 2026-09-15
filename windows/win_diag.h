#pragma once
// Diagnostico no Windows. Antes, qualquer erro na abertura fechava o Remix em silencio
// (o usuario so via o "carregando" e o processo sumia). Agora:
//   - remix-log.txt: cada etapa da abertura com horario, versao do Windows, som, erros
//     (fica ao lado do app no modo portatil, ou em %LOCALAPPDATA%\Remix);
//   - erro fatal (excecao, excecao C++ sem tratamento, abort) grava o motivo no log e
//     mostra um aviso na tela; parametro invalido numa funcao da CRT so e registrado
//     (antes o Windows encerrava o processo na hora, sem mensagem);
//   - MODO SEGURO: se a abertura anterior nao chegou ao fim, abre sem splash/som de
//     abertura, efeitos, bandeja, teclas de midia e atalhos globais.
#include "win_sys.h"
#include <csignal>
#include <cstdarg>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#ifndef _WRITE_ABORT_MSG
#define _WRITE_ABORT_MSG 0x1
#endif
#ifndef _CALL_REPORTFAULT
#define _CALL_REPORTFAULT 0x2
#endif

static HANDLE g_diagFile = INVALID_HANDLE_VALUE;
static CRITICAL_SECTION g_diagLock;
static bool g_diagLockOk = false;
static std::wstring g_diagLog, g_diagMarker;
static ULONGLONG g_diagT0 = 0;
static char g_diagStage[160] = "inicio";
static std::atomic<int> g_diagParamCount{0}, g_diagExcCount{0};
static std::atomic<bool> g_diagDying{false};

static void DiagLine(const char* fmt, ...) {
    static thread_local char buf[1600];
    int k = snprintf(buf, sizeof buf, "[+%llu ms] ", (unsigned long long)(GetTickCount64() - g_diagT0));
    if (k < 0) k = 0;
    va_list ap; va_start(ap, fmt);
    int m = vsnprintf(buf + k, sizeof buf - (size_t)k - 3, fmt, ap);
    va_end(ap);
    size_t len = (size_t)k + (size_t)(m < 0 ? 0 : std::min(m, (int)(sizeof buf - (size_t)k - 4)));
    buf[len++] = '\r'; buf[len++] = '\n';
    bool locked = false;
    if (g_diagLockOk) { if (g_diagDying.load()) locked = TryEnterCriticalSection(&g_diagLock) != FALSE; else { EnterCriticalSection(&g_diagLock); locked = true; } }
    if (g_diagFile != INVALID_HANDLE_VALUE) {   // sempre no fim do arquivo: uma segunda abertura do Remix tambem acrescenta linhas
        OVERLAPPED ov = {}; ov.Offset = 0xFFFFFFFF; ov.OffsetHigh = 0xFFFFFFFF;
        DWORD w = 0; WriteFile(g_diagFile, buf, (DWORD)len, &w, &ov); FlushFileBuffers(g_diagFile);
    }
    if (locked) LeaveCriticalSection(&g_diagLock);
}
void PlatformLog(const char* msg) { DiagLine("%s", msg); }
static void DiagStage(const char* s) { snprintf(g_diagStage, sizeof g_diagStage, "%s", s); DiagLine("etapa: %s", s); }
static std::wstring DiagLogName() { return g_diagLog.empty() ? std::wstring(L"remix-log.txt") : g_diagLog; }

// Aviso de erro fatal. Sem alocar memoria (o heap pode estar corrompido).
static void DiagDialog(const char* what) {
    static wchar_t wWhat[400], wStage[200], text[3000];
    wchar_t env[8];
    if (GetEnvironmentVariableW(L"REMIX_NO_CRASH_DIALOG", env, 8)) return;   // testes automaticos
    MultiByteToWideChar(CP_UTF8, 0, what, -1, wWhat, 400); wWhat[399] = 0;
    MultiByteToWideChar(CP_UTF8, 0, g_diagStage, -1, wStage, 200); wStage[199] = 0;
    swprintf(text, 3000,
             L"O Remix fechou por causa de um erro:\n%ls\n\nEtapa: %ls\n\nO relatório foi salvo em:\n%ls\n\n"
             L"Mande esse arquivo para quem te passou o Remix.\n"
             L"Na próxima abertura ele entra em modo seguro (sem splash, efeitos e bandeja).",
             wWhat, wStage, g_diagLog.empty() ? L"(não consegui criar o remix-log.txt)" : g_diagLog.c_str());
    MessageBoxW(NULL, text, L"Remix Player", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
}

// Enderecos de codigo do Remix.exe na pilha a partir de sp. Com o Remix-sym.exe do mesmo build,
// "bash linux/traduzir-log-windows.sh remix-log.txt" mostra a funcao e a linha de cada um.
static void DiagStackScan(ULONG_PTR sp) {
#if defined(__x86_64__) || defined(_M_X64)
    HMODULE self = GetModuleHandleW(NULL);
    if (!self) return;
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)((BYTE*)self + ((IMAGE_DOS_HEADER*)self)->e_lfanew);
    ULONG_PTR lo = (ULONG_PTR)self, hi = lo + nt->OptionalHeader.SizeOfImage;
    IMAGE_SECTION_HEADER* sec = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        if (memcmp(sec[i].Name, ".text", 5) == 0) { lo = (ULONG_PTR)self + sec[i].VirtualAddress; hi = lo + sec[i].Misc.VirtualSize; break; }
    NT_TIB* tib = (NT_TIB*)NtCurrentTeb();
    ULONG_PTR top = (ULONG_PTR)tib->StackBase;
    static char line[1100]; int n = snprintf(line, sizeof line, "  pilha (offsets no Remix.exe):");
    int found = 0;
    if (sp && sp < top && sp > (ULONG_PTR)tib->StackLimit - 0x100000)
        for (ULONG_PTR p = sp & ~(ULONG_PTR)7; p + sizeof(ULONG_PTR) <= top && p < sp + 0x8000 && found < 24 && n < (int)sizeof line - 24; p += sizeof(ULONG_PTR)) {
            ULONG_PTR v = *(ULONG_PTR*)p;
            if (v >= lo && v < hi) { n += snprintf(line + n, sizeof line - (size_t)n, " 0x%llX", (unsigned long long)(v - (ULONG_PTR)self)); ++found; }
        }
    DiagLine("%s", line);
#else
    (void)sp;
#endif
}

static LONG WINAPI DiagCrashFilter(EXCEPTION_POINTERS* ep) {
    // Excecao C++ sem catch (SEH do GCC) passa por aqui antes do std::terminate: devolve para o
    // __cxa_throw chamar o terminate, que grava a mensagem (o filtro padrao do MinGW fazia isso).
    if (ep && ep->ExceptionRecord && (ep->ExceptionRecord->ExceptionCode & 0x20FFFFFFul) == 0x20474343ul &&
        !(ep->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (g_diagDying.exchange(true)) return EXCEPTION_EXECUTE_HANDLER;
    DWORD code = 0; ULONG_PTR addr = 0;
    if (ep && ep->ExceptionRecord) { code = ep->ExceptionRecord->ExceptionCode; addr = (ULONG_PTR)ep->ExceptionRecord->ExceptionAddress; }
    static char mod[MAX_PATH * 3]; snprintf(mod, sizeof mod, "?");
    ULONG_PTR off = addr; HMODULE hm = NULL;
    if (addr && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)addr, &hm) && hm) {
        static wchar_t w[MAX_PATH];
        if (GetModuleFileNameW(hm, w, MAX_PATH)) { const wchar_t* base = wcsrchr(w, L'\\'); WideCharToMultiByte(CP_UTF8, 0, base ? base + 1 : w, -1, mod, sizeof mod, NULL, NULL); }
        off = addr - (ULONG_PTR)hm;
    }
    DiagLine("ERRO FATAL: excecao 0x%08lX em %s+0x%llX (thread %lu, etapa: %s)", (unsigned long)code, mod, (unsigned long long)off, (unsigned long)GetCurrentThreadId(), g_diagStage);
    if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
        ULONG_PTR kind = ep->ExceptionRecord->ExceptionInformation[0];
        DiagLine("  acesso invalido (%s) no endereco 0x%llX", kind == 1 ? "escrita" : kind == 8 ? "execucao" : "leitura", (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1]);
    }
#if defined(__x86_64__) || defined(_M_X64)
    if (ep && ep->ContextRecord) DiagStackScan((ULONG_PTR)ep->ContextRecord->Rsp);
#endif
    static char what[120]; snprintf(what, sizeof what, "exceção 0x%08lX em %s", (unsigned long)code, mod);
    DiagDialog(what);
    return EXCEPTION_EXECUTE_HANDLER;
}
static void DiagTerminate() {
    static char what[700]; snprintf(what, sizeof what, "exceção C++ sem tratamento");
    try { if (std::exception_ptr p = std::current_exception()) std::rethrow_exception(p); }
    catch (const std::exception& e) { snprintf(what, sizeof what, "exceção C++ sem tratamento: %s", e.what()); }
    catch (...) {}
    if (!g_diagDying.exchange(true)) {
        DiagLine("ERRO FATAL: %s (thread %lu, etapa: %s)", what, (unsigned long)GetCurrentThreadId(), g_diagStage);
        char here = 0; DiagStackScan((ULONG_PTR)&here);   // quem lancou a excecao esta logo acima na pilha
        DiagDialog(what);
    }
    TerminateProcess(GetCurrentProcess(), 3);
}
static void __cdecl DiagInvalidParam(const wchar_t*, const wchar_t* func, const wchar_t*, unsigned int, uintptr_t) {
    int n = ++g_diagParamCount; if (n > 20) return;
    char f8[200] = ""; if (func) WideCharToMultiByte(CP_UTF8, 0, func, -1, f8, sizeof f8, NULL, NULL);
    DiagLine("aviso: parametro invalido numa funcao da CRT%s%s - ignorado, o app continua%s", f8[0] ? ": " : "", f8, n == 20 ? " [proximos avisos omitidos]" : "");
}
static void DiagAbort(int) {
    if (!g_diagDying.exchange(true)) { DiagLine("ERRO FATAL: abort() (thread %lu, etapa: %s)", (unsigned long)GetCurrentThreadId(), g_diagStage); char here = 0; DiagStackScan((ULONG_PTR)&here); DiagDialog("abort()"); }
    TerminateProcess(GetCurrentProcess(), 3);
}
// Excecao tratada no WndProc (o app continua): registra as primeiras.
static void DiagException(UINT msg, const char* what) {
    int n = ++g_diagExcCount; if (n > 30) return;
    DiagLine("aviso: excecao tratada na janela (mensagem 0x%04X): %s%s", (unsigned)msg, what, n == 30 ? " [proximos avisos omitidos]" : "");
}

static std::string DiagWindowsVersion() {
    const wchar_t* key = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
    auto rs = [&](const wchar_t* name) -> std::wstring { wchar_t b[160]; DWORD sz = sizeof b; if (RegGetValueW(HKEY_LOCAL_MACHINE, key, name, RRF_RT_REG_SZ, NULL, b, &sz) == ERROR_SUCCESS) return b; return L""; };
    DWORD ubr = 0, sz = sizeof ubr; RegGetValueW(HKEY_LOCAL_MACHINE, key, L"UBR", RRF_RT_REG_DWORD, NULL, &ubr, &sz);
    std::wstring prod = rs(L"ProductName"), disp = rs(L"DisplayVersion"), build = rs(L"CurrentBuild");
    if (_wtoi(build.c_str()) >= 22000) { size_t k = prod.find(L"Windows 10"); if (k != std::wstring::npos) prod.replace(k, 10, L"Windows 11"); }   // o registro do 11 ainda diz "Windows 10"
    wchar_t out[400]; swprintf(out, 400, L"%ls %ls (build %ls.%lu)", prod.c_str(), disp.c_str(), build.c_str(), (unsigned long)ubr);
    return WideToUtf8(out);
}
// Abrir o .exe de dentro do .zip pelo Explorer tira so o exe para uma pasta temporaria.
static void DiagCheckZipTemp() {
    std::wstring low = Config::ExeDir(); for (auto& c : low) c = (wchar_t)towlower(c);
    if (low.find(L"\\temp1_") == std::wstring::npos && low.find(L".zip\\") == std::wstring::npos && low.find(L"\\rar$ex") == std::wstring::npos && low.find(L"\\7zo") == std::wstring::npos) return;
    DiagLine("aviso: o Remix parece ter sido aberto de dentro de um .zip (%s)", WideToUtf8(Config::ExeDir()).c_str());
    wchar_t env[8]; if (GetEnvironmentVariableW(L"REMIX_NO_CRASH_DIALOG", env, 8)) return;
    MessageBoxW(NULL, L"Parece que o Remix foi aberto direto de dentro do arquivo .zip.\n\n"
                      L"Assim o Windows tira só o Remix.exe e deixa de fora as imagens, a configuração e as músicas.\n\n"
                      L"Feche, clique com o botão direito no .zip, escolha \"Extrair tudo...\" e abra o Remix.exe da pasta extraída.",
                L"Remix Player", MB_OK | MB_ICONWARNING);
}
static void DiagInit(bool forceSafe) {
    g_diagT0 = GetTickCount64();
    InitializeCriticalSection(&g_diagLock); g_diagLockOk = true;
    std::vector<std::wstring> dirs = { Config::BaseDir() };
    wchar_t la[MAX_PATH]; DWORD k = GetEnvironmentVariableW(L"LOCALAPPDATA", la, MAX_PATH);
    if (k && k < MAX_PATH) dirs.push_back(std::wstring(la) + L"\\Remix");
    std::wstring dir;
    for (auto& d : dirs) {   // pasta do app sem permissao de escrita (ex.: Arquivos de Programas): usa %LOCALAPPDATA%\Remix
        CreateDirectoryW(d.c_str(), NULL);
        std::wstring log = d + L"\\remix-log.txt", old = d + L"\\remix-log-anterior.txt";
        if (GetFileAttributesW(log.c_str()) != INVALID_FILE_ATTRIBUTES) MoveFileExW(log.c_str(), old.c_str(), MOVEFILE_REPLACE_EXISTING);
        HANDLE h = CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) { g_diagFile = h; g_diagLog = log; dir = d; break; }
    }
    if (dir.empty()) dir = dirs[0];
    g_diagMarker = dir + L"\\remix-abrindo.tmp";
    bool lastFailed = GetFileAttributesW(g_diagMarker.c_str()) != INVALID_FILE_ATTRIBUTES;
    g_safeMode = forceSafe || lastFailed;
    HANDLE m = CreateFileW(g_diagMarker.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
    if (m != INVALID_HANDLE_VALUE) { DWORD w = 0; WriteFile(m, "abrindo\r\n", 9, &w, NULL); CloseHandle(m); }
    SetUnhandledExceptionFilter(DiagCrashFilter);
    _set_invalid_parameter_handler(DiagInvalidParam);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    signal(SIGABRT, DiagAbort);
    std::set_terminate(DiagTerminate);
    SYSTEMTIME st; GetLocalTime(&st);
    wchar_t exe[MAX_PATH] = L""; GetModuleFileNameW(NULL, exe, MAX_PATH);
    HDC hdc = GetDC(NULL); int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSX) : 0; if (hdc) ReleaseDC(NULL, hdc);
    DiagLine("Remix Player 1.2.0 (Windows) - log de diagnostico");
    {   // id do build (carimbo do cabecalho PE): o linux/traduzir-log-windows.sh acha o Remix-sym.exe certo por ele
        HMODULE self = GetModuleHandleW(NULL); DWORD stamp = 0;
        if (self) stamp = ((IMAGE_NT_HEADERS*)((BYTE*)self + ((IMAGE_DOS_HEADER*)self)->e_lfanew))->FileHeader.TimeDateStamp;
        time_t t = (time_t)stamp; struct tm g = {}; char when[40] = "?";
        if (stamp && gmtime_s(&g, &t) == 0) strftime(when, sizeof when, "%Y-%m-%d %H:%M UTC", &g);
        DiagLine("build: %08lX (compilado em %s)", (unsigned long)stamp, when);
    }
    DiagLine("data: %04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    DiagLine("windows: %s | %u processador(es) | tela %dx%d | DPI %d", DiagWindowsVersion().c_str(), (unsigned)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS),
             GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), dpi);
    DiagLine("exe: %s", WideToUtf8(exe).c_str());
    DiagLine("pasta do app: %s", WideToUtf8(Config::BaseDir()).c_str());
    if (g_safeMode) DiagLine("MODO SEGURO: %s", forceSafe ? "pedido na linha de comando (--seguro)" : "a abertura anterior nao terminou (veja remix-log-anterior.txt)");
    DiagCheckZipTemp();
}
static void DiagStartupOk() { DiagLine("inicializacao ok: 8 s aberto sem erro"); DeleteFileW(g_diagMarker.c_str()); }
static void DiagExit() { DiagLine("saida normal"); DeleteFileW(g_diagMarker.c_str()); }
static void DiagFatal(const wchar_t* msg) {
    DiagLine("ERRO FATAL: %s", WideToUtf8(msg).c_str());
    std::wstring t = std::wstring(msg) + L"\n\nDetalhes em:\n" + DiagLogName();
    MessageBoxW(NULL, t.c_str(), L"Remix Player", MB_OK | MB_ICONERROR);
}
// Outra abertura segura a instancia unica, mas a janela dela nao aparece (travou abrindo ou esta
// parada num aviso). Antes esta segunda abertura saia calada, como se o Remix nao abrisse.
static void DiagOtherInstanceStuck() {
    std::vector<std::wstring> logs = { Config::BaseDir() + L"\\remix-log.txt" };
    wchar_t la[MAX_PATH]; DWORD k = GetEnvironmentVariableW(L"LOCALAPPDATA", la, MAX_PATH);
    if (k && k < MAX_PATH) logs.push_back(std::wstring(la) + L"\\Remix\\remix-log.txt");
    for (auto& l : logs) {
        HANDLE h = CreateFileW(l.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) continue;
        char b[200]; int n = snprintf(b, sizeof b, "[outra abertura, processo %lu] o Remix ja estava rodando, mas sem janela depois de 5 s\r\n", (unsigned long)GetCurrentProcessId());
        DWORD w = 0; if (n > 0) WriteFile(h, b, (DWORD)n, &w, NULL);
        CloseHandle(h);
        break;
    }
    wchar_t env[8]; if (GetEnvironmentVariableW(L"REMIX_NO_CRASH_DIALOG", env, 8)) return;
    MessageBoxW(NULL, L"O Remix já está rodando, mas a janela dele não apareceu.\n\n"
                      L"Se em alguns segundos nada abrir: abra o Gerenciador de Tarefas (Ctrl+Shift+Esc), finalize o \"Remix.exe\" "
                      L"e abra de novo - ele volta em modo seguro.\n\n"
                      L"Se acontecer de novo, mande o arquivo remix-log.txt (fica na pasta do Remix.exe).",
                L"Remix Player", MB_OK | MB_ICONWARNING);
}
