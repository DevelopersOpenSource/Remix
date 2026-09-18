#pragma once
// Desenho do estilo REMIX (1.6) na casca Windows (GDI+). Mesma aparencia do
// linux/app_draw_remix.h: lateral com a biblioteca, tela inicial com fileiras e
// o player numa barra embaixo. A geometria vem de BuildLayoutRemix.

// ---------------------------------------------------------------- icones ---
static void RxIconCasa(Graphics& g,const RectF& r,const Color& c){
    Pen p(c,1.8f);
    float cx=r.X+r.Width/2, t=r.Y+r.Height*.12f, b=r.Y+r.Height*.86f, hw=r.Width*.42f;
    g.DrawLine(&p,cx,t,cx-hw,r.Y+r.Height*.46f);
    g.DrawLine(&p,cx,t,cx+hw,r.Y+r.Height*.46f);
    g.DrawLine(&p,cx-hw*.78f,r.Y+r.Height*.50f,cx-hw*.78f,b);
    g.DrawLine(&p,cx+hw*.78f,r.Y+r.Height*.50f,cx+hw*.78f,b);
    g.DrawLine(&p,cx-hw*.78f,b,cx+hw*.78f,b);
}
static void RxIconLupa(Graphics& g,const RectF& r,const Color& c){
    Pen p(c,1.8f);
    float cx=r.X+r.Width*.44f, cy=r.Y+r.Height*.42f, rad=r.Width*.28f;
    g.DrawEllipse(&p,cx-rad,cy-rad,rad*2,rad*2);
    Pen p2(c,2.f);
    g.DrawLine(&p2,cx+rad*.72f,cy+rad*.72f,r.X+r.Width*.86f,r.Y+r.Height*.86f);
}
static void RxIconLista(Graphics& g,const RectF& r,const Color& c){
    Pen p(c,1.8f);
    for(int i=0;i<3;i++){
        float y=r.Y+r.Height*(.26f+i*.24f);
        g.DrawLine(&p,r.X+r.Width*.16f,y,r.X+r.Width*.22f,y);
        g.DrawLine(&p,r.X+r.Width*.32f,y,r.X+r.Width*.86f,y);
    }
}
static void RxIconSom(Graphics& g,const RectF& r,const Color& c,bool mudo){
    SolidBrush b(c); Pen p(c,1.7f);
    float x=r.X+r.Width*.22f, cy=r.Y+r.Height/2, hh=r.Height*.16f;
    g.FillRectangle(&b,x,cy-hh,r.Width*.14f,hh*2);
    PointF tri[3]={PointF(x+r.Width*.14f,cy-r.Height*.30f),PointF(x+r.Width*.40f,cy),PointF(x+r.Width*.14f,cy+r.Height*.30f)};
    g.FillPolygon(&b,tri,3);
    if(mudo){ g.DrawLine(&p,r.X+r.Width*.52f,cy-r.Height*.18f,r.X+r.Width*.86f,cy+r.Height*.18f); g.DrawLine(&p,r.X+r.Width*.86f,cy-r.Height*.18f,r.X+r.Width*.52f,cy+r.Height*.18f); }
    else g.DrawArc(&p,r.X+r.Width*.34f,cy-r.Height*.30f,r.Width*.42f,r.Height*.60f,-55.f,110.f);
}
// Capa de um item (miniatura da internet vem em segundo plano, igual ao online).
static std::vector<std::pair<std::wstring,std::wstring>> g_rxThumbFila;
static void RxCapa(Graphics& g,const RectF& art,const std::wstring& capaUrl,const std::wstring& capaLocal,bool redondo,const Color& plate){
    SolidBrush pb(plate);
    if(redondo) g.FillEllipse(&pb,art.X,art.Y,art.Width,art.Height);
    else DrawRoundRect(g,art,(int)S(UI_R_CARD),&pb,nullptr);
    std::wstring arq=capaLocal;
    if(arq.empty()&&!capaUrl.empty()){
        arq=OnlineThumbFile(capaUrl);
        if(!g_thumbReady.count(arq)){
            std::error_code ec;
            if(std::filesystem::exists(std::filesystem::path(arq),ec)) g_thumbReady.insert(arq);
            else { g_rxThumbFila.push_back({capaUrl,capaUrl}); arq.clear(); }
        }
    }
    if(arq.empty()) return;
    Image* im=GetThumb(arq); if(!im||im->GetLastStatus()!=Ok) return;
    if(redondo){
        GraphicsPath clip; clip.AddEllipse(art.X,art.Y,art.Width,art.Height);
        Region old; g.GetClip(&old); g.SetClip(&clip);
        DrawImgCover(g,im,art);
        g.SetClip(&old);
    } else DrawImgCover(g,im,art);
}
static void RxBaixarCapasPendentes(){
    if(g_rxThumbFila.empty()) return;
    static std::set<std::wstring> pedidas;
    std::vector<std::pair<std::wstring,std::wstring>> v;
    for(auto& p:g_rxThumbFila){ if(pedidas.count(p.first)) continue; pedidas.insert(p.first); v.push_back({p.first,p.second}); }
    g_rxThumbFila.clear();
    if(!v.empty()) FetchThumbsAsync(v);
}

