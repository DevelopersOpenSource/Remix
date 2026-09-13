#pragma once
// Busca de capa na internet (Windows): WinHTTP + miniaturas GDI+.
// A parte comum (URLs, parse) esta em app_web_common.h.
#include "app_core.h"
#include "app_web_common.h"
#include <winhttp.h>
#include <gdiplus.h>

static std::string HttpGetBytes(const std::wstring& host,const std::wstring& path,std::wstring* outType=nullptr,DWORD* status=nullptr){
    std::string out;
    HINTERNET ses=WinHttpOpen(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) RemixPlayer/1.2",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
    if(!ses)return out;
    DWORD to=15000;
    WinHttpSetOption(ses,WINHTTP_OPTION_CONNECT_TIMEOUT,&to,sizeof(to));
    WinHttpSetOption(ses,WINHTTP_OPTION_SEND_TIMEOUT,&to,sizeof(to));
    WinHttpSetOption(ses,WINHTTP_OPTION_RECEIVE_TIMEOUT,&to,sizeof(to));
    HINTERNET con=WinHttpConnect(ses,host.c_str(),INTERNET_DEFAULT_HTTPS_PORT,0);
    if(con){
        HINTERNET req=WinHttpOpenRequest(con,L"GET",path.c_str(),NULL,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);
        if(req){
            std::wstring hdr=L"Accept-Language: pt-BR,pt;q=0.9,en;q=0.8\r\n";
            WinHttpAddRequestHeaders(req,hdr.c_str(),(DWORD)-1,WINHTTP_ADDREQ_FLAG_ADD);
            if(WinHttpSendRequest(req,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(req,NULL)){
                if(status){DWORD st=0,sz=sizeof(st);WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,NULL,&st,&sz,NULL);*status=st;}
                if(outType){wchar_t b[256]={0};DWORD sz=sizeof(b);if(WinHttpQueryHeaders(req,WINHTTP_QUERY_CONTENT_TYPE,NULL,b,&sz,NULL))*outType=b;}
                DWORD avail=0;
                while(WinHttpQueryDataAvailable(req,&avail)&&avail>0){
                    std::vector<char> buf(avail);DWORD rd=0;
                    if(!WinHttpReadData(req,buf.data(),avail,&rd)||rd==0)break;
                    out.append(buf.data(),rd);
                    if(out.size()>(size_t)40*1024*1024)break;
                }
            }
            WinHttpCloseHandle(req);
        }
        WinHttpCloseHandle(con);
    }
    WinHttpCloseHandle(ses);
    return out;
}
static std::wstring TempFileW(const std::wstring& name){ wchar_t tp[MAX_PATH]; GetTempPathW(MAX_PATH,tp); return std::wstring(tp)+name; }
static bool WriteBytesW(const std::wstring& f,const std::string& data){
    HANDLE fh=CreateFileW(f.c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(fh==INVALID_HANDLE_VALUE) return false;
    DWORD wr=0; BOOL ok=WriteFile(fh,data.data(),(DWORD)data.size(),&wr,NULL); CloseHandle(fh); return ok&&wr==data.size();
}
bool PlatformHttpGet(const std::string& url,std::string& body){
    std::wstring host,path; SplitUrl(Utf8ToWide(url),host,path);
    if(host.empty()) return false;
    DWORD st=0; body=HttpGetBytes(host,path,nullptr,&st);
    return !body.empty()&&(st==0||(st>=200&&st<400));
}
static void ClearWebResultsPlatform(){
    WebPick& wb=WP();
    for(auto&r:wb.res){ if(r.img){ delete (Gdiplus::Image*)r.img; r.img=nullptr; } r.cpu=nullptr; }
    wb.res.clear(); wb.cells.clear();
}
static void WebSearchAsync(std::wstring q){
    WebPick& wb=WP();
    unsigned gen=(unsigned)++wb.gen;
    wb.searching=true;
    { std::lock_guard<std::mutex> lk(wb.m); ClearWebResultsPlatform(); wb.sel=-1; wb.scroll=0; wb.status=L"Buscando imagens..."; }
    PlatformRedraw();
    std::thread([q,gen](){
        WebPick& wb=WP();
        std::string html=HttpGetBytes(L"www.bing.com",WebSearchPath(q));
        if(gen!=(unsigned)wb.gen.load())return;
        auto pr=ParseImageSearch(html);
        {
            std::lock_guard<std::mutex> lk(wb.m);
            if(gen!=(unsigned)wb.gen.load())return;
            for(auto&p:pr)wb.res.push_back(WebRes{p.first,p.second,nullptr,nullptr});
            wb.status=wb.res.empty()?L"Nada encontrado. Tente outras palavras.":L"Carregando miniaturas...";
        }
        AppPost(EV_REDRAW);
        for(size_t i=0;i<pr.size();++i){
            if(gen!=(unsigned)wb.gen.load())return;
            if(pr[i].second.empty())continue;
            std::wstring host,path2;SplitUrl(pr[i].second,host,path2);
            std::string img=HttpGetBytes(host,path2);
            if(img.size()>800){
                std::wstring f=TempFileW(L"remix_thumb_"+std::to_wstring(GetCurrentThreadId())+L"_"+std::to_wstring(i)+L".img");
                if(WriteBytesW(f,img)){
                    Gdiplus::Image* im=new Gdiplus::Image(f.c_str());
                    // forca a decodificacao antes de apagar o arquivo
                    UINT w=im->GetWidth(); (void)w;
                    DeleteFileW(f.c_str());
                    if(im->GetLastStatus()==Gdiplus::Ok){
                        std::lock_guard<std::mutex> lk(wb.m);
                        if(gen==(unsigned)wb.gen.load()&&i<wb.res.size()&&!wb.res[i].img){wb.res[i].img=im;im=nullptr;}
                    }
                    if(im)delete im;
                }
            }
            AppPost(EV_REDRAW);
        }
        { std::lock_guard<std::mutex> lk(wb.m); if(gen==(unsigned)wb.gen.load()){wb.status=L"Clique numa imagem e aperte USAR ESSA.";wb.searching=false;} }
        AppPost(EV_REDRAW);
    }).detach();
}
static void WebDownloadSelectedAsync(){
    WebPick& wb=WP();
    std::wstring url;
    {
        std::lock_guard<std::mutex> lk(wb.m);
        if(wb.sel<0||wb.sel>=(int)wb.res.size()||wb.downloading)return;
        url=wb.res[(size_t)wb.sel].murl;
        wb.downloading=true;
        wb.status=L"Baixando imagem em tamanho cheio...";
    }
    PlatformRedraw();
    std::thread([url](){
        std::wstring host,path;SplitUrl(url,host,path);
        std::wstring ctype;std::string data=HttpGetBytes(host,path,&ctype);
        auto&W=WDS();
        std::lock_guard<std::mutex> lk2(W.m);
        W.ok=false;W.path.clear();
        if(data.size()>3000){
            std::wstring ext=L".jpg";
            const char* t=ImageTypeFromBytes(data);
            if(t)ext=Utf8ToWide(t);
            else if(ctype.find(L"png")!=std::wstring::npos)ext=L".png";
            else if(ctype.find(L"webp")!=std::wstring::npos)ext=L".webp";
            else if(ctype.find(L"bmp")!=std::wstring::npos)ext=L".bmp";
            std::wstring f=TempFileW(L"remix_cover_full"+ext);
            if(WriteBytesW(f,data)){ Gdiplus::Image chk(f.c_str()); if(chk.GetLastStatus()==Gdiplus::Ok){W.ok=true;W.path=f;} }
        }
        AppPost(EV_WEB_DOWNLOAD_DONE);
    }).detach();
}
static void ConsumeWebDownload(){
    WebPick& wb=WP();
    auto&W=WDS();
    std::lock_guard<std::mutex> lk2(W.m);
    wb.downloading=false;
    if(W.ok&&!W.path.empty()){
        ApplyCoverPick(wb.track,W.path);
        DeleteFileW(W.path.c_str());W.path.clear();W.ok=false;
        wb.open=false;wb.editing=false;
    } else {
        std::lock_guard<std::mutex> lkw(wb.m);
        wb.status=L"Falha ao baixar. Tente outra imagem.";
    }
}
