#pragma once
// Casca Windows: hooks de sistema do app_core.h (eventos de thread, dialogos,
// monitor de pasta, lixeira, ffmpeg, abrir pasta) e a instancia unica.
// Incluido so pelo main.cpp (Windows).
#include "app_core.h"
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <deque>
#include <functional>

static HWND g_hwnd = nullptr;
#define WM_APP_EVENT (WM_APP+1)

// ---- fila de eventos thread -> UI ------------------------------------------
struct WinEv { int type; std::wstring s; int n; };
static std::mutex& EvMutex(){ static std::mutex m; return m; }
static std::deque<WinEv>& EvQueue(){ static std::deque<WinEv>* q=new std::deque<WinEv>(); return *q; }
void AppPost(int type,const std::wstring& s,int n){
    { std::lock_guard<std::mutex> lk(EvMutex()); EvQueue().push_back({type,s,n}); }
    if(g_hwnd) PostMessageW(g_hwnd,WM_APP_EVENT,0,0);
}
static void DrainEvents(){
    while(true){
        WinEv e;
        { std::lock_guard<std::mutex> lk(EvMutex()); if(EvQueue().empty()) return; e=std::move(EvQueue().front()); EvQueue().pop_front(); }
        HandleEvent(e.type,e.s,e.n);
    }
}
void PlatformRedraw(){ if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE); }

// ---- dialogos (rodam numa thread STA propria) -----------------------------
// O SHBrowseForFolder e o GetOpenFileName precisam de uma thread STA (OleInitialize).
// A janela principal roda em MTA (CoInitializeEx no wWinMain, o que faz o streaming do
// SoundCloud funcionar); nessa apartamento os dialogos de pasta/arquivo davam erro e
// derrubavam o app. Entao cada dialogo roda na sua propria thread STA e responde pela
// fila de eventos quando o usuario escolhe - a janela nao trava enquanto ele esta aberto.
static std::atomic<bool> g_pickerBusy{false};

static int CALLBACK BrowseInit(HWND h,UINT msg,LPARAM,LPARAM data){   // abre ja na pasta atual
    if(msg==BFFM_INITIALIZED && data) SendMessageW(h,BFFM_SETSELECTIONW,TRUE,data);
    return 0;
}
static std::wstring PickFolder(HWND owner,const std::wstring& initial){
    BROWSEINFOW bi={0}; bi.hwndOwner=owner; bi.lpszTitle=L"Escolha a pasta"; bi.ulFlags=BIF_NEWDIALOGSTYLE|BIF_RETURNONLYFSDIRS;
    if(!initial.empty()){ bi.lpfn=BrowseInit; bi.lParam=(LPARAM)initial.c_str(); }
    LPITEMIDLIST pidl=SHBrowseForFolderW(&bi); std::wstring result;
    if(pidl){wchar_t buf[MAX_PATH];if(SHGetPathFromIDListW(pidl,buf))result=buf;CoTaskMemFree(pidl);} return result;
}
static std::wstring PickImageFile(HWND owner){
    wchar_t file[MAX_PATH]={0}; OPENFILENAMEW ofn={0}; ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=owner;
    ofn.lpstrFilter=L"Imagens\0*.jpg;*.jpeg;*.png;*.bmp\0Todos\0*.*\0";ofn.lpstrFile=file;ofn.nMaxFile=MAX_PATH;ofn.lpstrTitle=L"Escolha a imagem";ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    return GetOpenFileNameW(&ofn)?file:L"";
}
static std::wstring PickAudioFiles(HWND owner){
    std::vector<wchar_t> buf(65536,0); OPENFILENAMEW ofn={0}; ofn.lStructSize=sizeof(ofn); ofn.hwndOwner=owner;
    ofn.lpstrFilter=L"Áudio\0*.mp3;*.flac;*.ogg;*.opus;*.m4a;*.wav;*.aac;*.wma;*.aiff;*.webm;*.mka\0Todos\0*.*\0";
    ofn.lpstrFile=buf.data(); ofn.nMaxFile=(DWORD)buf.size(); ofn.lpstrTitle=L"Escolha as músicas";
    ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_ALLOWMULTISELECT|OFN_EXPLORER;
    std::wstring out;
    if(GetOpenFileNameW(&ofn)){
        const wchar_t* p=buf.data(); std::wstring first=p; p+=first.size()+1;
        if(!*p) out=first;   // um arquivo so: caminho completo
        else while(*p){ std::wstring name=p; if(!out.empty()) out.push_back(L'\n'); out+=Config::Join(first,name); p+=name.size()+1; }
    }
    return out;
}
// roda o dialogo numa thread STA e devolve o resultado (vazio = cancelou) pela fila de eventos
static void RunPicker(int evType,int ctx,std::function<std::wstring()> pick){
    if(g_pickerBusy.exchange(true)){ AppPost(evType,L"",ctx); return; }   // um dialogo por vez
    std::thread([evType,ctx,pick]{
        std::wstring r;
        RemixSafe("dialogo de pasta/arquivo",[&]{
            HRESULT hr=OleInitialize(NULL);   // STA so nesta thread (a principal e MTA)
            RemixSafe("escolher pasta/arquivo",[&]{ r=pick(); });
            if(SUCCEEDED(hr)) OleUninitialize();
        });
        g_pickerBusy=false;
        AppPost(evType,r,ctx);
    }).detach();
}
static std::wstring DlFolderInit(){ std::wstring f=g_cfg.downloadFolder; return f.empty()?std::wstring():Config::FromPortable(f); }
void PlatformPickFolderAsync(){ HWND o=g_hwnd; std::wstring init=Config::FromPortable(g_cfg.musicFolder); RunPicker(EV_PICK_FOLDER,0,[o,init]{ return PickFolder(o,init); }); }
void PlatformPickImageAsync(int evType,int ctx){ HWND o=g_hwnd; RunPicker(evType,ctx,[o]{ return PickImageFile(o); }); }
void PlatformPickFolderFor(int evType,int ctx){ HWND o=g_hwnd; std::wstring init=(evType==EV_PICK_DLFOLDER||evType==EV_PICK_DLONCE)?DlFolderInit():std::wstring(); RunPicker(evType,ctx,[o,init]{ return PickFolder(o,init); }); }
void PlatformPickAudioFilesAsync(int evType,int ctx){ HWND o=g_hwnd; RunPicker(evType,ctx,[o]{ return PickAudioFiles(o); }); }
std::wstring PlatformClipboardText(){
    std::wstring r; if(!OpenClipboard(g_hwnd)) return r;
    HANDLE h=GetClipboardData(CF_UNICODETEXT);
    if(h){ wchar_t* p=(wchar_t*)GlobalLock(h); if(p){ r=p; GlobalUnlock(h); } }
    CloseClipboard(); return r;
}