// ------------------------------------------------------------- lateral -----
static void RxDrawSide(Graphics& g,const Color& ab,const Color& white,const Color& gray){
    if(R_rxSide.right<=R_rxSide.left) return;
    const wchar_t* nomes[3]={L"Início",L"Descobrir",L"Sua biblioteca"};
    for(int i=0;i<3;i++){
        RECT r=R_rxNav[i]; if(r.right<=r.left) continue;
        bool sel=(i==0&&g_rxPag==RXP_INICIO)||(i==1&&g_rxPag==RXP_DESCOBRIR)||(i==2&&g_rxPag==RXP_LISTA);
        bool hot=UiHot(r);
        RectF b=RF(r);
        if(sel||hot){ SolidBrush bg(ToGdi(sel?UI().surfaceHi:UI().surface)); DrawRoundRect(g,b,(int)S(UI_R_CARD),&bg,nullptr); }
        RectF ic(b.X+S(10),b.Y+S(9),S(20),S(20));
        Color c=sel?white:gray;
        if(i==0) RxIconCasa(g,ic,c); else if(i==1) RxIconLupa(g,ic,c); else RxIconLista(g,ic,c);
        SolidBrush tb(c);
        TextTrim(g,nomes[i],RectF(b.X+S(40),b.Y,b.Width-S(48),b.Height),S(13),&tb,sel,StringTrimmingEllipsisCharacter,true);
    }
    if(R_rxNovaPl.right>R_rxNovaPl.left){
        RECT r=R_rxNovaPl; RectF b=RF(r);
        SolidBrush bg(ToGdi(UiHot(r)?UI().surfaceHi:UI().surface));
        DrawRoundRect(g,b,(int)S(UI_R_PILL),&bg,nullptr);
        SolidBrush tb(UiHot(r)?white:gray);
        TextCenter(g,L"+  NOVA PLAYLIST",b,S(11),&tb,true);
    }
    for(size_t i=0;i<R_rxSidePl.size();i++){
        RECT r=R_rxSidePl[i]; RectF b=RF(r);
        bool sel = i==0 ? (g_rxPag==RXP_LISTA&&g_view==0)
                        : (g_rxPag==RXP_LISTA&&g_view==2&&g_openPl==(int)i-1);
        if(sel||UiHot(r)){ SolidBrush bg(ToGdi(sel?UI().surfaceHi:UI().surface)); DrawRoundRect(g,b,(int)S(UI_R_CARD),&bg,nullptr); }
        float cv=b.Height-S(10);
        RectF art(b.X+S(6),b.Y+S(5),cv,cv);
        Color plate=ToGdi(UI().bg);
        if(i==0){
            SolidBrush pb(plate); DrawRoundRect(g,art,(int)S(4),&pb,nullptr);
            RxIconLista(g,RectF(art.X+S(3),art.Y+S(3),art.Width-S(6),art.Height-S(6)),ab);
        } else RxCapa(g,art,L"",g_playlists[i-1].coverPath,false,plate);
        float tx=art.X+cv+S(9), tw=b.Width-(tx-b.X)-S(8);
        std::wstring nome = i==0?std::wstring(L"Todas as músicas"):g_playlists[i-1].name;
        std::wstring sub = i==0 ? (std::to_wstring(g_libCached?g_libTracks.size():g_tracks.size())+L" músicas")
                                : (L"Playlist  ·  "+std::to_wstring(g_playlists[i-1].entries.size())+L" músicas");
        SolidBrush nb(sel?ab:white), sb(gray);
        TextTrim(g,nome,RectF(tx,b.Y+S(6),tw,S(17)),S(12),&nb,false,StringTrimmingEllipsisCharacter);
        TextTrim(g,sub,RectF(tx,b.Y+S(23),tw,S(14)),S(10),&sb,false,StringTrimmingEllipsisCharacter);
    }
}

// ------------------------------------------------------- iconezinhos -------
static void RxIconLetra(Graphics& g,const RectF& r,const Color& c){
    Pen p(c,1.7f);
    for(int i=0;i<4;i++){ REAL y=r.Y+r.Height*(0.18f+i*0.22f); REAL w=r.Width*((i%2)?0.62f:0.86f); g.DrawLine(&p,r.X+r.Width*0.07f,y,r.X+r.Width*0.07f+w,y); }
}
static void RxIconFila(Graphics& g,const RectF& r,const Color& c){
    Pen p(c,1.7f);
    for(int i=0;i<3;i++){ REAL y=r.Y+r.Height*(0.22f+i*0.26f); g.DrawLine(&p,r.X+r.Width*0.08f,y,r.X+r.Width*0.62f,y); }
    g.DrawLine(&p,r.X+r.Width*0.74f,r.Y+r.Height*0.22f,r.X+r.Width*0.74f,r.Y+r.Height*0.78f);
    g.DrawLine(&p,r.X+r.Width*0.74f,r.Y+r.Height*0.78f,r.X+r.Width*0.92f,r.Y+r.Height*0.50f);
}
static void RxIconSaida(Graphics& g,const RectF& r,const Color& c){
    Pen p(c,1.7f); SolidBrush b(c);
    g.DrawRectangle(&p,r.X+r.Width*0.20f,r.Y+r.Height*0.10f,r.Width*0.60f,r.Height*0.80f);
    g.DrawEllipse(&p,r.X+r.Width*0.34f,r.Y+r.Height*0.44f,r.Width*0.32f,r.Height*0.32f);
    g.FillEllipse(&b,r.X+r.Width*0.46f,r.Y+r.Height*0.20f,r.Width*0.08f,r.Height*0.08f);
}
static void RxBotaoBarra(Graphics& g,const RECT& r,void(*ico)(Graphics&,const RectF&,const Color&),bool on,const Color& ab,const Color& white,const Color& gray){
    if(r.right<=r.left) return;
    RectF b=RF(r);
    bool hot=UiHot(r);
    if(on||hot){ SolidBrush bg(ToGdi(on?UI().surfaceHi:UI().surface)); DrawRoundRect(g,b,(int)S(6),&bg,nullptr); }
    ico(g,RectF(b.X+S(6),b.Y+S(6),b.Width-S(12),b.Height-S(12)),on?ab:(hot?white:gray));
}

