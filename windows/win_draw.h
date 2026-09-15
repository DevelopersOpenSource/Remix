#pragma once
// Casca Windows: desenho com GDI+ (mesma aparencia do Linux/raylib). Toda a
// logica e geometria vem de app_core.h / app_layout.h. Incluido so pelo main.cpp.
#include "app_core.h"
#include "app_layout.h"
#include <gdiplus.h>
using namespace Gdiplus;

static Image* g_coverImg = nullptr;
static Image* g_splashImg = nullptr;
static std::map<std::wstring, Image*> g_thumbCache;

static Color ToGdi(COLORREF c, BYTE a=255){ return Color(a, GetRValue(c), GetGValue(c), GetBValue(c)); }
static RectF RF(const RECT& r){ return RectF((REAL)r.left,(REAL)r.top,(REAL)(r.right-r.left),(REAL)(r.bottom-r.top)); }
static RectFC RC(const RectF& r){ return RectFC{r.X,r.Y,r.Width,r.Height}; }
static ULONGLONG g_t0 = GetTickCount64();
static DWORD NowMs(){ return (DWORD)(GetTickCount64()-g_t0); }

static void DrawRoundRect(Graphics& g, Rect r, int radius, Brush* fill, Pen* pen){
    GraphicsPath path; int d=std::max(1,radius*2);
    path.AddArc(r.X,r.Y,d,d,180,90); path.AddArc(r.X+r.Width-d,r.Y,d,d,270,90);
    path.AddArc(r.X+r.Width-d,r.Y+r.Height-d,d,d,0,90); path.AddArc(r.X,r.Y+r.Height-d,d,d,90,90);
    path.CloseFigure();
    if(fill) g.FillPath(fill,&path); if(pen) g.DrawPath(pen,&path);
}
static void DrawRoundRect(Graphics& g, const RectF& rf, int radius, Brush* fill, Pen* pen){
    DrawRoundRect(g,Rect((int)rf.X,(int)rf.Y,(int)(rf.X+rf.Width)-(int)rf.X,(int)(rf.Y+rf.Height)-(int)rf.Y),radius,fill,pen);
}
// Familias/fontes (criadas depois do GdiplusStartup).
// Familias de fonte com fallback: no Windows normal "Segoe UI" sempre existe;
// em Wine/Proton ou instalacoes enxutas cai para Arial/Tahoma/Liberation/DejaVu.
static FontFamily* PickFamily(const wchar_t* const* names,size_t n){
    for(size_t i=0;i<n;i++){ FontFamily* f=new FontFamily(names[i]); if(f->GetLastStatus()==Ok&&f->IsAvailable()) return f; delete f; }
    return FontFamily::GenericSansSerif()->Clone();
}
static FontFamily* UiFam(){ static FontFamily* f=nullptr; if(!f){ static const wchar_t* const n[]={L"Segoe UI",L"Arial",L"Tahoma",L"Liberation Sans",L"DejaVu Sans",L"Noto Sans"}; f=PickFamily(n,6);} return f; }
static FontFamily* SymFam(){ static FontFamily* f=nullptr; if(!f){ static const wchar_t* const n[]={L"Segoe UI Symbol",L"Segoe UI",L"DejaVu Sans",L"Noto Sans Symbols",L"Arial Unicode MS",L"Arial"}; f=PickFamily(n,6);} return f; }
static const Font* SymFont(float px){
    struct Ent{float px;Font* f;}; static std::vector<Ent> v;
    for(auto&e:v) if(e.px==px) return e.f;
    Font* nf=new Font(SymFam(),px,FontStyleRegular,UnitPixel); v.push_back({px,nf}); return nf;
}
static const Font* UiFont(float px,bool bold){
    struct Ent{float px;bool b;Font* f;}; static std::vector<Ent> v;
    for(auto&e:v) if(e.px==px&&e.b==bold) return e.f;
    Font* nf=new Font(UiFam(),px,bold?FontStyleBold:FontStyleRegular,UnitPixel); v.push_back({px,bold,nf}); return nf;
}
static void TextAt(Graphics& g,const std::wstring& s,float x,float y,float px,const Brush* b,bool bold=false){ g.DrawString(s.c_str(),-1,UiFont(px,bold),PointF(x,y),b); }
static void TextCenter(Graphics& g,const std::wstring& s,const RectF& rc,float px,const Brush* b,bool bold=false){
    StringFormat cf; cf.SetAlignment(StringAlignmentCenter); cf.SetLineAlignment(StringAlignmentCenter); cf.SetFormatFlags(StringFormatFlagsNoWrap); cf.SetTrimming(StringTrimmingEllipsisCharacter);
    g.DrawString(s.c_str(),-1,UiFont(px,bold),rc,&cf,b);
}
static void TextTrim(Graphics& g,const std::wstring& s,const RectF& rc,float px,const Brush* b,bool bold,StringTrimming trim,bool vcenter=false){
    StringFormat f; f.SetFormatFlags(StringFormatFlagsNoWrap); f.SetTrimming(trim); if(vcenter) f.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(s.c_str(),-1,UiFont(px,bold),rc,&f,b);
}
static float MeasureW(Graphics& g,const std::wstring& s,float px,bool bold){
    StringFormat nf; nf.SetFormatFlags(StringFormatFlagsNoWrap); RectF mw; g.MeasureString(s.c_str(),-1,UiFont(px,bold),PointF(0,0),&nf,&mw); return mw.Width;
}