// ---- lixeira / abrir pasta -------------------------------------------------
bool PlatformTrash(const std::wstring& path){
    std::wstring dbl=path; dbl.push_back(L'\0'); dbl.push_back(L'\0');
    SHFILEOPSTRUCTW op={0}; op.hwnd=g_hwnd; op.wFunc=FO_DELETE; op.pFrom=dbl.c_str(); op.fFlags=FOF_ALLOWUNDO|FOF_NOCONFIRMATION|FOF_SILENT|FOF_NOERRORUI;
    return SHFileOperationW(&op)==0 && !op.fAnyOperationsAborted;
}
void PlatformOpenFolder(const std::wstring& path){ ShellExecuteW(NULL,L"open",path.c_str(),NULL,NULL,SW_SHOWNORMAL); }

// ---- ffmpeg (opcional): so dentro do Remix, em <app>\assets\tools -------------
static std::wstring FfmpegPath(){
    static std::wstring cached; static bool tried=false;
    if(tried) return cached; tried=true;
    std::wstring cands[]={ Config::Join(Config::Join(Config::AssetDir(),L"tools"),L"ffmpeg.exe") };
    for(auto& c:cands) if(GetFileAttributesW(c.c_str())!=INVALID_FILE_ATTRIBUTES){ cached=c; return cached; }
    return cached;
}
bool PlatformHaveFfmpeg(){ return !FfmpegPath().empty(); }
std::wstring PlatformTranscodeToWav(const std::wstring& src){
    namespace fs=std::filesystem;
    std::wstring ff=FfmpegPath(); if(ff.empty()) return L"";
    std::error_code ec;
    fs::path cache(Config::CacheDir()); fs::create_directories(cache,ec);
    size_t h=std::hash<std::wstring>{}(src); wchar_t name[64]; swprintf(name,64,L"%016llx.wav",(unsigned long long)h);
    fs::path dst=cache/name;
    if(fs::exists(dst,ec)){
        auto ts=fs::last_write_time(fs::path(src),ec); std::error_code e2; auto td=fs::last_write_time(dst,e2);
        if(!ec&&!e2&&td>=ts&&fs::file_size(dst,e2)>1000) return dst.wstring();
    }
    fs::path tmp=cache/(std::wstring(name)+L".part");
    std::wstring cmd=L"\""+ff+L"\" -nostdin -loglevel error -y -i \""+src+L"\" -vn -map_metadata -1 -acodec pcm_s16le -f wav \""+tmp.wstring()+L"\"";
    STARTUPINFOW si={0}; si.cb=sizeof(si); PROCESS_INFORMATION pi={0};
    std::vector<wchar_t> cl(cmd.begin(),cmd.end()); cl.push_back(0);
    if(!CreateProcessW(NULL,cl.data(),NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi)) return L"";
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD code=1; GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    if(code!=0||!fs::exists(tmp,ec)){ fs::remove(tmp,ec); return L""; }
    fs::rename(tmp,dst,ec); if(ec) return L"";
    return dst.wstring();
}
static void CleanupCacheWin(int maxAgeHours){
    namespace fs=std::filesystem; std::error_code ec;
    fs::path cache(Config::CacheDir()); if(!fs::exists(cache,ec)) return;
    auto now=fs::file_time_type::clock::now();
    for(fs::directory_iterator it(cache,fs::directory_options::skip_permission_denied,ec),end;it!=end;it.increment(ec)){
        if(ec) break; std::error_code e2; if(!it->is_regular_file(e2)) continue;
        auto ext=it->path().extension().wstring(); if(ext!=L".wav"&&ext!=L".part") continue;
        auto t=fs::last_write_time(it->path(),e2); if(e2) continue;
        auto age=std::chrono::duration_cast<std::chrono::hours>(now-t).count();
        if(ext==L".part"||age>=maxAgeHours) fs::remove(it->path(),e2);
    }
}