// --------------------------------------------------------- barra de baixo ---
static void RxDrawBar(Graphics& g,int w,int h,const Color& ab,const Color& white,const Color& gray,const Color& navB,const Color& playP,const Color& playB){
    (void)h;
    RectF bar=RF(R_rxBar);
    SolidBrush bb(ToGdi(UI().bar)); g.FillRectangle(&bb,bar);
    Pen dv(ToGdi(UI().border),1.f); g.DrawLine(&dv,0.f,bar.Y+.5f,(REAL)w,bar.Y+.5f);
    const Track* ct=(g_current>=0&&g_current<(int)g_tracks.size())?&g_tracks[(size_t)g_current]:(g_nowPlayingValid?&g_nowPlaying:nullptr);
    {
        RectF art=RF(R_art);
        if(g_cfg.artShape==L"cd"){ Pen pn(ToGdi(UI().borderHi),1.2f); DrawCoverCircle(g,g_coverImg,Rect((int)art.X,(int)art.Y,(int)art.Width,(int)art.Height),&pn,g_player.playing?g_rotation:0); }
        else {
            SolidBrush pb(ToGdi(UI().surface)); DrawRoundRect(g,art,(int)S(UI_R_CARD),&pb,nullptr);
            if(g_coverImg&&g_coverImg->GetLastStatus()==Ok) DrawImgCover(g,g_coverImg,art);
        }
        float tx=art.X+art.Width+S(12);
        float tw=std::min((float)S(240),(float)R_shuffle.left-tx-S(20));
        if(tw>S(60)){
            std::wstring title=ct?ct->title:L"Nada tocando";
            std::wstring artist=ct?ct->artist:L"—";
            SolidBrush wb(white), gb(gray);
            RectF tr(tx,art.Y+S(4),tw,S(20));
            if(g_player.playing) DrawMarqueeText(g,title,UiFont(S(13),true),tr,&wb,false,S(26),NowMs());
            else TextTrim(g,title,tr,S(13),&wb,true,StringTrimmingEllipsisCharacter);
            TextTrim(g,artist,RectF(tx,art.Y+S(26),tw,S(16)),S(11),&gb,false,StringTrimmingEllipsisCharacter);
        }
    }
    {   // transporte
        SolidBrush sh(g_cfg.shuffle?ab:gray), rp(g_cfg.repeat?ab:gray);
        g.DrawString(L"⇄",-1,SymFont(S(15)),RF(R_shuffle),nullptr,&sh);
        g.DrawString(L"⟳",-1,SymFont(S(15)),RF(R_repeat),nullptr,&rp);
        SolidBrush nb(navB);
        {RECT z=R_prev; IconSkip(g,RectF(z.left+S(4),z.top+S(7),z.right-z.left-S(8),z.bottom-z.top-S(14)),&nb,false);}
        {RECT z=R_next; IconSkip(g,RectF(z.left+S(4),z.top+S(7),z.right-z.left-S(8),z.bottom-z.top-S(14)),&nb,true);}
        SolidBrush pp(playP); g.FillEllipse(&pp,RF(R_play));
        SolidBrush pbb(playB);
        {RECT z=R_play; RectF pl(z.left+S(11),z.top+S(11),z.right-z.left-S(22),z.bottom-z.top-S(22)); if(g_player.playing) IconPause(g,pl,&pbb); else IconPlay(g,pl,&pbb);}
        if(g_converting){ Pen ap(ab,3.f); RectF pr=RF(R_play); g.DrawArc(&ap,pr.X-S(4),pr.Y-S(4),pr.Width+S(8),pr.Height+S(8),(REAL)((NowMs()%1000)*0.36f),100.f); }
    }
    {   // barra de tempo
        DWORD pos=(g_current>=0&&g_player.loaded)?g_player.GetPositionMs():0;
        DWORD len=(g_current>=0&&g_player.loaded)?g_player.GetLengthMs():0;
        float frac=len?std::min(1.f,(float)pos/len):0;
        RectF sk=RF(R_seek);
        float by=sk.Y+sk.Height/2.f-S(2);
        SolidBrush tb(ToGdi(UI().border)); DrawRoundRect(g,RectF(sk.X,by,sk.Width,S(4)),2,&tb,nullptr);
        Color fillC=UiHot(R_seek)?ab:ToGdi(UI().text);
        SolidBrush fb(fillC);
        if(frac>0) DrawRoundRect(g,RectF(sk.X,by,sk.Width*frac,S(4)),2,&fb,nullptr);
        if(UiHot(R_seek)) g.FillEllipse(&fb,sk.X+sk.Width*frac-S(6),by+S(2)-S(6),S(12),S(12));
        SolidBrush gb(gray);
        StringFormat rfmt; rfmt.SetAlignment(StringAlignmentFar); rfmt.SetFormatFlags(StringFormatFlagsNoWrap);
        g.DrawString(FormatTime(pos).c_str(),-1,UiFont(S(10),true),RectF(sk.X-S(50),sk.Y-S(1),S(44),S(16)),&rfmt,&gb);
        TextAt(g,FormatTime(len),sk.X+sk.Width+S(6),sk.Y-S(1),S(10),&gb,true);
    }
    RxBotaoBarra(g,R_rxLetra,RxIconLetra,g_rxLetraOn,ab,white,gray);
    RxBotaoBarra(g,R_rxPainel,RxIconFila,g_rxPainelOn,ab,white,gray);
    RxBotaoBarra(g,R_rxSaida,RxIconSaida,!Player::OutDeviceName().empty(),ab,white,gray);
    {   // volume
        RectF vt=RF(R_vol);
        SolidBrush vb(ToGdi(UI().border)); DrawRoundRect(g,vt,2,&vb,nullptr);
        float vf=g_muted?0.f:(float)g_cfg.volume/100.f;
        SolidBrush vf2(UiHot(R_vol)?ab:ToGdi(UI().text));
        if(vf>0) DrawRoundRect(g,RectF(vt.X,vt.Y,vt.Width*vf,vt.Height),2,&vf2,nullptr);
        RectF ic=RF(R_volIcon);
        RxIconSom(g,RectF(ic.X+S(2),ic.Y+S(3),ic.Width-S(4),ic.Height-S(6)),UiHot(R_volIcon)?white:gray,g_muted);
    }
}