static void DrawCameraIcon(Graphics& g, const RECT& r, Color c){
    Pen p(c,1.6f); SolidBrush b(Color(30,c.GetRed(),c.GetGreen(),c.GetBlue()));
    Rect body(r.left+3,r.top+7,r.right-r.left-6,r.bottom-r.top-10); DrawRoundRect(g,body,4,nullptr,&p);
    g.FillEllipse(&b,(REAL)(r.left+(r.right-r.left)/2-4),(REAL)(r.top+11),(REAL)8,(REAL)8);
    g.DrawLine(&p,(REAL)r.left+7,(REAL)r.top+5,(REAL)r.left+12,(REAL)r.top+5);
}
static void DrawVolumeIcon(Graphics& g,const RECT& r,Color c,bool muted,int vol){
    float x=(float)r.left,y=(float)r.top,w=(float)(r.right-r.left),h=(float)(r.bottom-r.top),cy=y+h/2;
    SolidBrush b(c); Pen p(c,1.8f);
    g.FillRectangle(&b,x,cy-h*0.16f,w*0.22f,h*0.32f);
    PointF cone[4]={PointF(x+w*0.20f,cy-h*0.16f),PointF(x+w*0.48f,cy-h*0.42f),PointF(x+w*0.48f,cy+h*0.42f),PointF(x+w*0.20f,cy+h*0.16f)};
    g.FillPolygon(&b,cone,4);
    if(muted||vol==0){
        Pen px2(c,2.f);
        g.DrawLine(&px2,x+w*0.60f,cy-h*0.25f,x+w*0.95f,cy+h*0.25f);
        g.DrawLine(&px2,x+w*0.60f,cy+h*0.25f,x+w*0.95f,cy-h*0.25f);
    } else {
        g.DrawArc(&p,x+w*0.30f,cy-h*0.30f,w*0.60f,h*0.60f,-40.f,80.f);
        Pen p2(Color(vol>45?255:80,c.GetRed(),c.GetGreen(),c.GetBlue()),1.8f);
        g.DrawArc(&p2,x+w*0.30f,cy-h*0.50f,w*1.00f,h*1.00f,-40.f,80.f);
    }
}
static void IconPlay(Graphics& g,const RectF& r,SolidBrush* b){
    float x=r.X,y=r.Y,w=r.Width,h=r.Height;
    PointF pts[3]={PointF(x+w*.08f,y+h*.02f),PointF(x+w*.94f,y+h*.5f),PointF(x+w*.08f,y+h*.98f)};
    g.FillPolygon(b,pts,3);
}
static void IconPause(Graphics& g,const RectF& r,SolidBrush* b){
    float bw=r.Width*.26f;
    g.FillRectangle(b,r.X+r.Width*.10f,r.Y,bw,r.Height);
    g.FillRectangle(b,r.X+r.Width*.64f,r.Y,bw,r.Height);
}
static void IconSkip(Graphics& g,const RectF& r,SolidBrush* b,bool fwd){
    float x=r.X,y=r.Y,w=r.Width,h=r.Height,bar=w*.15f;
    if(fwd){ g.FillRectangle(b,x+w*.85f,y,bar,h); PointF pts[3]={PointF(x,y+h*.03f),PointF(x+w*.76f,y+h*.5f),PointF(x,y+h*.97f)}; g.FillPolygon(b,pts,3); }
    else { g.FillRectangle(b,x,y,bar,h); PointF pts[3]={PointF(x+w,y+h*.03f),PointF(x+w*.24f,y+h*.5f),PointF(x+w,y+h*.97f)}; g.FillPolygon(b,pts,3); }
}
static void DrawRunnerRect(Graphics& g,const RectF& r,float rad,const COLORREF& col,BYTE maxA,float head){
    if(maxA<6||r.Width<24||r.Height<24||!FxOn()) return;
    const int N=26; float span=.16f; RectFC rc=RC(r);
    for(int s=N;s>=1;s--){
        float u=head-span*(float)s/N, u2=head-span*(float)(s-1)/N;
        u-=floorf(u); u2-=floorf(u2);
        float x1,y1,x2,y2; PerimeterPoint(rc,rad,u,x1,y1); PerimeterPoint(rc,rad,u2,x2,y2);
        float tt=1.f-(float)s/N;
        BYTE a=(BYTE)(maxA*powf(tt,1.7f));
        Pen p(Color(a,GetRValue(col),GetGValue(col),GetBValue(col)),2.2f+tt*1.8f);
        g.DrawLine(&p,x1,y1,x2,y2);
    }
    float hx,hy; PerimeterPoint(rc,rad,head,hx,hy);
    SolidBrush hb(Color(maxA,GetRValue(col),GetGValue(col),GetBValue(col)));
    g.FillEllipse(&hb,hx-2.5f,hy-2.5f,5.f,5.f);
}
static void DrawRunnerCircle(Graphics& g,const RectF& r,const COLORREF& col,BYTE maxA,float head){
    if(maxA<6||r.Width<30||!FxOn()) return;
    const int N=26; float spanDeg=95.f;
    for(int s=N;s>=1;s--){
        float a1=head*360.f-spanDeg*(float)s/N;
        float tt=1.f-(float)s/N;
        BYTE a=(BYTE)(maxA*powf(tt,1.7f));
        Pen p(Color(a,GetRValue(col),GetGValue(col),GetBValue(col)),2.4f+tt*2.0f);
        g.DrawArc(&p,r.X,r.Y,r.Width,r.Height,a1,-spanDeg/(float)N-1.5f);
    }
}
static void DrawParticles(Graphics& g,const RectF& area,const COLORREF& col,float t,float amul){
    if(!FxOn()) return;
    const int NP=34;
    float spd=g_cfg.particlesSpeed/100.0f;
    SolidBrush* bk[6];
    for(int i=0;i<6;++i) bk[i]=new SolidBrush(Color((BYTE)(i*51),GetRValue(col),GetGValue(col),GetBValue(col)));
    for(int k=0;k<NP;k++){
        float sp=(10.f+(float)(k%13))*spd;
        float cyc=area.Height+30.f;
        float yy=area.Y+area.Height-fmodf(t*sp+(float)k*37.7f,cyc);
        float wob=sinf(t*.9f+(float)k*2.1f)*(6.f+(float)(k%5)*2.f);
        float xx=area.X+8.f+fmodf((float)k*53.3f,std::max(8.f,area.Width-16.f))+wob;
        float fadeTop=std::min(1.f,(yy-area.Y)/40.f);
        float fadeBot=std::min(1.f,(area.Y+area.Height-yy)/30.f);
        float al=std::min(.95f,.78f*amul)*std::max(0.f,std::min(1.f,fadeTop*fadeBot));
        if(al<=.02f) continue;
        int bi=(int)(al*5.999f); if(bi<0)bi=0; if(bi>5)bi=5;
        float rad=1.8f+(float)(k%4)*.7f;
        g.FillEllipse(bk[bi],xx-rad,yy-rad,rad*2.f,rad*2.f);
    }
    for(int i=0;i<6;++i) delete bk[i];
}
static void DrawMarqueeText(Graphics& g,const std::wstring& s,const Font* f,const RectF& rc,Brush* brk,bool centerWhenFit,float speedPx,DWORD nowMs){
    StringFormat nf; nf.SetFormatFlags(StringFormatFlagsNoWrap); nf.SetTrimming(StringTrimmingNone);
    RectF mw; g.MeasureString(s.c_str(),(INT)s.size(),f,PointF(0,0),&nf,&mw);
    Region old; g.GetClip(&old); g.SetClip(rc);
    if(mw.Width<=rc.Width+1.f){
        if(centerWhenFit) nf.SetAlignment(StringAlignmentCenter);
        g.DrawString(s.c_str(),(INT)s.size(),f,rc,&nf,brk);
    } else {
        float total=mw.Width+S(60);
        float off=fmodf((nowMs/1000.0f)*speedPx,total);
        g.DrawString(s.c_str(),(INT)s.size(),f,PointF(rc.X-off,rc.Y),&nf,brk);
        g.DrawString(s.c_str(),(INT)s.size(),f,PointF(rc.X-off+total,rc.Y),&nf,brk);
    }
    g.SetClip(&old);
}
static Image* GetThumb(const std::wstring& path){
    if(path.empty()) return nullptr;
    auto it=g_thumbCache.find(path); if(it!=g_thumbCache.end()) return it->second;
    Image* img=new Image(path.c_str()); if(img->GetLastStatus()!=Ok){delete img;img=nullptr;}
    g_thumbCache[path]=img; return img;
}
// ---- fundo (wallpaper / capa embaçada) em cache de bitmap ----
static Image* g_wallImg=nullptr; static std::wstring g_wallLoadedPath; static Bitmap* g_bgCache=nullptr; static DWORD g_bgKey=0;
static void InvalidateBgCache(){ if(g_bgCache){delete g_bgCache;g_bgCache=nullptr;} }
// caches do CD grande girando (offscreen rotacionado + capa reduzida p/ transformar barato)
static Bitmap* g_cdCache=nullptr; static int g_cdCacheN=0; static float g_cdRot=-9999.f; static Image* g_cdImgKey=nullptr;
static Bitmap* g_cdSrc=nullptr; static Image* g_cdSrcKey=nullptr;
static void InvalidateCdCache(){ if(g_cdCache){delete g_cdCache;g_cdCache=nullptr;} if(g_cdSrc){delete g_cdSrc;g_cdSrc=nullptr;} g_cdImgKey=nullptr; g_cdSrcKey=nullptr; }
static Image* GetWallpaperImage(){
    if(g_cfg.bgWallpaper.empty()){ if(g_wallImg){delete g_wallImg;g_wallImg=nullptr;g_wallLoadedPath.clear();} return nullptr; }
    if(!g_wallImg||g_wallLoadedPath!=g_cfg.bgWallpaper){
        if(g_wallImg){delete g_wallImg;g_wallImg=nullptr;}
        g_wallImg=new Image(g_cfg.bgWallpaper.c_str());
        if(g_wallImg->GetLastStatus()!=Ok){delete g_wallImg;g_wallImg=nullptr;}
        g_wallLoadedPath=g_cfg.bgWallpaper; InvalidateBgCache();
    }
    return g_wallImg;
}
static void DrawAppBackground(Graphics& g,int w,int h,const Color& fallback){
    Image* wp=GetWallpaperImage();
    bool useBlur=(!wp&&g_cfg.coverBlurBg&&g_coverImg&&g_coverImg->GetLastStatus()==Ok&&FxOn());
    if(!wp&&!useBlur){ RectF full(0,0,(REAL)w,(REAL)h); SolidBrush fb(fallback); g.FillRectangle(&fb,full); return; }
    DWORD key=(DWORD)w ^ ((DWORD)h<<16) ^ (g_cfg.coverBlurBg?0x55AA0000u:0u) ^ ((DWORD)(std::hash<std::wstring>{}(g_cfg.bgWallpaper)&0xFFFF)) ^ (DWORD)((uintptr_t)g_coverImg&0xFFF) ^ ((DWORD)g_current<<12);
    if(!g_bgCache||key!=g_bgKey){
        InvalidateBgCache();
        g_bgCache=new Bitmap(w,h,PixelFormat32bppARGB);
        if(g_bgCache->GetLastStatus()==Ok){
            Graphics cg(g_bgCache); cg.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            RectF full(0,0,(REAL)w,(REAL)h); SolidBrush fb(fallback); cg.FillRectangle(&fb,full);
            if(wp){
                REAL iw=(REAL)wp->GetWidth(),ih=(REAL)wp->GetHeight();
                REAL sc=std::max(full.Width/std::max<REAL>(1.f,iw),full.Height/std::max<REAL>(1.f,ih));
                REAL dw=iw*sc,dh=ih*sc;
                cg.DrawImage(wp,(full.Width-dw)/2.f,(full.Height-dh)/2.f,dw,dh);
                SolidBrush dk(Color(170,3,4,12)); cg.FillRectangle(&dk,full);
            } else {
                Bitmap tiny(40,40,PixelFormat32bppARGB);
                {Graphics tg(&tiny); tg.SetInterpolationMode(InterpolationModeHighQualityBilinear); tg.Clear(Color(255,6,8,20)); tg.DrawImage(g_coverImg,0,0,40,40);}
                cg.DrawImage(&tiny,full,0,0,40,40,UnitPixel);
                SolidBrush dk(Color(150,3,4,12)); cg.FillRectangle(&dk,full);
            }
            g_bgKey=key;
        } else { delete g_bgCache; g_bgCache=nullptr; }
    }
    if(g_bgCache) g.DrawImage(g_bgCache,0,0);
}
static void DrawCoverCircle(Graphics& g,Image* img,Rect r,Pen* pen,float rotation){
    float cx=r.X+r.Width/2.f,cy=r.Y+r.Height/2.f;g.TranslateTransform(cx,cy);g.RotateTransform(rotation);g.TranslateTransform(-cx,-cy);
    SolidBrush disc(Color(255,18,20,30));g.FillEllipse(&disc,(REAL)r.X,(REAL)r.Y,(REAL)r.Width,(REAL)r.Height);
    if(img&&img->GetLastStatus()==Ok){GraphicsPath clip;clip.AddEllipse((REAL)r.X,(REAL)r.Y,(REAL)r.Width,(REAL)r.Height);Region old;g.GetClip(&old);g.SetClip(&clip);g.DrawImage(img,r.X,r.Y,r.Width,r.Height);g.SetClip(&old);}
    SolidBrush hole(Color(255,7,9,18));float hr=r.Width*.11f;g.FillEllipse(&hole,(REAL)(cx-hr),(REAL)(cy-hr),(REAL)(hr*2),(REAL)(hr*2));g.DrawEllipse(pen,(REAL)r.X,(REAL)r.Y,(REAL)r.Width,(REAL)r.Height);g.ResetTransform();
}
// CD grande girando: desenha num offscreen so quando o angulo quantizado muda e reusa
// isso nos outros frames (o DrawImage rotacionado + clip era o que derrubava o FPS).
// A capa ainda e reduzida para ~768px antes de transformar: capa 4K reamostrada todo
// frame era o custo principal (girava travado com capas grandes).
static void DrawBigCd(Graphics& g,Image* img,Rect r,Pen* pen,float rotation){
    int n=r.Width>0?r.Width:1;
    float q=1.2f, qrot=(float)std::lround(rotation/q)*q;
    if(!g_cdCache||g_cdCacheN!=n||g_cdImgKey!=img||std::fabs(g_cdRot-qrot)>0.001f){
        if(!g_cdCache||g_cdCacheN!=n){ delete g_cdCache; g_cdCache=nullptr; }
        if(g_cdSrcKey!=img||!g_cdSrc){   // reduz a capa uma vez para a transformada ficar barata
            delete g_cdSrc; g_cdSrc=nullptr;
            if(img&&img->GetLastStatus()==Ok){
                int iw=img->GetWidth(),ih=img->GetHeight(),mx=std::max(iw,ih),cap=768;
                int sw=iw*std::min(cap,mx)/mx, sh=ih*std::min(cap,mx)/mx;
                if(sw<1)sw=1; if(sh<1)sh=1;
                g_cdSrc=new Bitmap(sw,sh,PixelFormat32bppARGB);
                Graphics sg(g_cdSrc); sg.SetInterpolationMode(InterpolationModeHighQualityBilinear);
                sg.Clear(Color(0,0,0,0)); sg.DrawImage(img,0,0,sw,sh);
            }
            g_cdSrcKey=img;
        }
        if(!g_cdCache) g_cdCache=new Bitmap(n,n,PixelFormat32bppARGB);
        Graphics cg(g_cdCache);
        cg.SetSmoothingMode(SmoothingModeAntiAlias);
        cg.Clear(Color(0,0,0,0));
        float ccx=n/2.f,ccy=n/2.f;
        cg.TranslateTransform(ccx,ccy);cg.RotateTransform(qrot);cg.TranslateTransform(-ccx,-ccy);
        SolidBrush disc(Color(255,18,20,30));cg.FillEllipse(&disc,(REAL)0,(REAL)0,(REAL)n,(REAL)n);
        if(g_cdSrc){GraphicsPath clip;clip.AddEllipse((REAL)0,(REAL)0,(REAL)n,(REAL)n);Region old;cg.GetClip(&old);cg.SetClip(&clip);cg.DrawImage(g_cdSrc,0,0,n,n);cg.SetClip(&old);}
        SolidBrush hole(Color(255,7,9,18));float hr=n*.11f;cg.FillEllipse(&hole,ccx-hr,ccy-hr,hr*2,hr*2);
        if(pen)cg.DrawEllipse(pen,(REAL)0,(REAL)0,(REAL)n,(REAL)n);
        cg.ResetTransform();
        g_cdCacheN=n; g_cdImgKey=img; g_cdRot=qrot;
    }
    g.DrawImage(g_cdCache,(REAL)r.X,(REAL)r.Y,(REAL)r.Width,(REAL)r.Height);
}
// ---- pecas novas do cabecalho ----
static void DrawPill(Graphics& g,const RECT& r,const std::wstring& t,bool on,float px){
    if(r.right-r.left<=0) return;
    RectF b=RF(r);
    SolidBrush bg(on?ToGdi(g_theme.accent,55):Color(255,13,16,30)); Pen pn(on?ToGdi(g_theme.accent):Color(255,60,64,88),1.2f);
    DrawRoundRect(g,b,(int)S(10),&bg,&pn);
    SolidBrush tb(on?ToGdi(g_theme.accent):Color(255,145,147,160));
    TextCenter(g,t,b,px,&tb,true);
}
static std::wstring SortLabel(){ return L"ORDEM: "+SortModeName(g_cfg.sortMode)+(g_cfg.sortMode==L"manual"?L"":(g_cfg.sortDesc?L" ▼":L" ▲")); }
static void DrawShapeToggle(Graphics& g,Brush* ab,Brush* white,Brush* gray){
    if(R_shapeTgl.right<=R_shapeTgl.left) return;
    RectF st=RF(R_shapeTgl);
    SolidBrush sbg(Color(255,13,16,30)); Pen spn(Color(255,60,64,88),1.2f);
    DrawRoundRect(g,st,(int)S(10),&sbg,&spn);
    bool cd=g_cfg.artShape==L"cd";
    RectF hl(cd?st.X+st.Width/2.f:st.X,st.Y,st.Width/2.f,st.Height);
    SolidBrush hbg(ToGdi(g_theme.accent,50)); DrawRoundRect(g,hl,(int)S(9),&hbg,nullptr);
    TextCenter(g,L"QUAD",RectF(st.X,st.Y,st.Width/2.f,st.Height),S(14),cd?gray:white,true);
    TextCenter(g,L"CD",RectF(st.X+st.Width/2.f,st.Y,st.Width/2.f,st.Height),S(14),!cd?gray:ab,true);
}
static void DrawChromeButtons(Graphics& g){
    if(R_close.right>R_close.left){
        RectF c=RF(R_close); SolidBrush bg(Color(255,40,18,26)); Pen pn(Color(255,120,50,64),1.2f); DrawRoundRect(g,c,(int)S(8),&bg,&pn);
        Pen w(Color(255,235,236,242),2.f);
        g.DrawLine(&w,c.X+c.Width*.32f,c.Y+c.Height*.32f,c.X+c.Width*.68f,c.Y+c.Height*.68f);
        g.DrawLine(&w,c.X+c.Width*.68f,c.Y+c.Height*.32f,c.X+c.Width*.32f,c.Y+c.Height*.68f);
    }
    if(R_min.right>R_min.left){
        RectF m=RF(R_min); SolidBrush bg(Color(255,16,19,34)); Pen pn(Color(255,60,64,88),1.2f); DrawRoundRect(g,m,(int)S(8),&bg,&pn);
        Pen w(Color(255,145,147,160),2.f); g.DrawLine(&w,m.X+m.Width*.30f,m.Y+m.Height*.62f,m.X+m.Width*.70f,m.Y+m.Height*.62f);
    }
}
static void DrawVolumeBar(Graphics& g,Brush* ab,Brush* gray,int barH){
    Color ic=g_muted?Color(255,145,147,160):ToGdi(g_theme.accent);
    DrawVolumeIcon(g,R_volIcon,ic,g_muted,g_cfg.volume);
    Rect vt(R_vol.left,R_vol.top,R_vol.right-R_vol.left,barH);SolidBrush vb(Color(255,40,42,60));DrawRoundRect(g,vt,2,&vb,nullptr);
    int fw=(int)((R_vol.right-R_vol.left)*(g_muted?0:g_cfg.volume)/100.f);
    if(fw>1){Rect vf(R_vol.left,R_vol.top,fw,barH);DrawRoundRect(g,vf,2,ab,nullptr);}
    wchar_t pct[16]; swprintf(pct,16,L"%d%%",g_muted?0:g_cfg.volume);
    TextAt(g,pct,(float)R_vol.right+S(8),(float)R_vol.top-S(5),S(10),gray);
}
static void DrawOrderArrows(Graphics& g,size_t i,Brush* ab,Brush* gray){
    if(g_cfg.sortMode!=L"manual"||i>=R_rowUp.size()) return;
    if(R_rowUp[i].right>R_rowUp[i].left) TextCenter(g,L"▲",RF(R_rowUp[i]),S(11),i>0?ab:gray);
    if(R_rowDown[i].right>R_rowDown[i].left) TextCenter(g,L"▼",RF(R_rowDown[i]),S(11),i+1<g_tracks.size()?ab:gray);
}
static void DrawStatusToast(Graphics& g,int w,int h){
    if(!StatusVisible()) return;
    float px=S(12); float tw=MeasureW(g,g_status,px,true)+S(36);
    RectF b(((float)w-tw)/2.f,(float)h-S(46),tw,S(32));
    SolidBrush bg(Color(230,12,15,30)); Pen pn(ToGdi(g_theme.accent),1.2f);
    DrawRoundRect(g,b,(int)S(10),&bg,&pn);
    SolidBrush white(Color(255,235,236,242)); TextCenter(g,g_status,b,px,&white,true);
}

static void DrawSettings(Graphics& g,int w,int h);

