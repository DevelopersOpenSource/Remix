// Remix Player — casca Windows (Win32 + GDI+). A logica e compartilhada com o
// Linux (app_core.h / app_input.h); aqui ficam a janela, o WndProc, os hooks
// que precisam da janela e o wWinMain. Audio: miniaudio (player_ma.h), igual
// ao Linux — EQ, FLAC/OGG, taxa nativa e ffmpeg valem nos dois.
#define _CRT_SECURE_NO_WARNINGS
#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include "app_core.h"
#include "app_layout.h"
#include "app_input.h"
#include "win_sys.h"
#include "win_web.h"
#include "win_draw.h"
#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "dwmapi.lib")

static ULONG_PTR g_gdiToken = 0;

// ------------------------------------------------ hooks com janela ---------
void PlatformLoadCover(const std::wstring& path){ if(g_coverImg){delete g_coverImg;g_coverImg=nullptr;} if(!path.empty()){ g_coverImg=new Image(path.c_str()); if(g_coverImg->GetLastStatus()!=Ok){delete g_coverImg;g_coverImg=nullptr;} } InvalidateBgCache(); }
void PlatformEvictThumb(const std::wstring& path){ auto it=g_thumbCache.find(path); if(it==g_thumbCache.end()) return; delete it->second; g_thumbCache.erase(it); }
void PlatformClearThumbs(){ for(auto&kv:g_thumbCache) delete kv.second; g_thumbCache.clear(); }
static void ClampToWork(int& w,int& h){
    RECT wa; if(!SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0)) return;
    int aw=wa.right-wa.left-40, ah=wa.bottom-wa.top-40;
    if(w>aw)w=std::max(MIN_WIN_W,aw);
    if(h>ah)h=std::max(MIN_WIN_H,ah);
}
static void NormalSize(int& w,int& h){
    RECT wa; int aw=1500, ah=812;
    if(SystemParametersInfoW(SPI_GETWORKAREA,0,&wa,0)){ aw=wa.right-wa.left-40; ah=wa.bottom-wa.top-40; }
    if(g_cfg.winW>=720&&g_cfg.winH>=540){ w=g_cfg.winW; h=g_cfg.winH; }
    else { w=std::min(1500,(int)(aw*0.92f)); h=std::min(812,(int)(ah*0.90f)); if(w<720)w=720; if(h<540)h=540; }
    ClampToWork(w,h);
}
void PlatformResizeForMode(){
    if(!g_hwnd) return;
    RECT wr;GetWindowRect(g_hwnd,&wr);
    int w,h; if(g_cfg.displayMode==L"vertical"){w=460;h=700;ClampToWork(w,h);} else NormalSize(w,h);
    SetWindowPos(g_hwnd,NULL,wr.left,wr.top,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
    RECT rc;GetClientRect(g_hwnd,&rc); g_winW=rc.right; g_winH=rc.bottom; BuildLayout();
}
void PlatformMinimize(){ if(g_hwnd) ShowWindow(g_hwnd,SW_MINIMIZE); }
void PlatformClose(){ if(g_hwnd) DestroyWindow(g_hwnd); }
void PlatformRestoreAndFocus(){ if(g_hwnd){ if(!IsWindowVisible(g_hwnd)) ShowWindow(g_hwnd,SW_SHOW); if(IsIconic(g_hwnd)) ShowWindow(g_hwnd,SW_RESTORE); SetForegroundWindow(g_hwnd); InvalidateRect(g_hwnd,NULL,FALSE); } }

// ---- bandeja (tray) + segundo plano + teclas de midia ----------------------
#define WM_TRAY (WM_APP+2)
#ifndef HSHELL_APPCOMMAND
#define HSHELL_APPCOMMAND 12
#endif
static NOTIFYICONDATAW g_nid; static bool g_trayOn=false; static UINT g_shellHookMsg=0; static HINSTANCE g_hInst=nullptr;
static void TrayAdd(HWND hwnd,HINSTANCE hInst){
    if(g_trayOn) return;
    memset(&g_nid,0,sizeof(g_nid)); g_nid.cbSize=sizeof(g_nid); g_nid.hWnd=hwnd; g_nid.uID=1;
    g_nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; g_nid.uCallbackMessage=WM_TRAY;
    g_nid.hIcon=(HICON)LoadImageW(hInst,MAKEINTRESOURCEW(IDI_APPICON),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
    if(!g_nid.hIcon) g_nid.hIcon=LoadIconW(hInst,MAKEINTRESOURCEW(IDI_APPICON));
    wcsncpy(g_nid.szTip,L"Remix Player",127); g_nid.szTip[127]=0;
    g_trayOn=Shell_NotifyIconW(NIM_ADD,&g_nid)!=FALSE;
}
static void TrayRemove(){ if(g_trayOn){ Shell_NotifyIconW(NIM_DELETE,&g_nid); g_trayOn=false; } }
static void TrayBalloon(const wchar_t* title,const wchar_t* text){
    if(!g_trayOn) return;
    NOTIFYICONDATAW n=g_nid; n.uFlags=NIF_INFO; wcsncpy(n.szInfoTitle,title,63); n.szInfoTitle[63]=0; wcsncpy(n.szInfo,text,255); n.szInfo[255]=0; n.dwInfoFlags=NIIF_INFO|NIIF_NOSOUND; n.uTimeout=6000;
    Shell_NotifyIconW(NIM_MODIFY,&n);
}
static void TrayMenu(HWND hwnd){
    HMENU m=CreatePopupMenu(); if(!m) return;
    bool hidden=!IsWindowVisible(hwnd);
    AppendMenuW(m,MF_STRING,1,hidden?L"Mostrar o Remix":L"Esconder (continua tocando)");
    AppendMenuW(m,MF_SEPARATOR,0,NULL);
    AppendMenuW(m,MF_STRING,2,g_player.playing?L"Pausar":L"Tocar");
    AppendMenuW(m,MF_STRING,3,L"Anterior");
    AppendMenuW(m,MF_STRING,4,L"Próxima");
    AppendMenuW(m,MF_SEPARATOR,0,NULL);
    AppendMenuW(m,MF_STRING,5,L"Sair do Remix");
    POINT p; GetCursorPos(&p); SetForegroundWindow(hwnd);
    int id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_NONOTIFY,p.x,p.y,0,hwnd,NULL);
    PostMessageW(hwnd,WM_NULL,0,0); DestroyMenu(m);
    switch(id){
    case 1: if(hidden) ShowFromBackground(); else HideToBackground(false); break;
    case 2: RemotePlayPause(); break;
    case 3: PrevOrRestart(); break;
    case 4: NextTrack(); break;
    case 5: QuitApp(); break;
    }
    if(g_hwnd) InvalidateRect(g_hwnd,NULL,FALSE);
}
// teclas de midia: WM_APPCOMMAND (janela ativa) e gancho do shell (em 2o plano)
static bool HandleAppCommand(int cmd){
    if(!g_cfg.sysMedia) return false;   // controles do sistema desligados nas configuracoes
    switch(cmd){
    case APPCOMMAND_MEDIA_PLAY_PAUSE: RemotePlayPause(); return true;
    case APPCOMMAND_MEDIA_PLAY: RemotePlay(); return true;
    case APPCOMMAND_MEDIA_PAUSE: RemotePause(); return true;
    case APPCOMMAND_MEDIA_STOP: RemoteStop(); return true;
    case APPCOMMAND_MEDIA_NEXTTRACK: NextTrack(); return true;
    case APPCOMMAND_MEDIA_PREVIOUSTRACK: PrevOrRestart(); return true;
    }
    return false;
}
void PlatformSetWindowSize(int w,int h){ if(!g_hwnd) return; RECT wr; GetWindowRect(g_hwnd,&wr); SetWindowPos(g_hwnd,NULL,wr.left,wr.top,w,h,SWP_NOZORDER|SWP_NOACTIVATE); RECT rc; GetClientRect(g_hwnd,&rc); g_winW=rc.right; g_winH=rc.bottom; BuildLayout(); }
void PlatformHide(){
    if(!g_hwnd) return;
    ShowWindow(g_hwnd,SW_HIDE);
    static bool told=false;
    if(!told){ told=true; TrayBalloon(L"Remix continua tocando",L"A música segue em segundo plano. Clique no ícone do Remix aqui na bandeja para abrir, ou botão direito para sair."); }
}
static Bitmap* g_lastFrame=nullptr;
bool PlatformScreenshot(const std::wstring& png){
    if(!g_lastFrame) return false;
    CLSID clsid; if(FindEncoderClsid(L"image/png",&clsid)<0) return false;
    return g_lastFrame->Save(png.c_str(),&clsid,nullptr)==Ok;
}

// ------------------------------------------------------------ pintura ------
static void OnPaint(HDC hdc){
    RECT rc;GetClientRect(g_hwnd,&rc); int w=rc.right,h=rc.bottom; if(w<=0||h<=0) return;
    if(!g_lastFrame||(int)g_lastFrame->GetWidth()!=w||(int)g_lastFrame->GetHeight()!=h){ delete g_lastFrame; g_lastFrame=new Bitmap(w,h,PixelFormat32bppARGB); }
    Graphics g(g_lastFrame); g.SetSmoothingMode(SmoothingModeAntiAlias); g.SetTextRenderingHint(TextRenderingHintAntiAlias);
    if(g_showSplash){DrawSplash(g,w,h);}
    else {
        if(g_cfg.displayMode==L"vertical")DrawVertical(g,w,h);else DrawNormal(g,w,h);
        if(g_imgMenuOpen)DrawImgMenu(g,w,h);
        if(OU().open)DrawOnline(g,w,h);   // os menus (ex.: escolher playlist) ficam por cima
        if(g_folderMenuOpen)DrawFolderMenu(g,w,h);
        if(g_ctxOpen)DrawCtxMenu(g);
        if(g_editArtist)DrawArtistEditor(g,w,h);
        if(WP().open){LayoutWebPick(w,h);DrawWebPick(g,w,h);}
        if(g_confirmOpen)DrawConfirm(g,w,h);
        DrawActivity(g,w,h);
        DrawStatusToast(g,w,h);
    }
    Graphics s(hdc); s.DrawImage(g_lastFrame,0,0);
}
// VK_* -> KeyCode (app_keys.h) e volta (para RegisterHotKey)
static int MapVkKc(WPARAM vk){
    if((vk>='A'&&vk<='Z')||(vk>='0'&&vk<='9')) return (int)vk;
    if(vk>=VK_F1&&vk<=VK_F12) return KC_F1+(int)(vk-VK_F1);
    if(vk>=VK_NUMPAD0&&vk<=VK_NUMPAD9) return KC_KP0+(int)(vk-VK_NUMPAD0);
    switch(vk){
    case VK_SPACE: return KC_SPACE; case VK_LEFT: return KC_LEFT; case VK_RIGHT: return KC_RIGHT; case VK_UP: return KC_UP; case VK_DOWN: return KC_DOWN;
    case VK_RETURN: return KC_ENTER; case VK_ESCAPE: return KC_ESC; case VK_BACK: return KC_BACKSPACE; case VK_DELETE: return KC_DELETE; case VK_TAB: return KC_TAB;
    case VK_INSERT: return KC_INSERT; case VK_HOME: return KC_HOME; case VK_END: return KC_END; case VK_PRIOR: return KC_PAGEUP; case VK_NEXT: return KC_PAGEDOWN;
    case VK_OEM_PLUS: return KC_EQUAL; case VK_OEM_MINUS: return KC_MINUS; case VK_OEM_COMMA: return KC_COMMA; case VK_OEM_PERIOD: return KC_PERIOD;
    case VK_OEM_1: return KC_SEMICOLON; case VK_OEM_7: return KC_APOSTROPHE; case VK_OEM_2: return KC_SLASH; case VK_OEM_5: return KC_BACKSLASH; case VK_OEM_4: return KC_LBRACKET; case VK_OEM_6: return KC_RBRACKET; case VK_OEM_3: return KC_GRAVE;
    case VK_ADD: return KC_KPADD; case VK_SUBTRACT: return KC_KPSUB; case VK_MULTIPLY: return KC_KPMUL; case VK_DIVIDE: return KC_KPDIV; case VK_DECIMAL: return KC_KPDECIMAL;
    case VK_MEDIA_PLAY_PAUSE: return KC_MEDIA_PLAY; case VK_MEDIA_NEXT_TRACK: return KC_MEDIA_NEXT; case VK_MEDIA_PREV_TRACK: return KC_MEDIA_PREV; case VK_MEDIA_STOP: return KC_MEDIA_STOP;
    case VK_VOLUME_UP: return KC_VOL_UP; case VK_VOLUME_DOWN: return KC_VOL_DOWN; case VK_VOLUME_MUTE: return KC_VOL_MUTE;
    case VK_PAUSE: return KC_PAUSE; case VK_SNAPSHOT: return KC_PRINT; case VK_SCROLL: return KC_SCROLL; case VK_NUMLOCK: return KC_NUMLOCK; case VK_CAPITAL: return KC_CAPSLOCK;
    default: return KC_NONE;
    }
}
static UINT KcToVk(int kc){
    if((kc>='A'&&kc<='Z')||(kc>='0'&&kc<='9')) return (UINT)kc;
    if(kc>=KC_F1&&kc<=KC_F12) return VK_F1+(UINT)(kc-KC_F1);
    if(kc>=KC_KP0&&kc<=KC_KP9) return VK_NUMPAD0+(UINT)(kc-KC_KP0);
    switch(kc){
    case KC_SPACE: return VK_SPACE; case KC_LEFT: return VK_LEFT; case KC_RIGHT: return VK_RIGHT; case KC_UP: return VK_UP; case KC_DOWN: return VK_DOWN;
    case KC_ENTER: return VK_RETURN; case KC_ESC: return VK_ESCAPE; case KC_BACKSPACE: return VK_BACK; case KC_DELETE: return VK_DELETE; case KC_TAB: return VK_TAB;
    case KC_INSERT: return VK_INSERT; case KC_HOME: return VK_HOME; case KC_END: return VK_END; case KC_PAGEUP: return VK_PRIOR; case KC_PAGEDOWN: return VK_NEXT;
    case KC_EQUAL: return VK_OEM_PLUS; case KC_MINUS: return VK_OEM_MINUS; case KC_COMMA: return VK_OEM_COMMA; case KC_PERIOD: return VK_OEM_PERIOD;
    case KC_SEMICOLON: return VK_OEM_1; case KC_APOSTROPHE: return VK_OEM_7; case KC_SLASH: return VK_OEM_2; case KC_BACKSLASH: return VK_OEM_5; case KC_LBRACKET: return VK_OEM_4; case KC_RBRACKET: return VK_OEM_6; case KC_GRAVE: return VK_OEM_3;
    case KC_KPADD: return VK_ADD; case KC_KPSUB: return VK_SUBTRACT; case KC_KPMUL: return VK_MULTIPLY; case KC_KPDIV: return VK_DIVIDE; case KC_KPDECIMAL: return VK_DECIMAL; case KC_KPENTER: return VK_RETURN;
    case KC_MEDIA_PLAY: return VK_MEDIA_PLAY_PAUSE; case KC_MEDIA_NEXT: return VK_MEDIA_NEXT_TRACK; case KC_MEDIA_PREV: return VK_MEDIA_PREV_TRACK; case KC_MEDIA_STOP: return VK_MEDIA_STOP;
    case KC_VOL_UP: return VK_VOLUME_UP; case KC_VOL_DOWN: return VK_VOLUME_DOWN; case KC_VOL_MUTE: return VK_VOLUME_MUTE;
    case KC_PAUSE: return VK_PAUSE; case KC_PRINT: return VK_SNAPSHOT; case KC_SCROLL: return VK_SCROLL; case KC_NUMLOCK: return VK_NUMLOCK; case KC_CAPSLOCK: return VK_CAPITAL;
    default: return 0;
    }
}
#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif
// atalhos GLOBAIS: RegisterHotKey (o Windows entrega WM_HOTKEY mesmo com outro programa na frente)
void PlatformUpdateGlobalHotkeys(){
    if(!g_hwnd) return;
    for(int a=0;a<HK_COUNT;a++) UnregisterHotKey(g_hwnd,100+a);
    for(int a=0;a<HK_COUNT;a++){
        const Hotkey& h=g_cfg.hk[a]; if(!h.global||!h.key) continue;
        UINT vk=KcToVk(h.key); if(!vk) continue;
        UINT m=((h.mods&KM_CTRL)?MOD_CONTROL:0)|((h.mods&KM_SHIFT)?MOD_SHIFT:0)|((h.mods&KM_ALT)?MOD_ALT:0)|MOD_NOREPEAT;
        RegisterHotKey(g_hwnd,100+a,m,vk);
    }
}
static ULONGLONG g_lastTick=0;
static bool g_editingText(){ return g_editArtist||(WP().open&&WP().editing)||(OU().open&&OU().editing); }

LRESULT CALLBACK WndProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    if(g_shellHookMsg&&msg==g_shellHookMsg&&wp==HSHELL_APPCOMMAND){ if(HandleAppCommand(GET_APPCOMMAND_LPARAM(lp))){ InvalidateRect(hwnd,NULL,FALSE); return TRUE; } }
    switch(msg){
    case WM_CLOSE: RequestClose(); return 0;   // X / Alt+F4: some e segue tocando (ou sai)
    case WM_TRAY:{ UINT e=(UINT)LOWORD(lp);
        if(e==WM_LBUTTONUP||e==WM_LBUTTONDBLCLK){ if(!IsWindowVisible(hwnd)||IsIconic(hwnd)) ShowFromBackground(); else SetForegroundWindow(hwnd); }
        else if(e==WM_RBUTTONUP||e==WM_CONTEXTMENU) TrayMenu(hwnd);
        return 0; }
    case WM_APPCOMMAND: if(HandleAppCommand(GET_APPCOMMAND_LPARAM(lp))){ InvalidateRect(hwnd,NULL,FALSE); return TRUE; } break;
    case WM_CREATE: g_lastTick=GetTickCount64(); SetTimer(hwnd,1,g_cfg.perfMode?66:40,NULL); return 0;
    case WM_SIZE:{ RECT rc;GetClientRect(hwnd,&rc); g_winW=rc.right; g_winH=rc.bottom; BuildLayout();
        if(g_cfg.displayMode!=L"vertical"&&wp!=SIZE_MINIMIZED&&g_winW>=720&&g_winH>=540){ RECT wr; GetWindowRect(hwnd,&wr); g_cfg.winW=wr.right-wr.left; g_cfg.winH=wr.bottom-wr.top; }
        InvalidateRect(hwnd,NULL,FALSE); return 0; }
    case WM_GETMINMAXINFO:{MINMAXINFO*m=(MINMAXINFO*)lp;if(g_cfg.displayMode==L"vertical"){m->ptMinTrackSize.x=340;m->ptMinTrackSize.y=560;}else{m->ptMinTrackSize.x=720;m->ptMinTrackSize.y=540;}return 0;}
    case WM_COPYDATA:{COPYDATASTRUCT*cds=(COPYDATASTRUCT*)lp;if(cds&&cds->lpData&&cds->cbData){std::wstring cmd((wchar_t*)cds->lpData,(cds->cbData/sizeof(wchar_t))-1);HandleCommand(cmd);return 1;}break;}
    case WM_NCCALCSIZE: return 0;   // janela sem borda: a area cliente e a janela inteira (wParam TRUE e FALSE; senao sobra moldura)
    case WM_NCHITTEST:{POINT p={(SHORT)LOWORD(lp),(SHORT)HIWORD(lp)};ScreenToClient(hwnd,&p);RECT r;GetClientRect(hwnd,&r);const int b=8;
        if(p.x<b&&p.y<b)return HTTOPLEFT;if(p.x>=r.right-b&&p.y<b)return HTTOPRIGHT;if(p.x<b&&p.y>=r.bottom-b)return HTBOTTOMLEFT;if(p.x>=r.right-b&&p.y>=r.bottom-b)return HTBOTTOMRIGHT;if(p.x<b)return HTLEFT;if(p.x>=r.right-b)return HTRIGHT;if(p.y<b)return HTTOP;if(p.y>=r.bottom-b)return HTBOTTOM;
        if(!g_showSplash&&!AnyOverlay()&&!g_showSettings&&PtIn(R_titlebar,p.x,p.y)&&HitTest(p.x,p.y)<0)return HTCAPTION;return HTCLIENT;}
    case WM_LBUTTONDOWN:{ int x=(SHORT)LOWORD(lp),y=(SHORT)HIWORD(lp); OnLButtonDown(x,y); if(g_dragSeek!=-1) SetCapture(hwnd); InvalidateRect(hwnd,NULL,FALSE); return 0; }
    case WM_RBUTTONDOWN:{ int x=(SHORT)LOWORD(lp),y=(SHORT)HIWORD(lp); OnRButtonDown(x,y); InvalidateRect(hwnd,NULL,FALSE); return 0; }
    case WM_MOUSEMOVE:{ if((wp&MK_LBUTTON)&&g_dragSeek!=-1){ OnMouseDrag((SHORT)LOWORD(lp)); InvalidateRect(hwnd,NULL,FALSE); } return 0; }
    case WM_LBUTTONUP: if(g_dragSeek!=-1){ OnLButtonUp(); ReleaseCapture(); } return 0;
    case WM_MOUSEWHEEL:{ int d=GET_WHEEL_DELTA_WPARAM(wp); OnWheel(d/120); InvalidateRect(hwnd,NULL,FALSE); return 0; }
    case WM_CHAR:{ wchar_t c=(wchar_t)wp;
        if(c>=32&&c!=127){ OnChar((int)c); InvalidateRect(hwnd,NULL,FALSE); return 0; }
        break; }
    case WM_KEYDOWN: case WM_SYSKEYDOWN:{
        int kc=MapVkKc(wp);
        if(kc){ int mods=((GetKeyState(VK_CONTROL)&0x8000)?KM_CTRL:0)|((GetKeyState(VK_SHIFT)&0x8000)?KM_SHIFT:0)|((GetKeyState(VK_MENU)&0x8000)?KM_ALT:0); OnKeyEvent(kc,mods); InvalidateRect(hwnd,NULL,FALSE); }
        if(msg==WM_SYSKEYDOWN) break;   // Alt+F4 etc. seguem para o DefWindowProc
        return 0; }
    case WM_HOTKEY:{ int a=(int)wp-100; if(a>=0&&a<HK_COUNT){ RunHotkeyAction(a); InvalidateRect(hwnd,NULL,FALSE); } return 0; }
    case WM_TIMER:{ if(wp==2){ KillTimer(hwnd,2); DestroyWindow(hwnd); return 0; }
        ULONGLONG now=GetTickCount64(); float dt=(float)(now-g_lastTick)/1000.f; g_lastTick=now; if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.04f;
        { RECT rc; GetClientRect(hwnd,&rc); if(rc.right>0&&rc.bottom>0&&(rc.right!=g_winW||rc.bottom!=g_winH)){ g_winW=rc.right; g_winH=rc.bottom; BuildLayout(); } }   // layout sempre no tamanho real
        DrainEvents(); Tick(dt); RunTimedActions(now-g_t0);
        static bool lastPerf=g_cfg.perfMode; if(lastPerf!=g_cfg.perfMode){ lastPerf=g_cfg.perfMode; SetTimer(hwnd,1,lastPerf?66:40,NULL); }
        if(g_cfg.sysMedia&&!g_trayOn) TrayAdd(hwnd,g_hInst); else if(!g_cfg.sysMedia&&g_trayOn) TrayRemove();
        InvalidateRect(hwnd,NULL,FALSE); return 0; }
    case WM_APP_EVENT: DrainEvents(); InvalidateRect(hwnd,NULL,FALSE); return 0;
    case WM_PAINT:{PAINTSTRUCT ps;HDC h=BeginPaint(hwnd,&ps);OnPaint(h);EndPaint(hwnd,&ps);return 0;}
    case WM_ERASEBKGND:return 1;
    case WM_DESTROY: KillTimer(hwnd,1); for(int a=0;a<HK_COUNT;a++) UnregisterHotKey(hwnd,100+a); TrayRemove(); if(g_shellHookMsg) DeregisterShellHookWindow(hwnd); CoreShutdown(); if(g_singleMutex){ReleaseMutex(g_singleMutex);CloseHandle(g_singleMutex);g_singleMutex=nullptr;} PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE, PWSTR, int nCmdShow){
    CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::wstring cmd,cmdName;
    bool noSplash=false; int exitAfter=0;
    for(int i=1;i<argc;i++){
        std::wstring a=argv[i];
        if((_wcsicmp(a.c_str(),L"--play-file")==0||_wcsicmp(a.c_str(),L"--play")==0)&&i+1<argc){cmd=argv[++i];}
        else if(_wcsicmp(a.c_str(),L"--after")==0&&i+1<argc){ std::string v=WideToUtf8(argv[++i]); size_t c=v.find(':'); if(c!=std::string::npos) g_timed.push_back({atoi(v.substr(0,c).c_str()),v.substr(c+1)}); }
        else if(_wcsicmp(a.c_str(),L"--home")==0&&i+1<argc){ std::wstring e=std::wstring(L"REMIX_HOME=")+argv[++i]; _wputenv(e.c_str()); }   // testes: pasta de config/biblioteca
        else if(_wcsicmp(a.c_str(),L"--no-splash")==0){ noSplash=true; }
        else if(_wcsicmp(a.c_str(),L"--exit-after")==0&&i+1<argc){ exitAfter=_wtoi(argv[++i]); }
        else if(_wcsicmp(a.c_str(),L"--cmd")==0&&i+1<argc){ cmdName=argv[++i]; }
        else if(!a.empty()&&a[0]!=L'-') cmd=a;
    }
    {   // instancia unica por pasta do app (--home/REMIX_HOME separa testes e copias portateis)
        std::wstring mname=L"RemixPlayer.SingleInstance";
        if(const wchar_t* hm=_wgetenv(L"REMIX_HOME")){ if(*hm){ unsigned long long x=1469598103934665603ULL; for(const wchar_t* p=hm;*p;++p){ x^=(unsigned long long)*p; x*=1099511628211ULL; } wchar_t b[24]; swprintf(b,24,L".%08llx",x&0xffffffffULL); mname+=b; } }
        g_singleMutex=CreateMutexW(NULL,TRUE,mname.c_str());
    }
    bool already=GetLastError()==ERROR_ALREADY_EXISTS;
    if(already){ SendToExisting(cmdName.empty()?L"PLAYFILE|"+cmd:L"CMD|"+cmdName); if(argv)LocalFree(argv); CloseHandle(g_singleMutex); g_singleMutex=nullptr; CoUninitialize(); return 0; }
    if(argv)LocalFree(argv);
    GdiplusStartupInput gi;GdiplusStartup(&g_gdiToken,&gi,NULL);
    g_cfg.Load();
    CleanupCacheWin(72);
    CoreInit();
    g_customChrome=true;   // janela sem borda: fechar/minimizar no cabecalho
    std::thread([]{int n=MigrateOversizedCovers();if(n>0)AppPost(EV_THUMBS_INVALIDATE,L"",n);}).detach();
    LoadFolderAndPlaylist();PlatformWatchStart(g_cfg.musicFolder);if(g_current>=0)PlayIndex(g_current,false);
    RestoreOpenPlaylist();
    if(!cmd.empty())g_pendingPlay=cmd;
    if(noSplash){ g_showSplash=false; }
    else {
        std::wstring splash=Config::Join(Config::Join(Config::AssetDir(),L"branding"),L"splash.png");g_splashImg=new Image(splash.c_str());if(g_splashImg->GetLastStatus()!=Ok){delete g_splashImg;g_splashImg=nullptr;}g_splashStart=GetTickCount64();
        Player::PlayOneShot(Config::Join(Config::Join(Config::AssetDir(),L"branding"),L"open.wav"));
    }
    WNDCLASSW wc={0};wc.lpfnWndProc=WndProc;wc.hInstance=hInst;wc.lpszClassName=L"MusicPlayerRemixWnd";wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hIcon=LoadIconW(hInst,MAKEINTRESOURCEW(IDI_APPICON));wc.style=CS_HREDRAW|CS_VREDRAW;RegisterClassW(&wc);
    DWORD style=WS_POPUP|WS_THICKFRAME|WS_MINIMIZEBOX|WS_VISIBLE;
    int startW,startH; if(g_cfg.displayMode==L"vertical"){startW=460;startH=700;ClampToWork(startW,startH);} else NormalSize(startW,startH);
    g_hwnd=CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,L"Remix Player",style,CW_USEDEFAULT,CW_USEDEFAULT,startW,startH,NULL,NULL,hInst,NULL);
    { COLORREF c=0xFFFFFFFEu; DwmSetWindowAttribute(g_hwnd,34,&c,sizeof(c)); }   // Win11: sem borda colorida (34 = DWMWA_BORDER_COLOR, valor = DWMWA_COLOR_NONE); no Win10 so retorna erro
    SetWindowPos(g_hwnd,NULL,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);   // recalcula a area cliente ja sem moldura
    { RECT rc;GetClientRect(g_hwnd,&rc); g_winW=rc.right; g_winH=rc.bottom; }
    g_hInst=hInst; if(g_cfg.sysMedia) TrayAdd(g_hwnd,hInst);
    PlatformUpdateGlobalHotkeys();
    if(!cmdName.empty()) HandleCommand(L"CMD|"+cmdName);
    g_shellHookMsg=RegisterWindowMessageW(L"SHELLHOOK"); if(!RegisterShellHookWindow(g_hwnd)) g_shellHookMsg=0;
    BuildLayout();if(g_cfg.musicFolder.empty())StartAutoScan();ShowWindow(g_hwnd,nCmdShow? nCmdShow:SW_SHOW);UpdateWindow(g_hwnd);
    if(exitAfter>0) SetTimer(g_hwnd,2,(UINT)exitAfter,NULL);   // testes: fecha sozinho
    if(!g_pendingPlay.empty())HandleCommand(L"PLAYFILE|"+g_pendingPlay);
    MSG msg;while(GetMessageW(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    if(g_coverImg)delete g_coverImg;if(g_splashImg)delete g_splashImg;PlatformClearThumbs();delete g_lastFrame;g_lastFrame=nullptr;
    GdiplusShutdown(g_gdiToken);CoUninitialize();return 0;
}