// ------------------------------------------------------- lista detalhada ----
static void RxEqBarras(Graphics& g,const RectF& r,const Color& c,bool animando){
    SolidBrush b(c);
    REAL bw=r.Width/5.f;
    for(int i=0;i<3;i++){
        REAL f=animando?(REAL)(0.35+0.65*std::fabs(sin((double)NowMs()/(220.0+i*70.0)+i))):0.5f;
        REAL h=r.Height*f;
        g.FillRectangle(&b,r.X+i*bw*1.6f,r.Y+r.Height-h,bw,h);
    }
}
static void RxDrawLista(Graphics& g,const Color& ab,const Color& white,const Color& gray){
    if(R_library.right<=R_library.left) return;
    REAL fT=S(12.5f), fS=S(10.5f);
    SolidBrush wb(white), gb(gray), abb(ab);
    for(size_t vi=0;vi<g_visible.size();++vi){
        size_t i=(size_t)g_visible[vi];
        if(i>=R_cardRects.size()||i>=g_tracks.size()) continue;
        RECT rr=R_cardRects[i]; if(rr.right<=rr.left) continue;
        const Track& t=g_tracks[i];
        bool cur=((int)i==g_current), hot=UiHot(rr);
        RectF row=RF(rr);
        if(cur||hot){ SolidBrush rb(ToGdi(cur?UI().surfaceHi:UI().surface)); DrawRoundRect(g,row,(int)S(UI_R_CARD),&rb,nullptr); }
        if(cur) DrawRoundRect(g,RectF(row.X,row.Y+S(6),S(3),row.Height-S(12)),(int)S(2),&abb,nullptr);
        REAL x=row.X+S(14);
        {
            RectF n(x,row.Y,S(26),row.Height);
            if(hot){ SolidBrush ic(white); IconPlay(g,RectF(n.X+S(6),n.Y+row.Height/2-S(7),S(13),S(14)),&ic); }
            else if(cur) RxEqBarras(g,RectF(n.X+S(6),n.Y+row.Height/2-S(7),S(14),S(14)),ab,g_player.playing);
            else TextCenter(g,std::to_wstring(vi+1),n,fS,&gb,false);
            x=n.X+n.Width+S(8);
        }
        REAL cv=row.Height-S(14);
        RectF art(x,row.Y+S(7),cv,cv);
        if(g_cfg.artShape==L"cd"){ Pen pn(ToGdi(UI().borderHi),1.f); DrawCoverCircle(g,GetThumb(t.coverPath),Rect((int)art.X,(int)art.Y,(int)art.Width,(int)art.Height),&pn,cur&&g_player.playing?g_rotation:0); }
        else { SolidBrush plate(ToGdi(UI().bg)); DrawRoundRect(g,art,(int)S(4),&plate,nullptr); Image* im=GetThumb(t.coverPath); if(im) DrawImgCover(g,im,art); }
        x=art.X+cv+S(12);
        REAL dirW=S(56), fonteW=row.Width>S(560)?S(120):0;
        REAL txW=row.Width-(x-row.X)-dirW-fonteW-S(20);
        if(txW<S(80)) txW=S(80);
        SolidBrush tb(cur?ab:white);
        TextTrim(g,t.title,RectF(x,row.Y+S(9),txW,S(19)),fT,&tb,true,StringTrimmingEllipsisCharacter);
        std::wstring sub=t.artist.empty()?std::wstring(L"Artista desconhecido"):t.artist;
        TextTrim(g,sub,RectF(x,row.Y+S(27),txW,S(16)),fS,&gb,false,StringTrimmingEllipsisCharacter);
        if(fonteW>0){
            bool online=IsOnlineTrack(t);
            std::wstring de;
            if(online){ const StreamInfo* c=FindSnap(t.path); de=c?StreamTagLabel(*c,true):std::wstring(L"Online"); }
            else de=std::filesystem::path(t.path).parent_path().filename().wstring();
            RectF fr(row.X+row.Width-dirW-fonteW-S(10),row.Y,fonteW,row.Height);
            if(online){ SolidBrush pill(ToGdi(UI().surfaceHi)); DrawRoundRect(g,RectF(fr.X+S(6),row.Y+row.Height/2-S(9),fonteW-S(12),S(18)),(int)S(9),&pill,nullptr); }
            SolidBrush fb(online?ab:gray);
            TextCenter(g,de,fr,S(9.5f),&fb,false);
        }
        int dur=DurSegundos(t.path,t.durSec);
        std::wstring tempo=dur>0?FormatTime((DWORD)dur*1000):std::wstring(L"--:--");
        StringFormat rf; rf.SetAlignment(StringAlignmentFar); rf.SetLineAlignment(StringAlignmentCenter); rf.SetFormatFlags(StringFormatFlagsNoWrap);
        g.DrawString(tempo.c_str(),-1,UiFont(fS,false),RectF(row.X+row.Width-dirW-S(10),row.Y,dirW,row.Height),&rf,&gb);
        DrawOrderArrows(g,i,&abb,&gb);
        DrawPickMark(g,i,rr);
    }
}