// ---- barra da biblioteca: abas / voltar, busca, nova playlist ----------------
static void DrawSearchIcon(Graphics& g,float cx,float cy,float r,Color c){ Pen p(c,1.6f); g.DrawEllipse(&p,cx-r,cy-r,2*r,2*r); Pen p2(c,2.f); g.DrawLine(&p2,cx+r*0.7f,cy+r*0.7f,cx+r*1.7f,cy+r*1.7f); }
static void DrawLibBar(Graphics& g,const Brush* ab,const Brush* white,const Brush* gray,Color cAb,Color cGray){
    if(R_libBar.right<=R_libBar.left) return;
    if(g_view==2){
        DrawPill(g,R_plBack,L"< PLAYLISTS",false,S(11)); DrawPill(g,R_plAdd,L"+ ADICIONAR",true,S(11));
        if(const Playlist* op=OpenPlaylistPtr()) DrawPill(g,R_plMode,(op->mode.empty()?g_cfg.onlineMode:op->mode)==L"download"?L"ONLINE: BAIXAR":L"ONLINE: STREAM",!op->mode.empty(),S(10));
    } else if(g_pickMode){
        DrawPill(g,R_pickDone,L"CONCLUIR ("+std::to_wstring(g_pickSel.size())+L")",true,S(11)); DrawPill(g,R_pickCancel,L"CANCELAR",false,S(11));
    } else {
        DrawPill(g,R_tabTracks,L"MÚSICAS",g_view==0,S(11));
        DrawPill(g,R_tabPlaylists,L"PLAYLISTS",g_view==1,S(11));
        DrawPill(g,R_tabOnline,L"ONLINE",OU().open,S(11));
    }
    if(R_plNew.right>R_plNew.left) DrawPill(g,R_plNew,L"+ NOVA PLAYLIST",false,S(11));
    if(R_searchBox.right>R_searchBox.left){
        RectF b=RF(R_searchBox);
        SolidBrush fb(Color(235,10,13,26)); Pen ln(g_searchFocus?ToGdi(g_theme.accent):Color(255,50,54,76),g_searchFocus?1.5f:1.f);
        DrawRoundRect(g,b,(int)S(9),&fb,&ln);
        DrawSearchIcon(g,b.X+S(16),b.Y+b.Height/2-S(1),S(5),g_searchFocus?cAb:cGray);
        float tx=b.X+S(32), tw=b.Width-S(32)-(g_searchBuf.empty()?S(10):S(120));
        if(g_searchBuf.empty()&&!g_searchFocus){
            std::wstring ph=(g_pickMode&&g_pickPl>=0&&g_pickPl<(int)g_playlists.size())?L"Marcando para \""+g_playlists[(size_t)g_pickPl].name+L"\"   ·   buscar...":(g_view==2&&g_openPl>=0&&g_openPl<(int)g_playlists.size())?L"Playlist: "+g_playlists[(size_t)g_openPl].name+L"   ·   clique aqui para buscar":L"Buscar faixa (nome, artista ou arquivo)...";
            TextTrim(g,ph,RectF(tx,b.Y,tw,b.Height),S(11),gray,false,StringTrimmingEllipsisCharacter,true);
        } else {
            std::wstring txt=g_searchBuf; if(g_searchFocus&&(NowMs()/500)%2==0) txt+=L"|";
            TextTrim(g,txt,RectF(tx,b.Y,tw,b.Height),S(12),white,false,StringTrimmingEllipsisCharacter,true);
        }
        if(!g_searchBuf.empty()){
            StringFormat sfFar; sfFar.SetAlignment(StringAlignmentFar); sfFar.SetLineAlignment(StringAlignmentCenter);
            std::wstring cnt=std::to_wstring(g_visible.size())+L" de "+std::to_wstring(g_tracks.size());
            g.DrawString(cnt.c_str(),-1,UiFont(S(10),false),RectF(b.X+b.Width-S(120),b.Y,S(80),b.Height),&sfFar,gray);
            StringFormat cf; cf.SetAlignment(StringAlignmentCenter); cf.SetLineAlignment(StringAlignmentCenter);
            g.DrawString(L"✕",-1,SymFont(S(12)),RF(R_searchClear),&cf,white);
        }
    }
}
// ---- cards de playlists ("Todas as musicas", cada playlist, "nova") ----------------
static void DrawPlaylistCards(Graphics& g,const Brush* ab,const Brush* white,const Brush* gray){
    if(g_view!=1) return;
    int n=(int)g_playlists.size()+2;
    const std::vector<Track>& lib=g_libCached?g_libTracks:g_tracks;
    StringFormat cf; cf.SetAlignment(StringAlignmentCenter); cf.SetLineAlignment(StringAlignmentCenter); cf.SetFormatFlags(StringFormatFlagsNoWrap); cf.SetTrimming(StringTrimmingEllipsisCharacter);
    for(int k=0;k<n&&k<(int)R_plCards.size();k++){
        RECT rr=R_plCards[(size_t)k]; if(rr.right<=rr.left) continue;
        RectF card=RF(rr); bool isAll=(k==0), isNew=(k==n-1); int pi=k-1;
        bool hot=isAll?(g_openPl<0):(!isNew&&pi==g_openPl);
        SolidBrush cb(Color(255,9,12,25)); Pen cp(hot?ToGdi(g_theme.accent,220):Color(255,40,44,65),1.5f);
        DrawRoundRect(g,card,16,&cb,&cp);
        if(isNew){
            g.DrawString(L"+",-1,UiFont(S(46),true),RectF(card.X,card.Y+S(26),card.Width,S(64)),&cf,ab);
            g.DrawString(L"NOVA PLAYLIST",-1,UiFont(S(12),true),RectF(card.X,card.Y+S(98),card.Width,S(24)),&cf,white);
            g.DrawString(L"vazia, de uma pasta, de um link ou da busca online",-1,UiFont(S(9),false),RectF(card.X+S(10),card.Y+S(124),card.Width-S(20),S(20)),&cf,gray);
            continue;
        }
        int cov=SI(96); RectF art(card.X+S(16),card.Y+S(16),(REAL)cov,(REAL)cov);
        std::wstring coverPath=isAll?(lib.empty()?L"":lib[0].coverPath):g_playlists[(size_t)pi].coverPath;
        Image* im=coverPath.empty()?nullptr:GetThumb(coverPath);
        SolidBrush plate(Color(255,18,21,34)); DrawRoundRect(g,art,10,&plate,nullptr);
        if(im) g.DrawImage(im,art.X,art.Y,art.Width,art.Height);
        else { Pen rp(ToGdi(g_theme.accent,160),2.f); g.DrawEllipse(&rp,art.X+cov*0.18f,art.Y+cov*0.18f,cov*0.64f,cov*0.64f); SolidBrush rb(ToGdi(g_theme.accent,160)); g.FillEllipse(&rb,art.X+cov*0.42f,art.Y+cov*0.42f,cov*0.16f,cov*0.16f); }
        float tx=art.X+cov+S(14), tw=card.Width-(tx-card.X)-S(12);
        std::wstring name=isAll?L"Todas as músicas":g_playlists[(size_t)pi].name;
        size_t count=isAll?lib.size():g_playlists[(size_t)pi].entries.size();
        TextTrim(g,name,RectF(tx,card.Y+S(18),tw,S(44)),S(16),white,true,StringTrimmingEllipsisWord);
        TextTrim(g,isAll?std::to_wstring(count)+(count==1?L" faixa na biblioteca":L" faixas na biblioteca"):PlaylistCardSubtitle(pi),RectF(tx,card.Y+S(66),tw,S(18)),S(11),ab,false,StringTrimmingEllipsisCharacter);
        TextTrim(g,isAll?L"clique: abrir a biblioteca":L"clique: abrir  ·  botão direito: opções",RectF(tx,card.Y+S(88),tw,S(16)),S(9),gray,false,StringTrimmingEllipsisCharacter);
        DrawPill(g,R_plPlay[(size_t)k],L"TOCAR",false,S(10));
        DrawPill(g,R_plShuf[(size_t)k],L"ALEATÓRIO",false,S(10));
    }
}
// marcando musicas para uma playlist: circulo de selecao por card/linha
static void DrawPickMark(Graphics& g,size_t i,const RECT& rr){
    if(!g_pickMode||i>=g_tracks.size()) return;
    bool have=PickAlreadyIn(g_tracks[i]), sel=g_pickSel.count(g_tracks[i].path)>0;
    RectF card=RF(rr);
    if(sel){ SolidBrush ov(ToGdi(g_theme.accent,40)); Pen pn(ToGdi(g_theme.accent),2.2f); DrawRoundRect(g,card,(int)S(12),&ov,&pn); }
    else if(have){ SolidBrush ov(Color(130,5,7,18)); DrawRoundRect(g,card,(int)S(12),&ov,nullptr); }
    REAL d=S(26); RectF c(card.X+card.Width-d-S(10),card.Y+S(10),d,d);
    SolidBrush fill(sel?ToGdi(g_theme.accent):(have?Color(255,60,64,88):Color(215,12,15,30))); g.FillEllipse(&fill,c);
    Pen ring(sel?Color(255,235,236,242):Color(255,120,124,150),1.6f); g.DrawEllipse(&ring,c);
    if(sel||have){ Pen ck(Color(255,255,255,255),2.4f); g.DrawLine(&ck,c.X+d*0.27f,c.Y+d*0.52f,c.X+d*0.44f,c.Y+d*0.70f); g.DrawLine(&ck,c.X+d*0.44f,c.Y+d*0.70f,c.X+d*0.75f,c.Y+d*0.32f); }
    if(have){ SolidBrush gr(Color(255,145,147,160)); StringFormat rf; rf.SetAlignment(StringAlignmentFar); rf.SetLineAlignment(StringAlignmentCenter); g.DrawString(L"já está",-1,UiFont(S(9),false),RectF(c.X-S(72),c.Y,S(66),d),&rf,&gr); }
}
static void DrawOnlineTag(Graphics& g,const RectF& art,const std::wstring& url){   // faixa online: etiqueta na capa + estado do canal (tocando / na fila)
    const StreamInfo* c=FindSnap(url);
    REAL hh=std::max((REAL)S(11),art.Height*0.18f);
    RectF tag(art.X,art.Y+art.Height-hh,art.Width,hh);
    SolidBrush bg(Color(200,6,8,18)); g.FillRectangle(&bg,tag);
    Color lc=(c&&c->phase==5)?Color(255,235,120,110):ToGdi(g_theme.accent);
    SolidBrush ac(lc); TextCenter(g,c?StreamTagLabel(*c,art.Width>=S(90)):std::wstring(L"ONLINE"),tag,std::max((REAL)S(7),hh*(c?0.5f:0.62f)),&ac,true);
    if(c&&c->phase!=5){ REAL fr=StreamBufFrac(*c); SolidBrush tb(Color(255,40,44,65)); g.FillRectangle(&tb,tag.X,tag.Y+tag.Height-S(2),tag.Width,(REAL)S(2)); g.FillRectangle(&ac,tag.X,tag.Y+tag.Height-S(2),tag.Width*fr,(REAL)S(2)); }
}
static void DrawNormal(Graphics& g,int w,int h){
    g_streamSnap=StreamSnapshot();
    DrawAppBackground(g,w,h,Color(255,5,7,18));
    Color accent=ToGdi(g_theme.accent); Pen ap(accent,2); SolidBrush ab(accent), white(Color(255,235,236,242)), gray(Color(255,145,147,160)), dimB(Color(255,44,48,70));
    const Font &fBrand=*UiFont(S(17),true),&fTitleBase=*UiFont(TextScale(20,g_cfg.titleScale),true),&fArtistBase=*UiFont(TextScale(13,g_cfg.artistScale),false),&fLabel=*UiFont(S(12),true),&fSmall=*UiFont(S(10),false);
    g.DrawString(L"REMIX",-1,&fBrand,PointF(S(18),S(14)),&ab);
    DrawShapeToggle(g,&ab,&white,&gray);
    DrawPill(g,R_autoTgl,L"AUTO",g_cfg.autoplay,S(12));
    DrawPill(g,R_sortBtn,SortLabel(),g_cfg.sortMode==L"manual",S(11));
    DrawPill(g,R_folderBtn,g_view==2?L"PASTA DA PLAYLIST ▼":L"PASTA ▼",g_folderMenuOpen,S(10));
    DrawChromeButtons(g);
    if(R_listBtn.right>R_listBtn.left){
        bool lm=g_cfg.listMode!=0;
        Pen lpn(lm?ToGdi(g_theme.accent):Color(255,90,94,118),1.6f);
        float bx=(REAL)(R_listBtn.left+R_listBtn.right)/2-S(20),by=(REAL)(R_listBtn.top+R_listBtn.bottom)/2-S(12);
        for(int i=0;i<3;i++){ g.DrawRectangle(&lpn,bx,by+i*S(10)-1.f,(REAL)S(5),(REAL)S(5)); g.DrawLine(&lpn,bx+S(11),by+i*S(10)+S(2),bx+S(36),by+i*S(10)+S(2)); }
    }
    g.DrawString(L"⚙",-1,SymFont(S(26)),PointF((REAL)R_gear.left+S(7),(REAL)R_gear.top+S(2)),&gray);
    DWORD pos=(g_current>=0&&g_player.loaded)?g_player.GetPositionMs():0;
    DWORD len=(g_current>=0&&g_player.loaded)?g_player.GetLengthMs():0;
    float frac=len?std::min(1.f,(float)pos/len):0;
    float tSec=NowMs()/1000.0f;
    COLORREF cNav=ResolveCustom(g_cfg.btnNavColor), cPlay=ResolveCustom(g_cfg.btnPlayColor);
    SolidBrush navB(ToGdi(cNav)); Pen playP(ToGdi(cPlay),2.4f); SolidBrush playB(ToGdi(cPlay));
    StringFormat cf; cf.SetAlignment(StringAlignmentCenter); cf.SetLineAlignment(StringAlignmentCenter);
    StringFormat noWrapF; noWrapF.SetFormatFlags(StringFormatFlagsNoWrap); noWrapF.SetTrimming(StringTrimmingEllipsisCharacter);
    {
        RectF pnl=RF(R_playerPanel);
        SolidBrush pb(Color(255,8,11,24)); DrawRoundRect(g,pnl,18,&pb,nullptr);
        {   // LED: contorno neon do painel (brilho, velocidade e efeito das configuracoes)
            BYTE la=LedAlpha(g_glowPhase,40,200); COLORREF cLed=ResolveCustom(g_cfg.ledColor);
            for(int i=0;i<3;i++){ Pen lp(ToGdi(cLed,(BYTE)(la/(i+1))),1.f+i*.5f); RectF gr(pnl.X-(REAL)i,pnl.Y-(REAL)i,pnl.Width+2.f*i,pnl.Height+2.f*i); DrawRoundRect(g,gr,18+i,nullptr,&lp); }
        }
        BYTE ra=RunnerAlpha();
        if(g_cfg.runnerOn&&ra>6) DrawRunnerRect(g,pnl,18.f,ResolveCustom(g_cfg.runnerColor),ra,g_runnerPhase);
        bool isCd=g_cfg.artShape==L"cd";
        int asize=R_art.right-R_art.left; int x0=R_art.left;
        Rect cvr(R_art.left,R_art.top,asize,asize);
        if(isCd){ DrawBigCd(g,g_coverImg,cvr,&ap,g_player.playing?g_rotation:0);
            if(g_cfg.particlesOn&&FxOn()){ GraphicsPath ep; ep.AddEllipse((REAL)cvr.X+4,(REAL)cvr.Y+4,(REAL)cvr.Width-8,(REAL)cvr.Height-8); Region eOld; g.GetClip(&eOld); g.SetClip(&ep);
                DrawParticles(g,RectF((REAL)cvr.X+6,(REAL)cvr.Y+6,(REAL)cvr.Width-12,(REAL)cvr.Height-12),ResolveCustom(g_cfg.particlesColor),tSec,1.7f); g.SetClip(&eOld); } }
        else{
            SolidBrush plate(Color(255,16,19,32)); DrawRoundRect(g,cvr,14,&plate,&ap);
            if(g_coverImg&&g_coverImg->GetLastStatus()==Ok){
                if(GlitchNow(NowMs())){
                    REAL sh=(REAL)asize,ih=(REAL)g_coverImg->GetHeight(),iw=(REAL)g_coverImg->GetWidth();
                    struct SL{REAL y0,y1,dx;}; SL sl[3]={{0,sh*.34f,-6},{sh*.34f,sh*.63f,5},{sh*.63f,sh,-3}};
                    for(auto&s:sl){ RectF dst((REAL)cvr.X+s.dx,(REAL)(cvr.Y+s.y0),(REAL)cvr.Width,s.y1-s.y0); g.DrawImage(g_coverImg,dst,(REAL)0,(s.y0/sh)*ih,iw,((s.y1-s.y0)/sh)*ih,UnitPixel); }
                    SolidBrush t1(Color(40,255,60,60)),t2(Color(40,60,180,255));
                    g.FillRectangle(&t1,(REAL)(cvr.X-4),(REAL)(cvr.Y+sh*.12f),(REAL)(cvr.Width+8),3.f);
                    g.FillRectangle(&t2,(REAL)(cvr.X+3),(REAL)(cvr.Y+sh*.55f),(REAL)(cvr.Width+6),3.f);
                } else g.DrawImage(g_coverImg,(REAL)cvr.X+3,(REAL)cvr.Y+3,(REAL)cvr.Width-6,(REAL)cvr.Height-6);
            }
            if(g_cfg.particlesOn&&FxOn()) DrawParticles(g,RectF((REAL)cvr.X+2,(REAL)cvr.Y+2,(REAL)cvr.Width-4,(REAL)cvr.Height-4),ResolveCustom(g_cfg.particlesColor),tSec,1.f);
        }
        const Track* ct=(g_current>=0&&g_current<(int)g_tracks.size())?&g_tracks[(size_t)g_current]:(g_nowPlayingValid?&g_nowPlaying:nullptr);
        std::wstring title=ct?ct->title:L"Nenhuma musica";
        std::wstring artist=ct?ct->artist:L"—";
        {
            int ty0=R_art.bottom+SI(3);
            int availTxt=R_wavePanel.top-SI(2)-ty0; if(availTxt<S(40)) availTxt=(int)S(40);
            float fth=TextScale(20,g_cfg.titleScale), fah=TextScale(13,g_cfg.artistScale);
            int th=std::max((int)S(22),std::min((int)(fth*1.35f)+SI(4),availTxt*45/100));
            int ah=std::max((int)S(16),availTxt-th-SI(2));
            const Font &fTitleFit=*UiFont(std::min(fth,(float)th*0.78f),true),&fArtistFit=*UiFont(std::min(fah,(float)ah*0.80f),false);
            RectF tr((REAL)x0,(REAL)ty0,(REAL)asize,(REAL)th);
            DrawMarqueeText(g,title,&fTitleFit,tr,&white,false,S(30),NowMs());
            int ay2=ty0+th+SI(2);
            RectF ar((REAL)x0,(REAL)ay2,(REAL)asize,(REAL)ah);
            DrawMarqueeText(g,artist,&fArtistFit,ar,&ab,false,S(26),NowMs());
            RectF mw; g.MeasureString(artist.c_str(),-1,&fArtistFit,ar,&noWrapF,&mw);
            REAL aw=mw.Width>2.f?std::min(mw.Width,(REAL)(asize-S(34))):(REAL)S(60);
            R_pencilPanel={(LONG)(x0+(int)aw+S(6)),(LONG)(ay2),(LONG)(x0+(int)aw+S(28)),(LONG)(ay2+ah)};
            g.DrawString(L"✎",-1,SymFont(S(13)),RF(R_pencilPanel),&cf,&gray);
        }
        int wy=R_wavePanel.top, wb=R_wavePanel.bottom; int wl=x0, wr=R_art.right;
        int countP=std::max(30,(wr-wl)/(FxOn()?6:12));
        float bw=2.6f,gap=countP>1?((wr-wl-countP*bw)/(float)(countP-1)):0.f;
        for(int k=0;k<countP;k++){
            float v=(len?AudioWaveBar(k,countP,pos,len,tSec):WaveIdleAt(k,countP,tSec));
            float bh=std::max(4.f,(float)(wb-wy)*v);
            float x=(float)wl+k*(bw+gap), y=((float)wy+(float)wb-bh)/2.f;
            float pk=countP>1?(float)k/(countP-1):0.f;
            g.FillRectangle(pk<=frac?(Brush*)&ab:(Brush*)&dimB,x,y,bw,bh);
        }
        float syl=(float)(R_seek.top+R_seek.bottom)/2.f;
        Pen sbg(Color(75,85,87,110),3),sfg(accent,3),kn(Color(220,255,255,255),1.4f);
        g.DrawLine(&sbg,(REAL)wl,syl,(REAL)wr,syl); g.DrawLine(&sfg,(REAL)wl,syl,(REAL)(wl+(wr-wl)*frac),syl);
        float kx=(REAL)wl+(REAL)(wr-wl)*frac;
        g.FillEllipse(&ab,kx-S(6),syl-S(6),(REAL)S(12),(REAL)S(12)); g.DrawEllipse(&kn,kx-S(6),syl-S(6),(REAL)S(12),(REAL)S(12));
        g.DrawString(FormatTime(pos).c_str(),-1,&fSmall,PointF((REAL)wl,syl+S(9)),&gray);
        {StringFormat sfR; sfR.SetAlignment(StringAlignmentFar); g.DrawString(FormatTime(len).c_str(),-1,&fSmall,RectF((REAL)wl,syl+S(9),(REAL)(wr-wl),18),&sfR,&gray);}
        g.DrawString(L"⇄",-1,SymFont(S(17)),RF(R_shuffle),&cf,g_cfg.shuffle?(Brush*)&navB:(Brush*)&gray);
        {RECT z=R_prev; RectF pv((REAL)(z.left+S(5)),(REAL)(z.top+S(9)),(REAL)(z.right-z.left-S(10)),(REAL)(z.bottom-z.top-S(18))); IconSkip(g,pv,&navB,false);}
        g.DrawEllipse(&playP,(REAL)R_play.left,(REAL)R_play.top,(REAL)(R_play.right-R_play.left),(REAL)(R_play.bottom-R_play.top));
    if(g_converting){ Pen spn(ToGdi(g_theme.accent),3.f); g.DrawArc(&spn,(REAL)R_play.left-S(4),(REAL)R_play.top-S(4),(REAL)(R_play.right-R_play.left)+S(8),(REAL)(R_play.bottom-R_play.top)+S(8),(REAL)(NowMs()%1000)*0.36f,100.f); }   // conectando/convertendo
        {RECT z=R_play; RectF pl((REAL)(z.left+S(17)),(REAL)(z.top+S(17)),(REAL)(z.right-z.left-S(34)),(REAL)(z.bottom-z.top-S(34))); if(g_player.playing) IconPause(g,pl,&playB); else IconPlay(g,pl,&playB);}
        {RECT z=R_next; RectF nx((REAL)(z.left+S(5)),(REAL)(z.top+S(9)),(REAL)(z.right-z.left-S(10)),(REAL)(z.bottom-z.top-S(18))); IconSkip(g,nx,&navB,true);}
        g.DrawString(L"⟳",-1,SymFont(S(17)),RF(R_repeat),&cf,g_cfg.repeat?(Brush*)&navB:(Brush*)&gray);
        DrawVolumeBar(g,&ab,&gray,5);
        Pen grip(Color(160,150,153,175),2);
        for(int i=1;i<=3;i++) g.DrawLine(&grip,(REAL)(pnl.X+pnl.Width-i*7),(REAL)(pnl.Y+pnl.Height-5),(REAL)(pnl.X+pnl.Width-5),(REAL)(pnl.Y+pnl.Height-i*7));
    }
    Region libOld; bool libClip=R_library.right>R_library.left; if(libClip){ g.GetClip(&libOld); g.SetClip(Rect(R_library.left,R_library.top,R_library.right-R_library.left,R_library.bottom-R_library.top)); }
    if(g_cfg.listMode!=0){
        const Font& fRowT=*UiFont(S(13),true);
        StringFormat trimL; trimL.SetTrimming(StringTrimmingEllipsisWord); trimL.SetFormatFlags(StringFormatFlagsNoWrap);
        for(size_t i=0;i<g_tracks.size() && i<R_cardRects.size();++i){
            RECT rr=R_cardRects[i]; if(rr.right-rr.left<=0) continue;
            bool cur=(int)i==g_current;
            RectF row=RF(rr);
            BYTE ra=RunnerAlpha();
            SolidBrush rb(cur?ToGdi(g_theme.accent,40):Color(210,9,12,25));
            Pen rp(cur?ToGdi(g_theme.accent,190):Color(255,32,36,56),1.2f);
            DrawRoundRect(g,row,(int)S(10),&rb,&rp);
            if(cur&&g_cfg.runnerOn&&ra>6&&FxOn()){ RectF edge((REAL)rr.left,(REAL)rr.top,S(3),(REAL)(rr.bottom-rr.top)); SolidBrush eb(ToGdi(ResolveCustom(g_cfg.runnerColor),(BYTE)(ra*.7f))); DrawRoundRect(g,edge,1,&eb,nullptr); }
            int th=SI(44); Rect art(rr.left+SI(10),rr.top+(rr.bottom-rr.top-th)/2,th,th);
            if(g_cfg.artShape==L"cd") DrawCoverCircle(g,GetThumb(g_tracks[i].coverPath),art,&rp,cur&&g_player.playing?g_rotation:0);
            else {SolidBrush plate(Color(255,18,21,34));DrawRoundRect(g,art,6,&plate,nullptr);Image* im=GetThumb(g_tracks[i].coverPath);if(im)g.DrawImage(im,art.X,art.Y,art.Width,art.Height);}
            if(IsOnlineTrack(g_tracks[i])) DrawOnlineTag(g,RectF((REAL)art.X,(REAL)art.Y,(REAL)art.Width,(REAL)art.Height),g_tracks[i].path);
            float tx=(REAL)rr.left+S(72);
            RectF rtT(tx,(REAL)rr.top+S(9),(REAL)(rr.right-tx-S(60)),(REAL)S(21));
            if(cur) DrawMarqueeText(g,g_tracks[i].title,&fRowT,rtT,&white,false,S(26),NowMs());
            else g.DrawString(g_tracks[i].title.c_str(),-1,&fRowT,rtT,&trimL,&white);
            {   // linha: artista + estado do canal de streaming (tocando / na fila: pronta, carregando...)
                REAL aw=(REAL)(rr.right-tx-S(60));
                const StreamInfo* sc=IsOnlineTrack(g_tracks[i])?FindSnap(g_tracks[i].path):nullptr;
                if(sc&&aw>S(380)){ aw-=S(250); SolidBrush qb(sc->phase==5?Color(255,235,120,110):Color(255,145,147,160)); StringFormat rf; rf.SetAlignment(StringAlignmentFar); rf.SetFormatFlags(StringFormatFlagsNoWrap); rf.SetTrimming(StringTrimmingEllipsisCharacter); g.DrawString(StreamRowText(*sc).c_str(),-1,&fSmall,RectF((REAL)rr.right-S(320),(REAL)rr.top+S(31),(REAL)S(236),(REAL)S(16)),&rf,&qb); }
                g.DrawString(g_tracks[i].artist.c_str(),-1,&fSmall,RectF(tx,(REAL)rr.top+S(31),aw,(REAL)S(16)),&trimL,&ab);
            }
            if(cur) g.FillEllipse(&ab,(REAL)(rr.right-S(24)),(REAL)(rr.top+(rr.bottom-rr.top)/2.f-S(4)),(REAL)S(8),(REAL)S(8));
            DrawOrderArrows(g,i,&ab,&gray);
            DrawPickMark(g,i,rr);
        }
    } else
    for(size_t i=0;i<g_tracks.size() && i<R_cardRects.size();++i){
        RECT rr=R_cardRects[i];
        if(rr.right-rr.left<=0) continue;
        bool cur=(int)i==g_current;
        Rect card(rr.left,rr.top,rr.right-rr.left,rr.bottom-rr.top);
        SolidBrush cb(Color(255,9,12,25));
        Pen cp(cur?ToGdi(g_theme.accent,220):Color(255,40,44,65),1.5f);
        DrawRoundRect(g,card,16,&cb,&cp);
        BYTE ra=RunnerAlpha();
        if(cur&&g_cfg.runnerOn&&ra>6) DrawRunnerRect(g,RF(rr),16.f,ResolveCustom(g_cfg.runnerColor),(BYTE)(ra*.55f),g_runnerPhase+.5f);
        int cover=SI(142); Rect art(rr.left+SI(18),rr.top+SI(18),cover,cover);
        if(g_cfg.artShape==L"cd") DrawCoverCircle(g,GetThumb(g_tracks[i].coverPath),art,&cp,cur&&g_player.playing?g_rotation:0);
        else {SolidBrush plate(Color(255,18,21,34));DrawRoundRect(g,art,10,&plate,nullptr);Image* im=GetThumb(g_tracks[i].coverPath);if(im)g.DrawImage(im,art.X,art.Y,art.Width,art.Height);}
        if(IsOnlineTrack(g_tracks[i])) DrawOnlineTag(g,RectF((REAL)art.X,(REAL)art.Y,(REAL)art.Width,(REAL)art.Height),g_tracks[i].path);
        if(cur&&g_cfg.particlesOn&&g_player.playing&&FxOn()){
            GraphicsPath cpt;
            if(g_cfg.artShape==L"cd") cpt.AddEllipse((REAL)art.X+3,(REAL)art.Y+3,(REAL)art.Width-6,(REAL)art.Height-6); else cpt.AddRectangle(Rect(art.X+3,art.Y+3,art.Width-6,art.Height-6));
            Region cOld; g.GetClip(&cOld); g.SetClip(&cpt);
            DrawParticles(g,RectF((REAL)art.X+4,(REAL)art.Y+4,(REAL)art.Width-8,(REAL)art.Height-8),ResolveCustom(g_cfg.particlesColor),tSec,1.25f);
            g.SetClip(&cOld);
        }
        if(!g_pickMode) DrawCameraIcon(g,R_cardCoverButtons[i],ToGdi(g_theme.accent,220));
        DrawOrderArrows(g,i,&ab,&gray);
        int tx=rr.left+SI(180);
        StringFormat trimF; trimF.SetTrimming(StringTrimmingEllipsisWord); trimF.SetFormatFlags(StringFormatFlagsNoWrap);
        int tw=rr.right-SI(50)-tx;
        RectF trc((REAL)tx,(REAL)(rr.top+SI(16)),(REAL)tw,(REAL)SI(34));
        RectF tar((REAL)tx,(REAL)(rr.top+SI(52)),(REAL)tw,(REAL)SI(24));
        if(cur){ DrawMarqueeText(g,g_tracks[i].title,&fTitleBase,trc,&white,false,S(30),NowMs()); DrawMarqueeText(g,g_tracks[i].artist,&fArtistBase,tar,&ab,false,S(24),NowMs()); }
        else { g.DrawString(g_tracks[i].title.c_str(),-1,&fTitleBase,trc,&trimF,&white); g.DrawString(g_tracks[i].artist.c_str(),-1,&fArtistBase,tar,&trimF,&ab); }
        DWORD cpos=(cur&&g_player.loaded)?pos:0, clen=(cur&&g_player.loaded)?len:0;
        float cfrac=clen?std::min(1.f,(float)cpos/clen):0;
        int wl=tx, wr=rr.right-SI(22), wyy=rr.top+SI(108), wbb=rr.top+SI(144);
        int count=std::max(30,(wr-wl)/(FxOn()?5:10));
        float bw=2.2f,gap=(wr-wl-count*bw)/(float)std::max(1,count-1);
        SolidBrush dimBar2(Color(255,48,51,72));
        for(int k=0;k<count;k++){
            float v=clen?AudioWaveBar(k,count,cpos,clen,tSec):WaveIdleAt(k,count,tSec);
            float bh=std::max(4.f,(float)(wbb-wyy)*v);
            float x=(float)wl+k*(bw+gap), y=((float)wyy+(float)wbb-bh)/2.f;
            float p=count>1?(float)k/(count-1):0.f;
            g.FillRectangle(p<=cfrac?(Brush*)&ab:(Brush*)&dimBar2,x,y,bw,bh);
        }
        float sy=(float)wbb+8;
        Pen sbg(Color(75,82,85,110),2),sfg(accent,2),kn(Color(220,255,255,255),1);
        g.DrawLine(&sbg,(REAL)wl,sy,(REAL)wr,sy); g.DrawLine(&sfg,(REAL)wl,sy,(REAL)(wl+(wr-wl)*cfrac),sy);
        float kx=(REAL)wl+(REAL)(wr-wl)*cfrac;
        g.FillEllipse(&ab,kx-5,sy-5,10.f,10.f); g.DrawEllipse(&kn,kx-5,sy-5,10.f,10.f);
        g.DrawString(FormatTime(cpos).c_str(),-1,&fSmall,PointF((REAL)wl,sy+7),&gray);
        {StringFormat right; right.SetAlignment(StringAlignmentFar); g.DrawString(FormatTime(clen).c_str(),-1,&fSmall,RectF((REAL)wl,sy+7,(REAL)(wr-wl),18),&right,&gray);}
        int ccx=(rr.left+rr.right)/2, cy=rr.bottom-SI(40);
        g.DrawString(L"⇄",-1,SymFont(S(18)),RectF((REAL)(ccx-SI(120)-S(15)),(REAL)(cy-S(11)),(REAL)S(30),(REAL)S(26)),&cf,g_cfg.shuffle?(Brush*)&navB:(Brush*)&gray);
        RectF pv((REAL)(ccx-SI(66)-S(14)),(REAL)(cy-S(11)),(REAL)S(28),(REAL)S(23)); IconSkip(g,pv,&navB,false);
        g.DrawEllipse(&playP,(REAL)(ccx-SI(25)),(REAL)(cy-S(25)),(REAL)S(50),(REAL)S(50));
        {RectF pl((REAL)(ccx-S(10)),(REAL)(cy-S(12)),(REAL)S(20),(REAL)S(24)); if(cur&&g_player.playing) IconPause(g,pl,&playB); else IconPlay(g,pl,&playB);}
        RectF nx((REAL)(ccx+SI(66)-S(14)),(REAL)(cy-S(11)),(REAL)S(28),(REAL)S(23)); IconSkip(g,nx,&navB,true);
        g.DrawString(L"⟳",-1,SymFont(S(18)),RectF((REAL)(ccx+SI(120)-S(15)),(REAL)(cy-S(11)),(REAL)S(30),(REAL)S(26)),&cf,g_cfg.repeat?(Brush*)&navB:(Brush*)&gray);
        if(cur){g.DrawString(L"▶",-1,&fSmall,PointF((REAL)(rr.right-SI(28)),(REAL)(rr.bottom-SI(24))),&ab);}
        DrawPickMark(g,i,rr);
    }
    if(libClip){ DrawPlaylistCards(g,&ab,&white,&gray); g.SetClip(&libOld); }
    DrawLibBar(g,&ab,&white,&gray,accent,Color(255,150,152,165));
    if(g_tracks.empty()&&g_view!=1&&R_library.right>R_library.left){   // lista vazia: no meio da area da lista, abaixo da barra
        RectF lib=RF(R_library);
        REAL ty=R_onlineInfo.bottom>R_onlineInfo.top?(REAL)R_onlineInfo.top-S(66):lib.Y+S(80);
        std::wstring t1,t2;
        if(g_view==2){ t1=L"Esta playlist está vazia"; t2=L"Adicione da biblioteca, arquivos, uma pasta (vinculada ou não), um link ou a busca online."; }
        else if(SS().busy){ t1=L"Procurando músicas no PC..."; t2=L"Músicas, Downloads, Documentos e Área de trabalho"; }
        else { t1=L"Nenhuma música encontrada"; t2=g_cfg.musicFolder.empty()?L"Escolha a pasta das suas músicas (ou use a aba ONLINE).":L"A pasta escolhida não tem músicas: "+g_cfg.musicFolder; }
        TextCenter(g,t1,RectF(lib.X,ty,lib.Width,S(26)),S(16),&white,true);
        StringFormat ec; ec.SetAlignment(StringAlignmentCenter); ec.SetLineAlignment(StringAlignmentCenter); ec.SetFormatFlags(StringFormatFlagsNoWrap); ec.SetTrimming(StringTrimmingEllipsisCharacter);
        g.DrawString(t2.c_str(),-1,UiFont(S(11),false),RectF(lib.X+S(20),ty+S(30),lib.Width-S(40),S(20)),&ec,&gray);
        if(R_onlineInfo.right>R_onlineInfo.left&&(g_view==2||!SS().busy)) DrawPill(g,R_onlineInfo,g_view==2?L"+ ADICIONAR MÚSICAS":L"ESCOLHER PASTA",true,S(12));
        (void)fLabel;
    }
    if(g_showSettings) DrawSettings(g,w,h);
}