// ---- monitor da pasta (ReadDirectoryChangesW) -----------------------------
static std::thread g_watchThread;
static HANDLE g_watchDir = nullptr;
static std::atomic<bool> g_watchStop{true};
static std::atomic<unsigned long long> g_watchChangeTick{0};
static std::atomic<int> g_watchPending{0};
static void FolderWatchThread(std::wstring folder){
    HANDLE h = CreateFileW(folder.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if(h==INVALID_HANDLE_VALUE) return;
    if(g_watchStop.load()){ CloseHandle(h); return; }
    g_watchDir = h;
    while(!g_watchStop.load()){
        DWORD bytes = 0; DWORD buf[16384];
        BOOL ok = ReadDirectoryChangesW(h, buf, sizeof(buf), TRUE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_DIR_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_SIZE, &bytes, NULL, NULL);
        if(ok){ g_watchChangeTick = (unsigned long long)GetTickCount64(); g_watchPending++; }
        else { if(g_watchStop.load()) break; Sleep(300); }
    }
    if(g_watchDir==h) g_watchDir = nullptr;
    CloseHandle(h);
}
void PlatformWatchStop(){
    g_watchStop = true;
    HANDLE h = g_watchDir;
    if(h) CancelIoEx(h, NULL);
    if(g_watchThread.joinable()) g_watchThread.join();
    g_watchThread = std::thread();
}
void PlatformWatchStart(const std::wstring& folder){
    PlatformWatchStop();
    std::wstring abs = Config::FromPortable(folder);
    if(abs.empty() || !Config::DirExists(abs)) return;
    g_watchStop = false;
    try { g_watchThread = std::thread(FolderWatchThread, abs); } catch(...) { g_watchStop = true; }
}
bool PlatformWatchTake(unsigned long long quietMs){
    if(g_watchPending.load()<=0) return false;
    if(GetTickCount64()-g_watchChangeTick.load()<quietMs) return false;
    return g_watchPending.exchange(0)>0;
}

// ---- instancia unica + comando --play-file ---------------------------------
static HANDLE g_singleMutex = nullptr;
static bool SendToExisting(const std::wstring& cmd){
    HWND h=FindWindowW(L"MusicPlayerRemixWnd",L"Remix Player"); if(!h) return false;
    COPYDATASTRUCT cds={0}; cds.dwData=1; cds.cbData=(DWORD)((cmd.size()+1)*sizeof(wchar_t)); cds.lpData=(void*)cmd.c_str();
    SendMessageW(h,WM_COPYDATA,0,(LPARAM)&cds); ShowWindow(h,SW_RESTORE); SetForegroundWindow(h); return true;
}