// ------------------------------------------------------------- letra -------
static void RxDrawLetra(Graphics& g,const Color& ab,const Color& white,const Color& gray){
    (void)ab;
    RectF main=RF(R_rxMain);
    Region old; g.GetClip(&old); g.SetClip(main);
    SolidBrush wb(white), gb(gray), fb(ToGdi(UI().textFaint));
    const Track* ct=(g_current>=0&&g_current<(int)g_tracks.size())?&g_tracks[(size_t)g_current]:(g_nowPlayingValid?&g_nowPlaying:nullptr);
    TextAt(g,ct?ct->title:std::wstring(L"Nada tocando"),main.X,main.Y+S(2),S(20),&wb,true);
    if(ct) TextAt(g,ct->artist,main.X,main.Y+S(28),S(11),&gb);
    if(!ct){ g.SetClip(&old); return; }
    letra::Letra L=letra::Para(ct->path,ct->title,ct->artist,DurSegundos(ct->path,ct->durSec),RxAvisarNovidades);
    if(L.estado==0){ TextTrim(g,L"Procurando a letra...",RectF(main.X,main.Y+S(90),main.Width,S(24)),S(13),&gb,false,StringTrimmingEllipsisCharacter,true); g.SetClip(&old); return; }
    if(L.estado==2){
        TextTrim(g,L"Não achei a letra desta música.",RectF(main.X,main.Y+S(90),main.Width,S(24)),S(14),&wb,true,StringTrimmingEllipsisCharacter,true);
        TextTrim(g,L"O Remix procura no LRCLIB pelo nome, artista e duração. Corrigir o nome ou o artista da faixa costuma resolver.",
                 RectF(main.X,main.Y+S(118),main.Width-S(40),S(22)),S(11),&gb,false,StringTrimmingEllipsisWord,true);
        g.SetClip(&old); return;
    }
    DWORD pos=g_player.loaded?g_player.GetPositionMs():0;
    int atual=letra::LinhaAtual(L,(int)pos);
    Region old2; g.GetClip(&old2); g.SetClip(RectF(main.X,main.Y+S(46),main.Width,main.Height-S(46)));
    if(L.sync){
        for(size_t i=0;i<R_rxLetraLinhas.size()&&i<L.linhas.size();i++){
            RECT r=R_rxLetraLinhas[i]; RectF b=RF(r);
            if(b.Y+b.Height<main.Y||b.Y>main.Y+main.Height) continue;
            bool ehAtual=((int)i==atual);
            SolidBrush c(ehAtual?white:(UiHot(r)?white:ToGdi(UI().textFaint)));
            if(ehAtual){ SolidBrush bg(ToGdi(UI().surface)); DrawRoundRect(g,RectF(b.X-S(6),b.Y-S(2),b.Width+S(12),b.Height+S(4)),(int)S(6),&bg,nullptr); }
            TextTrim(g,L.linhas[i].txt.empty()?std::wstring(L"♪"):L.linhas[i].txt,b,ehAtual?S(17):S(14),&c,ehAtual,StringTrimmingEllipsisWord,true);
        }
    } else {
        REAL y=main.Y+S(52)-(REAL)g_rxLetraScroll;
        std::wstring t=L.texto; size_t i=0;
        while(i<=t.size()&&y<main.Y+main.Height){
            size_t e=t.find(L'\n',i); if(e==std::wstring::npos) e=t.size();
            if(y>main.Y-S(20)) TextTrim(g,t.substr(i,e-i),RectF(main.X,y,main.Width-S(20),S(22)),S(13),&gb,false,StringTrimmingEllipsisWord,true);
            y+=S(24);
            if(e==t.size()) break;
            i=e+1;
        }
    }
    g.SetClip(&old2);
    if(!L.fonte.empty()){
        StringFormat rf; rf.SetAlignment(StringAlignmentFar); rf.SetLineAlignment(StringAlignmentCenter);
        g.DrawString((L.fonte+(L.sync?L"  ·  toque numa linha para pular":L"  ·  sem marcação de tempo")).c_str(),-1,UiFont(S(9.5f),false),
                     RectF(main.X,main.Y+main.Height-S(22),main.Width-S(16),S(18)),&rf,&fb);
    }
    g.SetClip(&old);
}