static void DrawVertical(Graphics& g,int w,int h){
    DrawAppBackground(g,w,h,Color(255,4,6,14)); Color accent=ToGdi(g_theme.accent); Pen ap(accent,2); SolidBrush ab(accent), white(Color(255,235,236,242)), gray(Color(255,145,147,160));
    const Font &ft=*UiFont(TextScale(17,g_cfg.titleScale),true),&fs=*UiFont(S(10),false);
    g.DrawString(L"⚙",-1,SymFont(S(26)),PointF((float)R_gear.left,(float)R_gear.top),&gray);   // no vertical o pill AUTO ocupa o canto do "REMIX"
    int ar=R_art.right-R_art.left; Rect art(R_art.left,R_art.top,ar,ar);
    if(g_cfg.artShape==L"square"){
        SolidBrush plate(Color(255,16,19,32)); DrawRoundRect(g,art,14,&plate,&ap);
        if(g_coverImg&&g_coverImg->GetLastStatus()==Ok){ Region old; g.GetClip(&old); g.SetClip(Rect(art.X+3,art.Y+3,art.Width-6,art.Height-6)); g.DrawImage(g_coverImg,art.X+3,art.Y+3,art.Width-6,art.Height-6); g.SetClip(&old); }
    } else DrawBigCd(g,g_coverImg,art,&ap,g_player.playing?g_rotation:0);
    if(g_cfg.particlesOn&&FxOn()){
        Region pOld; g.GetClip(&pOld);
        GraphicsPath vp; Rect vclip(art.X+3,art.Y+3,art.Width-6,art.Height-6);
        if(g_cfg.artShape==L"cd") vp.AddEllipse((REAL)vclip.X,(REAL)vclip.Y,(REAL)vclip.Width,(REAL)vclip.Height); else vp.AddRectangle(vclip);
        g.SetClip(&vp);
        DrawParticles(g,RectF((REAL)art.X+4,(REAL)art.Y+4,(REAL)art.Width-8,(REAL)art.Height-8),ResolveCustom(g_cfg.particlesColor),NowMs()/1000.f,1.6f);
        g.SetClip(&pOld);
    }
    DrawCameraIcon(g,R_verticalCoverButton,ToGdi(g_theme.accent,220));
    DrawShapeToggle(g,&ab,&white,&gray);
    if(R_autoTgl.right>R_autoTgl.left) DrawPill(g,R_autoTgl,L"AUTO",g_cfg.autoplay,S(11));
    DrawChromeButtons(g);
    const Track* ct=(g_current>=0&&g_current<(int)g_tracks.size())?&g_tracks[(size_t)g_current]:(g_nowPlayingValid?&g_nowPlaying:nullptr);
    std::wstring title=ct?ct->title:L"Nenhuma musica"; std::wstring artist=ct?ct->artist:L"—";
    StringFormat sfC;sfC.SetAlignment(StringAlignmentCenter);sfC.SetLineAlignment(StringAlignmentCenter);
    {
        int vy0=R_art.bottom+(int)S(8);
        int vB=R_shuffle.top-(int)S(8);
        int vAvail=vB-vy0; if(vAvail<(int)S(40)) vAvail=(int)S(40);
        float vfth=TextScale(17,g_cfg.titleScale), vfah=TextScale(12,g_cfg.artistScale);
        int vth=std::max((int)S(20),std::min((int)(vfth*1.35f)+(int)S(4),vAvail*55/100));
        int vah=std::max((int)S(16),vAvail-vth-(int)S(2));
        const Font &ftFit=*UiFont(std::min(vfth,(float)vth*0.78f),true),&faFit=*UiFont(std::min(vfah,(float)vah*0.80f),false);
        DrawMarqueeText(g,title,&ftFit,RectF(20,(REAL)vy0,(REAL)w-40,(REAL)vth),&white,true,S(30),NowMs());
        int vay=vy0+vth+(int)S(2);
        RectF arV(20,(REAL)vay,(REAL)w-40,(REAL)vah);
        DrawMarqueeText(g,artist,&faFit,arV,&ab,true,S(26),NowMs());
        float mwV=MeasureW(g,artist,std::min(vfah,(float)vah*0.80f),false);
        REAL aw=mwV>2.f?std::min(mwV,(REAL)(w/2-S(20))):(REAL)S(40);
        REAL px=(REAL)w/2+aw/2+S(4);
        int pencilH=std::min(vah,(int)(std::min(vfah,(float)vah*0.80f)*1.9f));   // altura da linha do artista
        R_pencilVert={(LONG)(px-S(6)),(LONG)(vay),(LONG)(px+S(18)),(LONG)(vay+pencilH)};
        g.DrawString(L"✎",-1,SymFont(S(13)),RF(R_pencilVert),&sfC,&gray);
    }
    DWORD pos=g_player.loaded?g_player.GetPositionMs():0,len=g_player.loaded?g_player.GetLengthMs():0;float frac=len?std::min(1.f,(float)pos/len):0;
    COLORREF cNav=ResolveCustom(g_cfg.btnNavColor),cPlay=ResolveCustom(g_cfg.btnPlayColor);SolidBrush navB(ToGdi(cNav));Pen playP(ToGdi(cPlay),2.2f);SolidBrush playB(ToGdi(cPlay));
    float tSec=NowMs()/1000.0f;
    int sl=R_seek.left,srx=R_seek.right,sw=srx-sl;int count=std::max(28,sw/(FxOn()?5:10));float bw=2.4f,gap=(sw-count*bw)/(float)std::max(1,count-1),top=(float)R_seek.top,bottom=(float)R_seek.bottom;
    {SolidBrush dimBar(Color(255,50,52,70));
    for(int i=0;i<count;i++){float p=count==1?0.f:(float)i/(count-1),bh=std::max(4.f,(bottom-top)*(len?AudioWaveBar(i,count,pos,len,tSec):WaveIdleAt(i,count,tSec))),x=sl+i*(bw+gap),y=(top+bottom-bh)/2;g.FillRectangle(p<=frac?(Brush*)&ab:(Brush*)&dimBar,(REAL)x,(REAL)y,(REAL)bw,(REAL)bh);}}
    float sy=bottom+8;Pen sbg(Color(75,85,87,110),3),sfg(accent,3),kn(Color(220,255,255,255),1);g.DrawLine(&sbg,(REAL)sl,(REAL)sy,(REAL)srx,(REAL)sy);g.DrawLine(&sfg,(REAL)sl,(REAL)sy,(REAL)(sl+sw*frac),(REAL)sy);float kx=sl+sw*frac;g.FillEllipse(&ab,(REAL)(kx-6),(REAL)(sy-6),(REAL)12.f,(REAL)12.f);g.DrawEllipse(&kn,(REAL)(kx-6),(REAL)(sy-6),(REAL)12.f,(REAL)12.f);g.DrawString(FormatTime(pos).c_str(),-1,&fs,PointF((float)sl,(float)sy+10),&gray);{StringFormat sfR;sfR.SetAlignment(StringAlignmentFar);g.DrawString(FormatTime(len).c_str(),-1,&fs,RectF((REAL)sl,(REAL)sy+10,(REAL)sw,18),&sfR,&gray);}
    g.DrawString(L"⇄",-1,SymFont(S(13)),RF(R_shuffle),&sfC,g_cfg.shuffle?(Brush*)&navB:(Brush*)&gray);
    {RectF pv((REAL)(R_prev.left+4),(REAL)(R_prev.top+9),(REAL)(R_prev.right-R_prev.left-8),(REAL)(R_prev.bottom-R_prev.top-18)); IconSkip(g,pv,&navB,false);}
    g.DrawEllipse(&playP,(REAL)R_play.left,(REAL)R_play.top,(REAL)(R_play.right-R_play.left),(REAL)(R_play.bottom-R_play.top));
    if(g_converting){ Pen spn(ToGdi(g_theme.accent),3.f); g.DrawArc(&spn,(REAL)R_play.left-S(4),(REAL)R_play.top-S(4),(REAL)(R_play.right-R_play.left)+S(8),(REAL)(R_play.bottom-R_play.top)+S(8),(REAL)(NowMs()%1000)*0.36f,100.f); }   // conectando/convertendo
    {RectF pl((REAL)(R_play.left+17),(REAL)(R_play.top+17),(REAL)(R_play.right-R_play.left-34),(REAL)(R_play.bottom-R_play.top-34)); if(g_player.playing) IconPause(g,pl,&playB); else IconPlay(g,pl,&playB);}
    {RectF nx((REAL)(R_next.left+4),(REAL)(R_next.top+9),(REAL)(R_next.right-R_next.left-8),(REAL)(R_next.bottom-R_next.top-18)); IconSkip(g,nx,&navB,true);}
    g.DrawString(L"⟳",-1,SymFont(S(13)),RF(R_repeat),&sfC,g_cfg.repeat?(Brush*)&navB:(Brush*)&gray);
    DrawVolumeBar(g,&ab,&gray,4);
    BYTE a=LedAlpha(g_glowPhase,35,180);COLORREF cLed=ResolveCustom(g_cfg.ledColor);for(int i=0;i<4;i++){Pen p(ToGdi(cLed,(BYTE)(a/(i+1))),1+i);g.DrawRectangle(&p,i,i,w-1-i*2,h-1-i*2);}
    if(g_cfg.runnerOn&&FxOn()){
        BYTE ra=RunnerAlpha(); COLORREF rc=ResolveCustom(g_cfg.runnerColor);
        if(ra>6){
            DrawRunnerRect(g,RectF(2,2,(REAL)w-4,(REAL)h-4),10.f,rc,ra,g_runnerPhase);
            RectF artRing((REAL)art.X-6,(REAL)art.Y-6,(REAL)art.Width+12,(REAL)art.Height+12);
            if(g_cfg.artShape==L"square") DrawRunnerRect(g,artRing,14.f,rc,ra,g_runnerPhase+.25f); else DrawRunnerCircle(g,artRing,rc,ra,g_runnerPhase+.25f);
            RectF pring((REAL)R_play.left-5,(REAL)R_play.top-5,(REAL)(R_play.right-R_play.left)+10,(REAL)(R_play.bottom-R_play.top)+10);
            DrawRunnerCircle(g,pring,rc,(BYTE)(ra*.8f),g_runnerPhase+.6f);
        }
    }
    if(g_showSettings) DrawSettings(g,w,h);
}

static void DrawSettings(Graphics& g,int w,int h){
    SolidBrush ov(Color(255,5,7,17)); g.FillRectangle(&ov,0,0,w,h);
    RECT rp=R_settingsPanel; int px=rp.left,py=rp.top,pw=rp.right-rp.left,ph=rp.bottom-rp.top;
    Color accent=ToGdi(g_theme.accent);
    SolidBrush white(Color(255,235,236,242)), gray(Color(255,150,152,165)), ab(accent), dim(Color(255,30,33,48));
    const Font *h1=UiFont(22,true),*lab=UiFont(13,true),*sm=UiFont(11,false),*st=UiFont(14,true),*fBtn=UiFont(12,true);
    StringFormat sfCC; sfCC.SetAlignment(StringAlignmentCenter); sfCC.SetLineAlignment(StringAlignmentCenter);
    g.DrawString(L"CONFIGURAÇÕES",-1,h1,PointF((REAL)(px+28),(REAL)(py+20)),&ab);
    g.DrawString(L"×",-1,SymFont(S(20)),PointF((REAL)(R_settingsClose.left+8),(REAL)(py+14)),&white);
    Region oldClip; g.GetClip(&oldClip);
    g.SetClip(Rect(px+6,py+56,pw-12,ph-64));
    g.TranslateTransform(0,-(REAL)g_setScroll);
    auto btn=[&](RECT rr,const std::wstring& t,bool on){
        Rect b(rr.left,rr.top,rr.right-rr.left,rr.bottom-rr.top);
        if(b.Width<10||b.Height<10) return;
        if(on){ RectF glow((REAL)b.X-2,(REAL)b.Y-2,(REAL)b.Width+4,(REAL)b.Height+4); Pen gp(ToGdi(g_theme.accent,60),4.f); DrawRoundRect(g,glow,14,nullptr,&gp); }
        SolidBrush fb(on?ToGdi(g_theme.accent,78):Color(255,18,21,38));
        Pen bp(on?ToGdi(g_theme.accent):Color(255,64,69,97),on?2.f:1.3f);
        DrawRoundRect(g,b,10,&fb,&bp);
        RectF hl(b.X+4,(REAL)b.Y+2,(REAL)b.Width-8,(REAL)b.Height*0.44f);
        SolidBrush hb(Color(on?30:12,255,255,255)); DrawRoundRect(g,hl,7,&hb,nullptr);
        StringFormat cfB; cfB.SetAlignment(StringAlignmentCenter); cfB.SetLineAlignment(StringAlignmentCenter); cfB.SetFormatFlags(StringFormatFlagsNoWrap); cfB.SetTrimming(StringTrimmingEllipsisCharacter);
        g.DrawString(t.c_str(),-1,fBtn,RectF(b.X,b.Y,(REAL)b.Width,(REAL)b.Height),&cfB,on?(Brush*)&white:(Brush*)&gray);
    };
    {
        SolidBrush sfb(Color(255,12,15,29)); Pen sbp(Color(255,38,42,64),1.f); Pen dv(Color(255,32,36,56),1.f);
        for(auto&s:g_setSections){ RectF sr=RF(s.first); DrawRoundRect(g,sr,12,&sfb,&sbp); g.DrawString(s.second.c_str(),-1,st,PointF(sr.X+16,sr.Y+12),&ab); g.DrawLine(&dv,sr.X+14,sr.Y+40,sr.X+sr.Width-14.f,sr.Y+40); }
    }
    for(auto&lp:g_setLabels) g.DrawString(lp.second.c_str(),-1,lab,PointF((REAL)lp.first.left,(REAL)lp.first.top),&ab);
    std::wstring mode=g_cfg.musicFolder.empty()?L"PADRÃO — Músicas, Downloads, Documentos, Área de trabalho":L"PASTA PERSONALIZADA";
    g.DrawString(mode.c_str(),-1,sm,PointF((REAL)R_settingsDefault.left,(REAL)(R_settingsDefault.top-18)),&white);
    btn(R_settingsDefault,L"PADRÃO (PASTAS DO USUÁRIO)",g_cfg.musicFolder.empty());
    btn(R_settingsCustom,L"ESCOLHER PASTA",!g_cfg.musicFolder.empty());
    if(!g_cfg.musicFolder.empty()){ StringFormat tw; tw.SetFormatFlags(StringFormatFlagsNoWrap); tw.SetTrimming(StringTrimmingEllipsisPath); g.DrawString(g_cfg.musicFolder.c_str(),-1,sm,RectF((REAL)R_settingsDefault.left,(REAL)(R_settingsDefault.bottom+6),(REAL)(pw-120),(REAL)16),&tw,&gray); }
    btn(R_settingsModeSquare,L"QUADRADO",g_cfg.displayMode==L"normal"&&g_cfg.artShape==L"square");
    btn(R_settingsModeCd,L"CD",g_cfg.displayMode==L"normal"&&g_cfg.artShape==L"cd");
    btn(R_settingsModeVertical,L"VERTICAL",g_cfg.displayMode==L"vertical");
    for(size_t i=0;i<R_themeCirclesSettings.size()&&i<g_themes.size();++i){
        auto&r=R_themeCirclesSettings[i]; bool sel=g_themes[i].id==g_cfg.theme;
        SolidBrush b(ToGdi(g_themes[i].accent,90)); g.FillEllipse(&b,(REAL)r.left,(REAL)r.top,(REAL)(r.right-r.left),(REAL)(r.bottom-r.top));
        Pen p(sel?Color(255,235,236,242):ToGdi(g_themes[i].accent),sel?2.4f:1.3f); g.DrawEllipse(&p,(REAL)r.left,(REAL)r.top,(REAL)(r.right-r.left),(REAL)(r.bottom-r.top));
    }
    btn(R_setParticles,L"PARTÍCULAS: "+std::wstring(g_cfg.particlesOn?L"LIGADO":L"DESLIGADO"),g_cfg.particlesOn);
    btn(R_setGlitch,L"GLITCH",g_cfg.glitchOn);
    btn(R_setPerf,g_cfg.perfMode?L"MODO LEVE: LIGADO (PC fraco)":L"MODO LEVE: DESLIGADO",g_cfg.perfMode);
    if(R_shortcutsBox.right>R_shortcutsBox.left){
        for(int a=0;a<HK_COUNT;a++){
            RECT kr=R_hkKey[a]; if(kr.right<=kr.left) continue;
            TextTrim(g,HkLabel(a),RectF((REAL)R_shortcutsBox.left+20,(REAL)kr.top,(REAL)(kr.left-R_shortcutsBox.left-30),(REAL)(kr.bottom-kr.top)),11,a%2?&gray:&white,false,StringTrimmingEllipsisCharacter,true);
            bool cap=(g_hkCapture==a);
            btn(kr,cap?L"pressione a tecla...":HotkeyLabel(g_cfg.hk[a]),cap||g_cfg.hk[a].key!=0);
            btn(R_hkScope[a],g_cfg.hk[a].global?L"GLOBAL":L"FOCO",g_cfg.hk[a].global);
        }
        btn(R_hkReset,L"RESTAURAR PADRÕES",false);
        g.DrawString(L"Clique na tecla para trocar (Backspace limpa). FOCO = só com a janela do Remix ativa (não atrapalha jogos).",-1,sm,PointF((REAL)R_hkReset.right+16,(REAL)R_hkReset.top+2),&gray);
        g.DrawString(L"GLOBAL = funciona em 2º plano ou com outro programa na frente (registrado no Windows).",-1,sm,PointF((REAL)R_hkReset.right+16,(REAL)R_hkReset.top+20),&gray);
    }
    btn(R_setWallChoose,L"WALLPAPER: ESCOLHER IMAGEM...",false);
    btn(R_setWallClear,L"REMOVER WALLPAPER",!g_cfg.bgWallpaper.empty());
    btn(R_setCoverBlur,L"FUNDO EMBAÇADO (USA A CAPA): "+std::wstring(g_cfg.coverBlurBg?L"LIGADO":L"DESLIGADO"),g_cfg.coverBlurBg);
    btn(R_setAutoplay,g_cfg.autoplay?L"AUTOPLAY: LIGADO":L"AUTOPLAY: DESLIGADO",g_cfg.autoplay);
    btn(R_setSort,L"ORDEM: "+SortModeName(g_cfg.sortMode)+L" ▼",g_cfg.sortMode==L"manual");
    btn(R_setSortDir,g_cfg.sortDesc?L"DECRESCENTE":L"CRESCENTE",false);
    btn(R_setBgClose,g_cfg.bgOnClose?L"FECHAR: CONTINUA TOCANDO":L"FECHAR: ENCERRA O APP",g_cfg.bgOnClose);
    btn(R_setSysMedia,g_cfg.sysMedia?L"CONTROLES DO SISTEMA: SIM":L"CONTROLES DO SISTEMA: NÃO",g_cfg.sysMedia);
    btn(R_setQuit,L"SAIR DO REMIX (Ctrl+Q)",false);
    if(R_setOnMode.right>R_setOnMode.left){   // ONLINE
        bool probed=OT().probed.load(); bool okT=probed&&YtdlpOk()&&FfmpegOk(); std::wstring tools;
        if(!probed){ tools=L"Verificando yt-dlp, ffmpeg e node..."; EnsureToolsAsync(); }
        else { std::lock_guard<std::mutex> lk(OT().m);
            tools=(OT().ytdlp.empty()&&OT().ytPython.empty())?L"yt-dlp: NÃO ENCONTRADO":L"yt-dlp "+OT().vYt;
            tools+=OT().ffmpeg.empty()?L"   ·   ffmpeg: NÃO ENCONTRADO":L"   ·   ffmpeg "+OT().vFf;
            tools+=OT().jsName.empty()?(OT().jsOld.empty()?std::wstring(L"   ·   Deno/Node.js: não (o YouTube pode falhar)"):L"   ·   "+OT().jsOld+L" é antigo (precisa Deno 2.3+ ou Node 22+)"):L"   ·   "+OT().jsName+L" "+OT().vJs;
            if(!OT().spotdl.empty()) tools+=L"   ·   spotdl "+OT().vSpot; }
        SolidBrush warn(Color(255,235,150,110));
        TextTrim(g,tools,RectF((REAL)R_setOnMode.left,(REAL)R_setOnMode.top-24,(REAL)(R_setOnFmt.right-R_setOnMode.left),18),11,okT||!probed?&white:&warn,false,StringTrimmingEllipsisCharacter);
        btn(R_setOnMode,g_cfg.onlineMode==L"download"?L"AO TOCAR: BAIXAR":L"AO TOCAR: STREAMING",g_cfg.onlineMode==L"download");
        btn(R_setOnFmt,L"FORMATO: "+std::wstring(g_cfg.onlineFormat==L"original"?L"ORIGINAL":(g_cfg.onlineFormat==L"m4a"?L"M4A":L"MP3")),false);
        static const wchar_t* srcs[3]={L"BUSCA: YOUTUBE MUSIC",L"BUSCA: YOUTUBE",L"BUSCA: SOUNDCLOUD"};
        btn(R_setOnSrc,srcs[std::max(0,std::min(2,g_cfg.onlineSource))],false);
        btn(R_setOnFolder,L"PASTA DOS DOWNLOADS...",!g_cfg.downloadFolder.empty());
        TextTrim(g,L"Downloads em "+OnlineDownloadBaseCached()+L"   ·   streaming fica só na memória (fechar o app não deixa arquivo pela metade)",RectF((REAL)R_setOnSrc.left,(REAL)R_setOnSrc.bottom+8,(REAL)(R_setOnFolder.right-R_setOnSrc.left),16),11,&gray,false,StringTrimmingEllipsisPath);
        btn(R_setOnRecheck,L"PROCURAR DE NOVO",false);
        TextTrim(g,okT?L"Spotify, Deezer e Apple Music: o Remix lê a lista e acha cada música no YouTube Music.":L"Rode o INSTALAR-DEPENDENCIAS.bat (na pasta do Remix.exe): instala yt-dlp, FFmpeg e Deno pelo winget.",
            RectF((REAL)R_setOnRecheck.right+14,(REAL)R_setOnRecheck.top,(REAL)(R_setOnFolder.right-R_setOnRecheck.right-14),(REAL)(R_setOnRecheck.bottom-R_setOnRecheck.top)),11,&gray,false,StringTrimmingEllipsisCharacter,true);
    }
    g.DrawString(L"Controles do sistema: teclas de mídia e o ícone na bandeja (ao lado do relógio).",-1,sm,PointF((REAL)(R_setQuit.right+14),(REAL)(R_setQuit.top+10)),&gray);
    g.DrawString(g_cfg.autoplay?L"Ao acabar uma musica, toca a proxima (ordem da lista ou aleatorio com ⇄).":L"Ao acabar uma musica, para. Toque a proxima manualmente.",-1,sm,PointF((REAL)R_setAutoplay.left,(REAL)(R_setAutoplay.bottom+8)),&gray);
    g.DrawString(g_cfg.sortMode==L"manual"?L"Ordem manual: use as setas ▲▼ nas faixas (ou Ctrl+↑/↓ na faixa atual). Salva em order.ini.":L"A ordem escolhida fica salva e vale para o autoplay e para ◀ ▶.",-1,sm,PointF((REAL)R_setAutoplay.left,(REAL)(R_setAutoplay.bottom+26)),&gray);
    btn(R_setEqOn,g_cfg.eqOn?L"EQUALIZADOR: LIGADO":L"EQUALIZADOR: DESLIGADO",g_cfg.eqOn);
    btn(R_setEqReset,L"ZERAR",false);
    auto isEq=[&](int id){return id>=Z_EQ_BASE&&id<Z_EQ_BASE+8;};
    auto sval=[&](int id)->int{if(isEq(id))return g_cfg.eq[id-Z_EQ_BASE];switch(id){case Z_UI_SCALE:return g_cfg.uiScale;case Z_TITLE_SCALE:return g_cfg.titleScale;case Z_ARTIST_SCALE:return g_cfg.artistScale;case Z_VERTICAL_SCALE:return g_cfg.verticalScale;case Z_PLAYER_SIZE_SLIDER:return g_cfg.playerScale;case Z_LED_BRIGHT:return g_cfg.ledBrightness;case Z_RUNNER_SPEED:return g_cfg.runnerSpeed;case Z_PART_SPEED:return g_cfg.particlesSpeed;case Z_CD_SPEED:return g_cfg.cdSpeed;default:return g_cfg.ledSpeed;}};
    static const wchar_t* eqNames[8]={L"60 Hz (sub-grave)",L"150 Hz (grave)",L"400 Hz",L"1 kHz (voz)",L"2.5 kHz",L"6 kHz (presença)",L"10 kHz",L"15 kHz (brilho)"};
    auto sname=[&](int id)->const wchar_t*{if(isEq(id))return eqNames[id-Z_EQ_BASE];switch(id){case Z_UI_SCALE:return L"Escala geral";case Z_TITLE_SCALE:return L"Nome da musica";case Z_ARTIST_SCALE:return L"Nome do artista";case Z_VERTICAL_SCALE:return L"Escala do vertical";case Z_PLAYER_SIZE_SLIDER:return L"Tamanho do player";case Z_LED_BRIGHT:return L"Brilho do LED";case Z_RUNNER_SPEED:return L"Velocidade da linha (corredor)";case Z_PART_SPEED:return L"Velocidade das particulas";case Z_CD_SPEED:return L"Velocidade de giro do CD";default:return L"Velocidade do LED";}};
    for(auto&s:g_setSliders){
        int val=sval(s.id); bool eq=isEq(s.id);
        SolidBrush knob((eq&&!g_cfg.eqOn)?Color(255,90,94,118):accent);
        g.DrawString(sname(s.id),-1,sm,PointF((REAL)s.r.left,(REAL)s.r.top-16),&gray);
        Rect tr(s.r.left,s.r.top,s.r.right-s.r.left,7); DrawRoundRect(g,tr,3,&dim,nullptr);
        float q=(float)(val-s.minv)/std::max(1,s.maxv-s.minv);
        int fill=(int)((s.r.right-s.r.left)*std::max(0.f,std::min(1.f,q)));
        if(eq){ int mid=(s.r.right-s.r.left)/2; int a=std::min(mid,fill), b=std::max(mid,fill);
            if(b-a>1){Rect fr(s.r.left+a,s.r.top,b-a,7);DrawRoundRect(g,fr,3,&knob,nullptr);}
            Pen mp(Color(255,70,74,95),1); g.DrawLine(&mp,(REAL)(s.r.left+mid),(REAL)s.r.top-3,(REAL)(s.r.left+mid),(REAL)s.r.top+10);
        } else if(fill>2){Rect fr(s.r.left,s.r.top,fill,7);DrawRoundRect(g,fr,3,&ab,nullptr);}
        g.FillEllipse(&knob,(REAL)(s.r.left+fill-7),(REAL)(s.r.top-3),(REAL)14.f,(REAL)14.f);
        wchar_t bv[24]; if(eq) swprintf(bv,24,L"%+d dB",val); else swprintf(bv,24,L"%d%%",val);
        g.DrawString(bv,-1,sm,PointF((REAL)(s.r.right+10),(REAL)(s.r.top-5)),&white);
    }
    btn(R_setEffect,L"Efeito: "+g_cfg.ledEffect+L" ▼",false);
    auto rowIdAt=[&](size_t i)->std::wstring{ if(i==0) return L""; size_t nt=g_themes.size(); if(i<=nt) return g_themes[i-1].id; size_t bi=i-nt-1; return bi<15?std::wstring(g_brightIds[bi]):std::wstring(); };
    auto colorRowDraw=[&](std::vector<RECT>&v,const std::wstring& cur){
        bool dimmed=g_cfg.autoColor;
        for(size_t i=0;i<v.size();++i){
            std::wstring id=rowIdAt(i);
            COLORREF c=(i==0||id.empty())?g_theme.accent:(id[0]==L'#'?ParseHexColor(id):FindTheme(g_themes,id).accent);
            RectF rr=RF(v[i]);
            if(i==0){ SolidBrush b(Color(dimmed?60:80,GetRValue(c),GetGValue(c),GetBValue(c))); g.FillEllipse(&b,rr.X,rr.Y,rr.Width,rr.Height); g.DrawString(L"T",-1,sm,rr,&sfCC,&white); }
            else { SolidBrush b(Color((BYTE)(dimmed?55:150),GetRValue(c),GetGValue(c),GetBValue(c))); g.FillEllipse(&b,rr.X,rr.Y,rr.Width,rr.Height); }
            bool sel=((i==0&&cur.empty())||(i>0&&!cur.empty()&&cur==id));
            Pen p(sel?Color(255,240,242,250):Color((BYTE)(dimmed?60:255),90,94,115),sel?2.2f:1.1f); g.DrawEllipse(&p,rr.X,rr.Y,rr.Width,rr.Height);
        }
    };
    btn(R_setAutoColor,g_cfg.autoColor?L"CORES: AUTOMÁTICO (segue o tema)":L"CORES: MANUAIS",g_cfg.autoColor);
    colorRowDraw(R_playColors,g_cfg.btnPlayColor); colorRowDraw(R_navColors,g_cfg.btnNavColor);
    btn(R_setRunnerToggle,g_cfg.runnerOn?L"LIGADO":L"DESLIGADO",g_cfg.runnerOn);
    colorRowDraw(R_runColors,g_cfg.runnerColor); colorRowDraw(R_partColors,g_cfg.particlesColor); colorRowDraw(R_ledColors,g_cfg.ledColor);
    g.ResetTransform(); g.SetClip(&oldClip);
}