// --------------------------------------------------- painel da direita -----
static void RxDrawPainel(Graphics& g,const Color& ab,const Color& white,const Color& gray){
    (void)ab;
    if(R_rxNp.right<=R_rxNp.left) return;
    RectF p=RF(R_rxNp);
    SolidBrush bg(ToGdi(UI().bg,215));
    DrawRoundRect(g,p,(int)S(10),&bg,nullptr);
    Region old; g.GetClip(&old); g.SetClip(p);
    SolidBrush wb(white), gb(gray), fb(ToGdi(UI().textFaint));
    const Track* ct=(g_current>=0&&g_current<(int)g_tracks.size())?&g_tracks[(size_t)g_current]:(g_nowPlayingValid?&g_nowPlaying:nullptr);
    REAL x=p.X+S(14), w=p.Width-S(28), y=p.Y+S(14);
    TextAt(g,L"TOCANDO AGORA",x,y,S(9.5f),&fb,true); y+=S(20);
    if(!ct){ TextTrim(g,L"Nada tocando.",RectF(x,y+S(10),w,S(20)),S(12),&gb,false,StringTrimmingEllipsisCharacter,true); g.SetClip(&old); return; }
    RectF art(x,y,w,w);
    if(g_cfg.artShape==L"cd"){ Pen pn(ToGdi(UI().borderHi),1.2f); DrawCoverCircle(g,g_coverImg,Rect((int)art.X,(int)art.Y,(int)art.Width,(int)art.Height),&pn,g_player.playing?g_rotation:0); }
    else { SolidBrush plate(ToGdi(UI().surface)); DrawRoundRect(g,art,(int)S(UI_R_CARD),&plate,nullptr); if(g_coverImg&&g_coverImg->GetLastStatus()==Ok) DrawImgCover(g,g_coverImg,art); }
    y=art.Y+art.Height+S(12);
    TextTrim(g,ct->title,RectF(x,y,w,S(24)),S(16),&wb,true,StringTrimmingEllipsisCharacter); y+=S(24);
    TextTrim(g,ct->artist.empty()?std::wstring(L"Artista desconhecido"):ct->artist,RectF(x,y,w,S(18)),S(11),&gb,false,StringTrimmingEllipsisCharacter); y+=S(24);
    {
        int dur=DurSegundos(ct->path,ct->durSec);
        std::wstring de=IsOnlineTrack(*ct)?std::wstring(L"Online"):std::filesystem::path(ct->path).parent_path().filename().wstring();
        TextTrim(g,de+(dur>0?(L"  ·  "+FormatTime((DWORD)dur*1000)):std::wstring()),RectF(x,y,w,S(16)),S(10),&fb,false,StringTrimmingEllipsisCharacter);
        y+=S(24);
    }
    TextAt(g,L"A SEGUIR",x,y,S(9.5f),&fb,true); y+=S(18);
    int mostrados=0;
    for(int k=1;k<=4&&mostrados<4;k++){
        int idx=-1;
        if(g_cfg.shuffle&&!g_shufQueue.empty()&&g_shufPos>=0){ int q=g_shufPos+k; if(q<(int)g_shufQueue.size()) idx=g_shufQueue[(size_t)q]; }
        else if(g_current>=0&&g_current+k<(int)g_tracks.size()) idx=g_current+k;
        if(idx<0||idx>=(int)g_tracks.size()) break;
        const Track& t=g_tracks[(size_t)idx];
        REAL rh=S(38);
        if(y+rh>p.Y+p.Height-S(8)) break;
        RectF cvr(x,y+S(3),rh-S(8),rh-S(8));
        SolidBrush pl2(ToGdi(UI().surface)); DrawRoundRect(g,cvr,(int)S(4),&pl2,nullptr);
        Image* im=GetThumb(t.coverPath); if(im) DrawImgCover(g,im,cvr);
        TextTrim(g,t.title,RectF(cvr.X+cvr.Width+S(8),y+S(2),w-cvr.Width-S(12),S(17)),S(11),&wb,false,StringTrimmingEllipsisCharacter);
        TextTrim(g,t.artist,RectF(cvr.X+cvr.Width+S(8),y+S(18),w-cvr.Width-S(12),S(15)),S(9.5f),&gb,false,StringTrimmingEllipsisCharacter);
        y+=rh; mostrados++;
    }
    g.SetClip(&old);
}

// ------------------------------------------------- cabecalho da biblioteca --
static std::wstring RxNomeDaPagina(){
    if(g_view==2){ const Playlist* p=OpenPlaylistPtr(); if(p) return p->name; return L"Playlist"; }
    if(g_view==1) return L"Playlists";
    return L"Todas as músicas";
}
static void RxDrawCabecalho(Graphics& g,const Color& ab,const Color& white,const Color& gray){
    if(R_rxCab.right<=R_rxCab.left) return;
    RectF b=RF(R_rxCab);
    SolidBrush wb(white), gb(gray);
    TextTrim(g,RxNomeDaPagina(),RectF(b.X,b.Y,b.Width-S(250),S(32)),S(24),&wb,true,StringTrimmingEllipsisCharacter,true);
    size_t n=g_visible.size();
    std::wstring sub=std::to_wstring(n)+(n==1?L" música":L" músicas");
    if(!g_searchBuf.empty()) sub+=L"  ·  filtrando por \""+g_searchBuf+L"\"";
    TextTrim(g,sub,RectF(b.X,b.Y+S(38),b.Width-S(250),S(18)),S(11),&gb,false,StringTrimmingEllipsisCharacter,true);
    if(R_rxTocar.right>R_rxTocar.left){
        RectF t=RF(R_rxTocar);
        SolidBrush tb(ab); DrawRoundRect(g,t,(int)(t.Height/2.f),&tb,nullptr);
        SolidBrush tt(ToGdi(UI().bg)); TextCenter(g,L"TOCAR",t,S(11),&tt,true);
        RECT ra=R_rxAleat; RectF a=RF(ra);
        SolidBrush abg(ToGdi(UiHot(ra)?UI().surfaceHi:UI().surface));
        DrawRoundRect(g,a,(int)(a.Height/2.f),&abg,nullptr);
        SolidBrush at(g_cfg.shuffle?ab:white); TextCenter(g,L"ALEATÓRIO",a,S(11),&at,true);
    }
}