static void DrawSplash(Graphics& g,int w,int h){SolidBrush b(Color(255,3,4,10));g.FillRectangle(&b,0,0,w,h);if(g_splashImg&&g_splashImg->GetLastStatus()==Ok){float iw=(float)g_splashImg->GetWidth(),ih=(float)g_splashImg->GetHeight(),sc=std::min((w-80)/iw,(h-100)/ih),dw=iw*sc,dh=ih*sc;g.DrawImage(g_splashImg,(w-dw)/2,(h-dh)/2-8,dw,dh);} }

static void DrawArtistEditor(Graphics& g,int w,int h){
    SolidBrush ov(Color(170,2,4,10)); g.FillRectangle(&ov,0,0,w,h);
    LayoutEditor(w,h);
    int bx=R_editBox.left,by=R_editBox.top,bw=R_editBox.right-R_editBox.left,bh=R_editBox.bottom-R_editBox.top;
    RectF box((REAL)bx,(REAL)by,(REAL)bw,(REAL)bh);
    SolidBrush pb(Color(255,12,15,30)); Pen apn(ToGdi(g_theme.accent),2); DrawRoundRect(g,box,14,&pb,&apn);
    const Font *lab=UiFont(S(13),true),*sm=UiFont(S(10),false),*txt=UiFont(S(14),false);
    SolidBrush abr(ToGdi(g_theme.accent)), white(Color(255,235,236,242)), gray(Color(255,150,152,165));
    const wchar_t* etitle=g_editMode==1?L"RENOMEAR ARQUIVO (no disco)":g_editMode==2?L"NOVA PLAYLIST":g_editMode==4?L"COLAR LINK NA PLAYLIST":g_editMode==5?L"NOVA PLAYLIST A PARTIR DE UM LINK":g_editMode==3?L"RENOMEAR PLAYLIST":L"EDITAR NOME DO ARTISTA";
    g.DrawString(etitle,-1,lab,PointF((REAL)(bx+22),(REAL)(by+18)),&abr);
    std::wstring t;
    if(g_editMode==2) t=L"Nome da playlist (as músicas ficam onde estão; só o caminho é guardado)";
    else if(g_editMode==4||g_editMode==5) t=L"Música, álbum ou playlist do Spotify, YouTube / YouTube Music, Deezer, Apple Music ou SoundCloud  (Ctrl+V cola)";
    else if(g_editMode==3) t=L"Playlist: "+(g_editTrack>=0&&g_editTrack<(int)g_playlists.size()?g_playlists[(size_t)g_editTrack].name:L"");
    else t=(g_editMode==1?L"Arquivo: ":L"Faixa: ")+(g_editTrack>=0&&g_editTrack<(int)g_tracks.size()?(g_editMode==1?std::filesystem::path(g_tracks[g_editTrack].path).filename().wstring():g_tracks[g_editTrack].title):L"");
    TextTrim(g,t,RectF((REAL)(bx+22),(REAL)(by+46),(REAL)(bw-44),16),S(10),&gray,false,StringTrimmingEllipsisCharacter);
    RectF line((REAL)(bx+22),(REAL)(by+74),(REAL)(bw-44),(REAL)36);
    SolidBrush lb(Color(255,20,23,38)); DrawRoundRect(g,line,8,&lb,nullptr);
    StringFormat sfL; sfL.SetFormatFlags(StringFormatFlagsNoWrap);
    PointF tp((REAL)(bx+32),(REAL)(by+82));
    Region old; g.GetClip(&old); g.SetClip(line);
    RectF emw; g.MeasureString(g_editBuf.c_str(),-1,txt,tp,&sfL,&emw); REAL eoff=emw.Width>(REAL)(bw-64)?emw.Width-(REAL)(bw-64):0;   // link longo: mostra o final
    g.DrawString(g_editBuf.c_str(),-1,txt,PointF(tp.X-eoff,tp.Y),&sfL,&white);
    if((NowMs()/500)%2==0){ REAL cx=tp.X+(emw.Width>1.f?emw.Width-eoff:0.f); if(cx>tp.X+(REAL)(bw-64)) cx=tp.X+(REAL)(bw-64); Pen cp(Color(255,235,236,242),2); g.DrawLine(&cp,cx+3,(REAL)(by+80),cx+3,(REAL)(by+102)); }
    g.SetClip(&old);
    auto ebtnC=[&](RECT r,const wchar_t*s,bool primary){ RectF b=RF(r); SolidBrush fb(primary?ToGdi(g_theme.accent,60):Color(255,24,27,42)); Pen p(primary?ToGdi(g_theme.accent):Color(255,70,74,95),1.5f); DrawRoundRect(g,b,8,&fb,&p); TextCenter(g,s,b,S(10),primary?(Brush*)&abr:(Brush*)&gray); };
    ebtnC(R_editSave,L"SALVAR",true); ebtnC(R_editCancel,L"CANCELAR",false);
    g.DrawString(L"ENTER salva   ·   ESC cancela",-1,sm,PointF((REAL)(bx+22),(REAL)(by+bh-40)),&gray);
}
static void DrawImgMenu(Graphics& g,int w,int h){
    SolidBrush ov(Color(150,2,4,10));g.FillRectangle(&ov,0,0,w,h);
    SolidBrush pb(Color(255,12,15,30));Pen apn(ToGdi(g_theme.accent),1.6f); DrawRoundRect(g,RF(R_imgBox),(int)S(12),&pb,&apn);
    { SolidBrush fb(Color(255,20,23,38));Pen ln(Color(255,58,62,86),1.f); DrawRoundRect(g,RF(R_imgLocal),(int)S(8),&fb,&ln); SolidBrush wh(Color(255,235,236,242)); TextCenter(g,L"Imagem deste computador",RF(R_imgLocal),S(11),&wh,true); }
    { SolidBrush fb(Color(255,20,23,38));Pen ln(ToGdi(g_theme.accent),1.2f); DrawRoundRect(g,RF(R_imgWeb),(int)S(8),&fb,&ln); SolidBrush abr(ToGdi(g_theme.accent)); TextCenter(g,L"Buscar na internet",RF(R_imgWeb),S(11),&abr,true); }
}
static void DrawWebPick(Graphics& g,int w,int h){
    WebPick& wb=WP();
    SolidBrush ov(Color(190,2,4,10));g.FillRectangle(&ov,(REAL)0,(REAL)0,(REAL)w,(REAL)h);
    RECT&b=wb.box;int bw=b.right-b.left,bh=b.bottom-b.top;
    SolidBrush pb(Color(255,10,13,26));Pen apn(ToGdi(g_theme.accent),1.8f); DrawRoundRect(g,RF(b),(int)S(14),&pb,&apn);
    const Font *lab=UiFont(S(11),true),*sm=UiFont(S(10),false);
    SolidBrush white(Color(255,235,236,242)),gray(Color(255,150,152,165)),ab(ToGdi(g_theme.accent));
    StringFormat cf;cf.SetAlignment(StringAlignmentCenter);cf.SetLineAlignment(StringAlignmentCenter);
    StringFormat lf;lf.SetFormatFlags(StringFormatFlagsNoWrap);lf.SetTrimming(StringTrimmingEllipsisCharacter);
    {
        RectF q=RF(wb.qbox);
        SolidBrush qb(Color(255,18,21,36));Pen qp(wb.editing?ToGdi(g_theme.accent):Color(255,58,62,88),wb.editing?1.8f:1.2f); DrawRoundRect(g,q,(int)S(9),&qb,&qp);
        std::wstring show=wb.query.empty()&&!wb.editing?L"Pesquisar (nome da musica)...":wb.query;
        g.DrawString(show.c_str(),-1,sm,PointF(q.X+S(12),q.Y+(q.Height-S(16))/2.f),&lf,wb.query.empty()&&!wb.editing?(Brush*)&gray:(Brush*)&white);
        if(wb.editing&&(NowMs()/500)%2==0){ RectF mw;g.MeasureString(wb.query.c_str(),-1,sm,PointF(q.X+S(12),q.Y+(q.Height-S(16))/2.f),&lf,&mw); REAL cx=q.X+S(12)+mw.Width; Pen cp(Color(255,235,236,242),2);g.DrawLine(&cp,cx+S(3),q.Y+S(7),cx+S(3),q.Y+q.Height-S(7)); }
    }
    { RectF s=RF(wb.btnSearch); SolidBrush sb(ToGdi(g_theme.accent,55));Pen sp(ToGdi(g_theme.accent),1.4f); DrawRoundRect(g,s,(int)S(9),&sb,&sp); g.DrawString(L"BUSCAR",-1,lab,s,&cf,&ab); }
    g.DrawString(L"✕",-1,SymFont(S(13)),RF(wb.btnClose),&cf,&white);
    Region oldClip;g.GetClip(&oldClip);
    g.SetClip(Rect(b.left+(int)S(8),b.top+(int)S(54),bw-(int)S(16),bh-(int)S(116)));
    {
        std::lock_guard<std::mutex> lk(wb.m);
        for(size_t i=0;i<wb.res.size()&&i<wb.cells.size();++i){
            RECT c=wb.cells[i];if(c.right-c.left<=0)continue;
            bool sel=wb.sel==(int)i;
            RectF cell=RF(c);
            SolidBrush cb(Color(255,16,19,34));Pen cpn(sel?ToGdi(g_theme.accent):Color(255,45,49,72),sel?2.4f:1.1f); DrawRoundRect(g,cell,(int)S(8),&cb,&cpn);
            Image* im=(Image*)wb.res[i].img;
            if(im){ float iw=(REAL)im->GetWidth(),ih=(REAL)im->GetHeight(); float sc=std::min((cell.Width-S(8))/iw,(cell.Height-S(8))/ih); float dw=iw*sc,dh=ih*sc; g.DrawImage(im,cell.X+(cell.Width-dw)/2.f,cell.Y+(cell.Height-dh)/2.f,dw,dh); }
            else g.DrawString(L"...",-1,sm,cell,&cf,&gray);
        }
    }
    g.ResetClip();g.SetClip(&oldClip);
    std::wstring status; {std::lock_guard<std::mutex> lk(wb.m);status=wb.status;}
    g.DrawString(status.c_str(),-1,sm,RectF((REAL)(b.left+S(16)),(REAL)(b.bottom-S(46)),(REAL)(bw-S(270)),(REAL)S(30)),&lf,&gray);
    {
        bool can=false;{std::lock_guard<std::mutex> lk(wb.m);can=(wb.sel>=0&&!wb.downloading);}
        RectF u=RF(wb.btnUse); SolidBrush ub(can?ToGdi(g_theme.accent,80):Color(255,22,25,40)); Pen up(can?ToGdi(g_theme.accent):Color(255,70,74,95),1.6f); DrawRoundRect(g,u,(int)S(9),&ub,&up); g.DrawString(L"USAR ESSA",-1,lab,u,&cf,can?(Brush*)&ab:(Brush*)&gray);
        RectF cn=RF(wb.btnCancel); SolidBrush nb(Color(255,20,23,38));Pen np(Color(255,70,74,95),1.4f); DrawRoundRect(g,cn,(int)S(9),&nb,&np); g.DrawString(L"CANCELAR",-1,lab,cn,&cf,&gray);
    }
}
// ---- downloads (pilula no canto) e tela de busca online ----
static void DrawActivity(Graphics& g,int w,int h){
    int waiting=0; float pct=0; std::wstring title;
    if(!LayoutActivity(w,h,waiting,pct,title)) return;
    RectF b=RF(R_activity); SolidBrush bg(Color(235,12,15,30)); Pen pn(ToGdi(g_theme.accent),1.2f);
    DrawRoundRect(g,b,(int)S(10),&bg,&pn);
    RectF bar(b.X+S(10),b.Y+b.Height-S(8),b.Width-S(20),S(3));
    SolidBrush bb(Color(255,40,44,65)), fb(ToGdi(g_theme.accent)); g.FillRectangle(&bb,bar); g.FillRectangle(&fb,bar.X,bar.Y,bar.Width*std::max(0.f,std::min(1.f,pct/100.f)),bar.Height);
    wchar_t pc[16]; swprintf(pc,16,L"%.0f%%",pct);
    std::wstring t=L"↓ "+(title.empty()?std::wstring(L"na fila"):std::wstring(pc)+L"  "+title)+(waiting>0?L"   (+"+std::to_wstring(waiting)+L" na fila)":L"");
    SolidBrush wt(Color(255,235,236,242));
    TextTrim(g,t,RectF(b.X+S(12),b.Y,b.Width-S(24),b.Height-S(4)),S(11),&wt,false,StringTrimmingEllipsisCharacter,true);
}
static void DrawSpinner(Graphics& g,REAL cx,REAL cy,REAL r,Color c,REAL thick){
    Pen bg(Color(50,c.GetR(),c.GetG(),c.GetB()),thick); g.DrawEllipse(&bg,cx-r,cy-r,2*r,2*r);
    Pen fg(c,thick); fg.SetStartCap(LineCapRound); fg.SetEndCap(LineCapRound);
    g.DrawArc(&fg,cx-r,cy-r,2*r,2*r,(REAL)(NowMs()%1000)*0.36f,110.f);
}
// enquanto a busca nao volta: linhas-fantasma piscando + circulo girando + segundos
static void DrawOnlineLoading(Graphics& g,bool link,int source,ULONGLONG since){
    RectF L=RF(R_onList); REAL rowH=S(58); int rows=std::max(1,(int)(L.Height/rowH));
    for(int k=0;k<rows;k++){
        RectF row(L.X+S(8),L.Y+k*rowH,L.Width-S(16),rowH-S(6));
        int a=(int)(10+18*(0.5f+0.5f*sinf(NowMs()/260.f-k*0.7f)));
        SolidBrush rb(Color(255,16,19,34)); Pen rp(Color(255,40,44,64),1.f); DrawRoundRect(g,row,(int)S(9),&rb,&rp);
        SolidBrush bar(Color(255,(BYTE)(30+a),(BYTE)(33+a),(BYTE)(52+a)));
        REAL th=row.Height-S(12), tx=row.X+S(12)+th;
        DrawRoundRect(g,RectF(row.X+S(6),row.Y+S(6),th,th),(int)S(6),&bar,nullptr);
        DrawRoundRect(g,RectF(tx,row.Y+S(10),row.Width*(0.30f+0.05f*(k%3)),S(12)),(int)S(5),&bar,nullptr);
        DrawRoundRect(g,RectF(tx,row.Y+S(30),row.Width*0.18f,S(9)),(int)S(4),&bar,nullptr);
    }
    REAL cx=L.X+L.Width/2, cy=L.Y+L.Height*0.42f;
    SolidBrush panel(Color(235,10,13,26)); Pen pn(ToGdi(g_theme.accent,120),1.2f);
    DrawRoundRect(g,RectF(cx-S(200),cy-S(60),S(400),S(120)),(int)S(14),&panel,&pn);
    DrawSpinner(g,cx,cy-S(18),S(18),ToGdi(g_theme.accent),S(3.5f));
    static const wchar_t* srcN[3]={L"YouTube Music",L"YouTube",L"SoundCloud"};
    unsigned secs=since?(unsigned)((GetTickCount64()-since)/1000):0;
    std::wstring t=(link?std::wstring(L"Lendo o link"):L"Buscando no "+std::wstring(srcN[std::max(0,std::min(2,source))]))+L"...   "+std::to_wstring(secs)+L" s";
    SolidBrush wt(Color(255,235,236,242)), gr(Color(255,150,152,165));
    TextCenter(g,t,RectF(cx-S(190),cy+S(12),S(380),S(22)),S(12),&wt,true);
    TextCenter(g,link?L"links do Spotify podem levar uns 20 s":L"costuma levar de 2 a 6 segundos",RectF(cx-S(190),cy+S(34),S(380),S(18)),S(10),&gr,false);
}
static void DrawOnline(Graphics& g,int w,int h){
    OnlineUI& u=OU();
    LayoutOnline(w,h);
    SolidBrush ov(Color(200,2,4,10)); g.FillRectangle(&ov,0,0,w,h);
    RectF box=RF(u.box); SolidBrush pb(Color(255,10,13,26)); Pen apn(ToGdi(g_theme.accent),1.8f);
    DrawRoundRect(g,box,(int)S(14),&pb,&apn);
    SolidBrush white(Color(255,235,236,242)), gray(Color(255,150,152,165)), ab(ToGdi(g_theme.accent));
    StringFormat cf; cf.SetAlignment(StringAlignmentCenter); cf.SetLineAlignment(StringAlignmentCenter);
    int tpl=OnlineTargetPl();
    TextTrim(g,tpl>=0?L"BUSCAR ONLINE   →   "+g_playlists[(size_t)tpl].name:std::wstring(L"BUSCAR ONLINE  /  COLAR LINK"),RectF(box.X+S(18),box.Y+S(12),box.Width-S(80),S(34)),S(14),&ab,true,StringTrimmingEllipsisCharacter,true);
    g.DrawString(L"✕",-1,SymFont(S(14)),RF(u.btnClose),&cf,&white);
    {
        RectF q=RF(u.qbox); SolidBrush qb(Color(255,18,21,36)); Pen qp(u.editing?ToGdi(g_theme.accent):Color(255,58,62,88),u.editing?1.8f:1.2f);
        DrawRoundRect(g,q,(int)S(9),&qb,&qp);
        Region old; g.GetClip(&old); g.SetClip(q);
        StringFormat nf; nf.SetFormatFlags(StringFormatFlagsNoWrap);
        REAL tw=u.query.empty()?0:MeasureW(g,u.query,S(12),false), maxw=q.Width-S(24), off=tw>maxw?tw-maxw:0;
        if(u.query.empty()&&!u.editing) TextTrim(g,L"Nome da música ou artista... ou cole um link (Spotify, YouTube, Deezer, Apple Music, SoundCloud)",RectF(q.X+S(12),q.Y,maxw,q.Height),S(11),&gray,false,StringTrimmingEllipsisCharacter,true);
        else g.DrawString(u.query.c_str(),-1,UiFont(S(12),false),PointF(q.X+S(12)-off,q.Y+(q.Height-S(18))/2.f),&nf,&white);
        if(u.editing&&(NowMs()/500)%2==0){ REAL cx=q.X+S(12)+tw-off; Pen cp(Color(255,235,236,242),2); g.DrawLine(&cp,cx+S(2),q.Y+S(9),cx+S(2),q.Y+q.Height-S(9)); }
        g.SetClip(&old);
    }
    { RectF sb=RF(u.btnSearch); SolidBrush f(ToGdi(g_theme.accent,55)); Pen sp(ToGdi(g_theme.accent),1.4f); DrawRoundRect(g,sb,(int)S(9),&f,&sp); if(u.busy.load()) DrawSpinner(g,sb.X+sb.Width/2,sb.Y+sb.Height/2,S(9),ToGdi(g_theme.accent),S(2.5f)); else TextCenter(g,L"BUSCAR",sb,S(12),&ab,true); }
    static const wchar_t* srcN[3]={L"YOUTUBE MUSIC",L"YOUTUBE",L"SOUNDCLOUD"};
    for(int i=0;i<3;i++) DrawPill(g,u.src[i],srcN[i],u.source==i,S(10));
    Region oldL; g.GetClip(&oldL); g.SetClip(RF(R_onList));
    {
        std::lock_guard<std::mutex> lk(u.m);
        for(size_t i=0;i<u.res.size()&&i<u.rows.size();++i){
            RECT rr=u.rows[i]; if(rr.right<=rr.left) continue;
            const OTrack& t=u.res[i];
            RectF row=RF(rr); SolidBrush rb(Color(255,16,19,34)); Pen rp(Color(255,45,49,72),1.f); DrawRoundRect(g,row,(int)S(9),&rb,&rp);
            REAL th=row.Height-S(12); RectF art(row.X+S(6),row.Y+S(6),th,th);
            SolidBrush plate(Color(255,24,27,44)); DrawRoundRect(g,art,(int)S(6),&plate,nullptr);
            if(!t.thumb.empty()){ std::wstring tf=OnlineThumbFile(t.thumb); if(g_thumbReady.count(tf)){ Image* im=GetThumb(tf); if(im) g.DrawImage(im,art); } }
            REAL tx=art.X+art.Width+S(12), tw=(REAL)u.bPlay[i].left-tx-S(10);
            TextTrim(g,t.title.empty()?t.url:t.title,RectF(tx,row.Y+S(5),tw,S(24)),S(13),&white,true,StringTrimmingEllipsisWord,true);
            std::wstring sub=t.artist;
            if(t.dur>0){ wchar_t d[16]; swprintf(d,16,L"%d:%02d",t.dur/60,t.dur%60); sub+=(sub.empty()?L"":L"  ·  ")+std::wstring(d); }
            sub+=(sub.empty()?L"":L"  ·  ")+std::wstring(SourceName(t.src));
            TextTrim(g,sub,RectF(tx,row.Y+S(29),tw,S(18)),S(10),&ab,false,StringTrimmingEllipsisCharacter,true);
            auto ib=[&](const RECT& r,const wchar_t* s,bool primary){ RectF b=RF(r); SolidBrush f(primary?ToGdi(g_theme.accent,60):Color(255,24,27,42)); Pen p(primary?ToGdi(g_theme.accent):Color(255,70,74,95),1.2f); DrawRoundRect(g,b,(int)S(8),&f,&p); g.DrawString(s,-1,SymFont(S(14)),b,&cf,primary?(Brush*)&ab:(Brush*)&white); };
            ib(u.bPlay[i],L"▶",true); ib(u.bDl[i],L"↓",false); ib(u.bAdd[i],L"+",false);
        }
        if(u.res.empty()&&u.busy.load()) DrawOnlineLoading(g,u.fromLink,u.source,u.busySince);
    }
    g.SetClip(&oldL);
    std::wstring st; bool fromLink=false; size_t n=0;
    { std::lock_guard<std::mutex> lk(u.m); st=u.status; fromLink=u.fromLink; n=u.res.size(); }
    bool hasAll=u.btnAddAll.right>u.btnAddAll.left;
    REAL footW=hasAll?(REAL)(u.btnAddAll.left-u.box.left)-S(30):box.Width-S(32);
    REAL fx=box.X+S(16); bool busy=u.busy.load();
    if(busy){ DrawSpinner(g,fx+S(8),box.Y+box.Height-S(32),S(8),ToGdi(g_theme.accent),S(2.5f)); fx+=S(26); }
    TextTrim(g,st+(n&&!busy?L"   ·   ▶ toca  ↓ baixa  + playlist":L""),RectF(fx,box.Y+box.Height-S(52),footW-(fx-box.X-S(16)),S(40)),S(11),&gray,false,StringTrimmingEllipsisCharacter,true);
    if(hasAll) DrawPill(g,u.btnAddAll,tpl>=0?L"ADICIONAR TODAS NA PLAYLIST":(fromLink?L"SALVAR COMO PLAYLIST":L"ADICIONAR TODAS..."),true,S(11));
}
// ---- menus flutuantes / confirmacao ----
static void DrawMenuBox(Graphics& g,const RECT& box){ SolidBrush pb(Color(255,12,15,30)); Pen apn(ToGdi(g_theme.accent),1.4f); DrawRoundRect(g,RF(box),(int)S(10),&pb,&apn); }
static void DrawFolderMenu(Graphics& g,int w,int h){
    SolidBrush ov(Color(90,2,4,10)); g.FillRectangle(&ov,0,0,w,h);
    DrawMenuBox(g,R_folderBox);
    SolidBrush white(Color(255,235,236,242)), acc(ToGdi(g_theme.accent));
    for(size_t i=0;i<R_folderItems.size()&&i<g_folderItemPaths.size();++i){
        RectF r=RF(R_folderItems[i]); const std::wstring& p=g_folderItemPaths[i];
        bool cur=(!p.empty()&&p!=L"*"&&_wcsicmp(p.c_str(),g_cfg.musicFolder.c_str())==0)||(p==L"*"&&g_cfg.musicFolder.empty());
        SolidBrush fb(cur?ToGdi(g_theme.accent,50):Color(255,20,23,38)); Pen ln(cur?ToGdi(g_theme.accent):Color(255,50,54,76),1.f); DrawRoundRect(g,r,(int)S(7),&fb,&ln);
        std::wstring label=p.empty()?L"Escolher outra pasta...":(p==L"*"?L"Padrão: Músicas, Downloads, Documentos, Área de trabalho":std::filesystem::path(p).filename().wstring()+L"   ("+p+L")");
        TextTrim(g,label,RectF(r.X+S(10),r.Y,r.Width-S(16),r.Height),S(11),cur?&acc:&white,p.empty()||p==L"*",StringTrimmingEllipsisPath,true);
    }
}
static void DrawCtxMenu(Graphics& g){
    DrawMenuBox(g,R_ctxBox);
    SolidBrush white(Color(255,235,236,242)), red(Color(255,255,120,130));
    for(size_t i=0;i<R_ctxItems.size()&&i<g_ctxLabels.size();++i){
        RectF r=RF(R_ctxItems[i]); bool danger=i<g_ctxDanger.size()&&g_ctxDanger[i];
        SolidBrush fb(Color(255,20,23,38)); Pen ln(danger?Color(255,120,50,64):Color(255,50,54,76),1.f); DrawRoundRect(g,r,(int)S(6),&fb,&ln);
        TextTrim(g,g_ctxLabels[i],RectF(r.X+S(10),r.Y,r.Width-S(12),r.Height),S(11),danger?&red:&white,false,StringTrimmingEllipsisCharacter,true);
    }
}
static void DrawConfirm(Graphics& g,int w,int h){
    SolidBrush ov(Color(170,2,4,10)); g.FillRectangle(&ov,0,0,w,h);
    OpenConfirm();
    RectF box=RF(R_confirmBox);
    SolidBrush pb(Color(255,12,15,30)); Pen apn(Color(255,190,70,90),2); DrawRoundRect(g,box,14,&pb,&apn);
    SolidBrush red(Color(255,255,120,130)), white(Color(255,235,236,242)), gray(Color(255,150,152,165));
    TextAt(g,g_confirmKind==1?L"EXCLUIR PLAYLIST":L"EXCLUIR MÚSICA",box.X+22,box.Y+18,S(13),&red,true);
    TextTrim(g,g_confirmText,RectF(box.X+22,box.Y+52,box.Width-44,40),S(11),&white,false,StringTrimmingEllipsisCharacter);
    TextAt(g,L"O arquivo vai para a Lixeira (da para recuperar).",box.X+22,box.Y+82,S(10),&gray);
    auto cbtn=[&](RECT r,const wchar_t* t,bool primary){ RectF b=RF(r); SolidBrush fb(primary?Color(255,90,30,40):Color(255,24,27,42)); Pen p(primary?Color(255,220,80,100):Color(255,70,74,95),1.5f); DrawRoundRect(g,b,8,&fb,&p); TextCenter(g,t,b,S(10),primary?(Brush*)&white:(Brush*)&gray); };
    cbtn(R_confirmYes,L"EXCLUIR",true); cbtn(R_confirmNo,L"CANCELAR",false);
}