// ---------------------------------------------------------- tela inicial ---
static std::wstring RxSaudacao(){
    time_t tt=time(nullptr); struct tm lt{};
    localtime_s(&lt,&tt);
    if(lt.tm_hour<6) return L"Boa madrugada";
    if(lt.tm_hour<12) return L"Bom dia";
    if(lt.tm_hour<18) return L"Boa tarde";
    return L"Boa noite";
}
static const wchar_t* RxNomeTipo(int kind){
    return kind==desc::K_ALBUM?L"Álbum":kind==desc::K_PLAYLIST?L"Playlist":kind==desc::K_ARTISTA?L"Artista":L"Música";
}
static void RxDrawMainBg(Graphics& g){
    if(R_rxMain.right<=R_rxMain.left) return;
    RectF m=RF(R_rxMain);
    SolidBrush bg(ToGdi(UI().bg,215));
    DrawRoundRect(g,RectF(m.X,m.Y-S(8),m.Width,m.Height+S(12)),(int)S(10),&bg,nullptr);
}
static void RxDrawBusca(Graphics& g,const Color& white,const Color& gray){
    if(R_searchBox.right<=R_searchBox.left) return;
    RectF b=RF(R_searchBox);
    SolidBrush bg(ToGdi(UI().surface)); Pen pn(ToGdi(g_searchFocus?UI().borderHi:UI().border),1.2f);
    DrawRoundRect(g,b,(int)(b.Height/2.f),&bg,&pn);
    RxIconLupa(g,RectF(b.X+S(10),b.Y+S(9),S(18),S(18)),g_searchFocus?white:gray);
    std::wstring txt=g_searchBuf.empty()?std::wstring(L"Buscar na sua biblioteca"):g_searchBuf;
    SolidBrush tb(g_searchBuf.empty()?gray:white);
    TextTrim(g,txt,RectF(b.X+S(36),b.Y,b.Width-S(70),b.Height),S(12),&tb,false,StringTrimmingEllipsisCharacter,true);
    if(g_searchFocus&&(NowMs()/500)%2==0){
        float tw=MeasureW(g,g_searchBuf,S(12),false);
        Pen cp(white,1.4f); g.DrawLine(&cp,b.X+S(37)+tw,b.Y+S(10),b.X+S(37)+tw,b.Y+b.Height-S(10));
    }
    if(!g_searchBuf.empty()){ SolidBrush xb(gray); TextCenter(g,L"✕",RF(R_searchClear),S(12),&xb,false); }
}
static void RxDrawInicio(Graphics& g,int w,int h,const Color& ab,const Color& white,const Color& gray){
    (void)w; (void)h;
    if(g_rxPag==RXP_INICIO) desc::Atualizar(false,RxAvisarNovidades);
    RectF main=RF(R_rxMain);
    Region old; g.GetClip(&old); g.SetClip(main);
    SolidBrush wb(white), gb(gray);
    std::wstring tituloPag = g_rxPag==RXP_INICIO?RxSaudacao():(g_rxGenero.empty()?std::wstring(L"Descobrir"):g_rxGeneroNome);
    TextAt(g,tituloPag,main.X,main.Y-(REAL)g_rxScroll+S(2),S(22),&wb,true);
    if(R_rxBuscarOn.right>R_rxBuscarOn.left){
        RECT rb=R_rxBuscarOn; RectF b=RF(rb);
        SolidBrush bg(ToGdi(UiHot(rb)?UI().surfaceHi:UI().surface));
        DrawRoundRect(g,b,(int)(b.Height/2.f),&bg,nullptr);
        TextCenter(g,L"BUSCAR ONLINE",b,S(10),&wb,true);
    }
    if(R_rxVoltar.right>R_rxVoltar.left){
        RECT rv=R_rxVoltar; RectF b=RF(rv);
        SolidBrush bg(ToGdi(UiHot(rv)?UI().surfaceHi:UI().surface));
        DrawRoundRect(g,b,(int)(b.Height/2.f),&bg,nullptr);
        TextCenter(g,L"← GÊNEROS",b,S(10),&wb,true);
    }
    desc::Home home; std::vector<desc::Item> generos;
    if(g_rxPag==RXP_INICIO) home=desc::Copia();
    else {
        desc::AtualizarGeneros(g_rxGenero,g_rxGeneroNome,RxAvisarNovidades);
        if(!g_rxGenero.empty()) desc::HomeDoGenero(g_rxGenero,home);
        else generos=desc::ListaGeneros();
    }
    for(const RxAtalho& k:g_rxAtalhos){   // atalhos: capa pequena + nome, em duas colunas
        if(k.r.right<=k.r.left) continue;
        RectF b=RF(k.r);
        if(b.Y>main.Y+main.Height||b.Y+b.Height<main.Y) continue;
        bool hot=UiHot(k.r);
        SolidBrush bg(ToGdi(hot?UI().surfaceHi:UI().surface));
        DrawRoundRect(g,b,(int)S(UI_R_CARD),&bg,nullptr);
        REAL cv=b.Height;
        RectF art(b.X,b.Y,cv,cv);
        RxCapa(g,art,L"",k.capa,false,ToGdi(UI().bg));
        REAL tx=art.X+cv+S(12), tw=b.Width-(tx-b.X)-S(52);
        TextTrim(g,k.nome,RectF(tx,b.Y+S(10),tw,S(19)),S(12),&wb,true,StringTrimmingEllipsisCharacter);
        TextTrim(g,k.sub,RectF(tx,b.Y+S(29),tw,S(16)),S(10),&gb,false,StringTrimmingEllipsisCharacter);
        if(hot){
            REAL d=S(30), px=b.X+b.Width-d-S(10), py=b.Y+(b.Height-d)/2;
            SolidBrush pb(ab); g.FillEllipse(&pb,px,py,d,d);
            SolidBrush ic(ToGdi(UI().bg));
            IconPlay(g,RectF(px+d*.34f,py+d*.28f,d*.40f,d*.44f),&ic);
        }
    }
    for(size_t f=0;f<g_rxFilas.size();f++){
        const RxFila& fl=g_rxFilas[f];
        std::wstring titulo,nota;
        if(fl.fonte==-1){ titulo=L"Tocados recentemente"; nota=L"volte de onde parou"; }
        else if(fl.fonte==-2){ titulo=L"Explorar por gênero"; nota=L"as paradas de cada estilo"; }
        else if(fl.fonte==-3){ titulo=L"Sua mistura"; nota=L"do seu jeito, muda todo dia"; }
        else if((size_t)fl.fonte<home.fileiras.size()){ titulo=home.fileiras[(size_t)fl.fonte].titulo; nota=home.fileiras[(size_t)fl.fonte].nota; }
        RectF hr=RF(fl.head);
        if(hr.Y<main.Y+main.Height&&hr.Y+S(40)>main.Y){
            TextAt(g,titulo,hr.X,hr.Y,S(15),&wb,true);
            if(!nota.empty()) TextAt(g,nota,hr.X+MeasureW(g,titulo,S(15),true)+S(10),hr.Y+S(4),S(10),&gb);
            if(fl.verTudo.right>fl.verTudo.left){
                SolidBrush vb(UiHot(fl.verTudo)?white:gray);
                StringFormat rfmt; rfmt.SetAlignment(StringAlignmentFar); rfmt.SetLineAlignment(StringAlignmentCenter);
                g.DrawString(RxFileiraAberta(fl.fonte)?L"VER MENOS":L"VER TUDO",-1,UiFont(S(10),true),RF(fl.verTudo),&rfmt,&vb);
            }
        }
    }
    for(const RxCard& k:g_rxCards){
        RectF b=RF(k.r);
        if(b.Y>main.Y+main.Height||b.Y+b.Height<main.Y) continue;
        bool hot=UiHot(k.r);
        if(hot){ SolidBrush bg(ToGdi(UI().surface)); DrawRoundRect(g,RectF(b.X-S(6),b.Y-S(6),b.Width+S(12),b.Height+S(12)),(int)S(UI_R_CARD),&bg,nullptr); }
        float cv=b.Width;
        RectF art(b.X,b.Y,cv,cv);
        std::wstring nome,sub,capaUrl,capaLocal; bool redondo=false;
        if((size_t)k.fila>=g_rxFilas.size()) continue;
        const RxFila& fl=g_rxFilas[(size_t)k.fila];
        if(fl.fonte==-2){
            if((size_t)k.item>=generos.size()) continue;
            const desc::Item& gI=generos[(size_t)k.item];
            RxCapa(g,art,gI.capa,L"",false,ToGdi(UI().surfaceHi));
            SolidBrush veu(Color(120,0,0,0)); DrawRoundRect(g,art,(int)S(UI_R_CARD),&veu,nullptr);
            TextTrim(g,gI.titulo,RectF(art.X+S(8),art.Y+art.Height-S(34),art.Width-S(16),S(26)),S(13),&wb,true,StringTrimmingEllipsisCharacter,true);
            continue;
        }
        if(fl.fonte==-1||fl.fonte==-3){
            const std::vector<int>& lista=(fl.fonte==-3)?g_rxMistura:g_rxRecentes;
            if((size_t)k.item>=lista.size()) continue;
            const std::vector<Track>& fonteT=(g_libCached&&!g_libTracks.empty())?g_libTracks:g_tracks;
            int ti=lista[(size_t)k.item]; if(ti<0||ti>=(int)fonteT.size()) continue;
            const Track* t=&fonteT[(size_t)ti];
            nome=t->title; sub=t->artist.empty()?std::wstring(L"Música"):t->artist; capaLocal=t->coverPath;
            redondo=(g_cfg.artShape==L"cd");
        } else {
            if((size_t)fl.fonte>=home.fileiras.size()) continue;
            const desc::Shelf& s=home.fileiras[(size_t)fl.fonte];
            if((size_t)k.item>=s.itens.size()) continue;
            const desc::Item& it=s.itens[(size_t)k.item];
            nome=it.titulo; sub=it.sub.empty()?RxNomeTipo(it.kind):it.sub; capaUrl=it.capa;
            redondo=(it.kind==desc::K_ARTISTA);
        }
        RxCapa(g,art,capaUrl,capaLocal,redondo,ToGdi(UI().surfaceHi));
        if(hot){
            RectF p=RF(k.play);
            SolidBrush pb(ab); g.FillEllipse(&pb,p);
            SolidBrush ic(ToGdi(UI().bg));
            IconPlay(g,RectF(p.X+p.Width*.32f,p.Y+p.Height*.26f,p.Width*.42f,p.Height*.48f),&ic);
        }
        TextTrim(g,nome,RectF(b.X,b.Y+cv+S(8),b.Width,S(18)),S(12),&wb,true,StringTrimmingEllipsisCharacter);
        TextTrim(g,sub,RectF(b.X,b.Y+cv+S(26),b.Width,S(16)),S(10),&gb,false,StringTrimmingEllipsisCharacter);
    }
    if(g_rxFilas.empty()){
        bool bus=(g_rxPag==RXP_INICIO?desc::Carregando():desc::CarregandoGeneros());
        std::wstring msg=bus?L"Buscando...":L"Sem novidades agora. Toque alguma música e volte aqui.";
        TextTrim(g,msg,RectF(main.X,main.Y+S(90),main.Width,S(30)),S(13),&gb,false,StringTrimmingEllipsisCharacter,true);
    }
    {   // barrinha de rolagem discreta (so quando tem mais coisa para baixo)
        REAL vis=main.Height;
        if((REAL)g_rxContentH>vis+4){
            REAL fr=vis/(REAL)g_rxContentH, alt=std::max((REAL)S(30),main.Height*fr);
            REAL pos=main.Y+(main.Height-alt)*((REAL)g_rxScroll/(REAL)std::max(1,g_rxContentH-(int)vis));
            SolidBrush sb(Color(90,255,255,255));
            DrawRoundRect(g,RectF(main.X+main.Width-S(5),pos,S(3),alt),(int)S(2),&sb,nullptr);
        }
    }
    g.SetClip(&old);
    RxBaixarCapasPendentes();
}
