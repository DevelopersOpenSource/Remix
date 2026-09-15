#pragma once
// Geometria compartilhada (Windows e Linux): BuildLayout / LayoutSettings.
// Usa g_winW/g_winH (area cliente) e so produz RECTs; quem desenha e cada
// plataforma. Os menus flutuantes (pasta, contexto, imagem, confirmacao) sao
// posicionados ao abrir, em app_core.h.
#include "app_core.h"

static void LayoutSettings(int w,int h){
    g_setSliders.clear(); g_setLabels.clear(); g_setSections.clear();
    R_themeCirclesSettings.clear(); R_runColors.clear(); R_playColors.clear(); R_navColors.clear();
    R_partColors.clear(); R_ledColors.clear();
    R_shortcutsBox={0,0,0,0};
    // TELA INTEIRA e escala FIXA: as configuracoes NAO usam S()/SI() (uiScale afeta so o player).
    int m=10;
    int pw=w-m*2, ph=h-m*2, px=m, py=m;
    R_settingsPanel={px,py,px+pw,py+ph};
    R_settingsClose={px+pw-58,py+12,px+pw-20,py+48};
    int top=py+66;
    int L=px+30, R=px+pw-30;
    bool wide=(R-L)>=880;
    auto slider=[&](int id,int x,int y,int wid,int minv,int maxv){
        g_setSliders.push_back({{x,y,x+wid,y+8},{x-10,y-16,x+wid+52,y+24},id,minv,maxv});
    };
    auto label=[&](int x,int y,const wchar_t*s){ g_setLabels.push_back({{x,y,x+400,y+18},s}); };
    auto colorRow=[&](std::vector<RECT>&v,int x,int y,int maxW)->int{
        v.push_back({x,y,x+22,y+22});
        size_t nt=g_themes.size();
        int total=1+(int)nt+15, gp=30, d=22;
        int cap=std::max(4,maxW/gp);
        int rows=(total+cap-1)/cap, idx=1;
        for(size_t i=0;i<nt;i++){int r=idx/cap,c=idx%cap;int xx=x+c*gp,yy=y+r*36;v.push_back({xx,yy,xx+d,yy+d});idx++;}
        for(int i=0;i<15;i++){int r=idx/cap,c=idx%cap;int xx=x+c*gp,yy=y+r*36;v.push_back({xx,yy,xx+d,yy+d});idx++;}
        return rows;
    };
    auto sect=[&](int x,int y,int cw,int hh,const wchar_t*t){ g_setSections.push_back({{x,y,x+cw,y+hh},t}); };
    // secoes comuns as duas larguras
    auto sectLibrary=[&](int x,int& y,int cw){
        sect(x,y,cw,132,L"BIBLIOTECA");
        int bw=(cw-60)/2;
        R_settingsDefault={x+20,y+60,x+20+bw,y+96}; R_settingsCustom={x+30+bw,y+60,x+cw-20,y+96};
        y+=152;
    };
    auto sectMode=[&](int x,int& y,int cw){
        sect(x,y,cw,186,L"MODO DE EXIBIÇÃO");
        int mw3=(cw-60)/3;
        R_settingsModeSquare={x+20,y+58,x+20+mw3,y+94};
        R_settingsModeCd={x+30+mw3,y+58,x+30+mw3*2,y+94};
        R_settingsModeVertical={x+40+mw3*2,y+58,x+cw-20,y+94};
        slider(Z_CD_SPEED,x+20,y+122,cw-110,0,200);
        y+=208;
    };
    auto sectThemes=[&](int x,int& y,int cw){
        sect(x,y,cw,120,L"TEMAS");
        size_t nt=g_themes.size(); int cap=std::max(3,(cw-48)/60);
        for(size_t i=0;i<nt;i++){int r=(int)(i/cap),c=(int)(i%cap);int xx=x+24+c*60,yy=y+54+r*62;R_themeCirclesSettings.push_back({xx,yy,xx+46,yy+46});}
        int tr_=((int)nt+cap-1)/cap; y+=66+tr_*62;
    };
    auto sectEffects=[&](int x,int& y,int cw){
        sect(x,y,cw,96,L"EFEITOS");
        R_setParticles={x+20,y+52,x+185,y+86}; R_setGlitch={x+195,y+52,x+320,y+86};
        R_setPerf={x+330,y+52,x+cw-20,y+86};
        y+=116;
    };
    auto sectColors=[&](int x,int& y,int cw){
        sect(x,y,cw,380,L"CORES E PARTÍCULAS");
        R_setAutoColor={x+20,y+50,x+320,y+86};
        slider(Z_PART_SPEED,x+20,y+124,cw-110,10,300);
        int c1=y+172;
        label(x+20,c1,L"Cor das partículas");
        int rp=colorRow(R_partColors,x+20,c1+26,cw-40);
        int c2=c1+26+rp*36+18;
        label(x+20,c2,L"Cor dos LEDs");
        int rl=colorRow(R_ledColors,x+20,c2+26,cw-40);
        int usedH=(c2+26+rl*36+18)-y;
        g_setSections.back().first.bottom=g_setSections.back().first.top+usedH;
        y+=usedH+20;
    };
    auto sectBackground=[&](int x,int& y,int cw){
        sect(x,y,cw,156,L"FUNDO");
        R_setWallChoose={x+20,y+52,x+270,y+88};
        R_setWallClear={x+280,y+52,x+430,y+88};
        R_setCoverBlur={x+20,y+102,x+cw-20,y+136};
        y+=176;
    };
    auto sectPlayback=[&](int x,int& y,int cw){
        sect(x,y,cw,226,L"REPRODUÇÃO");
        int bw=(cw-60)/3;
        R_setAutoplay={x+20,y+50,x+20+bw,y+84};
        R_setSort={x+30+bw,y+50,x+30+bw*2,y+84};
        R_setSortDir={x+40+bw*2,y+50,x+cw-20,y+84};
        // 2a linha (abaixo das duas linhas de ajuda): ao fechar / controles do sistema
        int hw=(cw-50)/2;
        R_setBgClose={x+20,y+134,x+20+hw,y+168};
        R_setSysMedia={x+30+hw,y+134,x+cw-20,y+168};
        // 3a linha: sair de vez (mesmo tocando em 2o plano)
        R_setQuit={x+20,y+178,x+20+hw,y+212};
        y+=246;
    };
    auto sectScales=[&](int x,int& y,int cw){
        sect(x,y,cw,330,L"TAMANHOS E ESCALAS");
        const int ids[5]={Z_UI_SCALE,Z_TITLE_SCALE,Z_ARTIST_SCALE,Z_VERTICAL_SCALE,Z_PLAYER_SIZE_SLIDER};
        const int mn[5]={70,80,80,70,60}, mx[5]={150,180,200,120,170};
        for(int k=0;k<5;k++) slider(ids[k],x+20,y+68+k*56,cw-110,mn[k],mx[k]);
        y+=352;
    };
    auto sectLed=[&](int x,int& y,int cw){
        sect(x,y,cw,204,L"CONTROLE DO LED");
        slider(Z_LED_BRIGHT,x+20,y+62,cw-110,0,100);
        slider(Z_LED_SPEED,x+20,y+118,cw-110,0,100);
        R_setEffect={x+20,y+154,x+260,y+186};
        y+=226;
    };
    auto sectButtons=[&](int x,int& y,int cw){
        sect(x,y,cw,260,L"BOTÕES");
        label(x+20,y+48,L"Botão play");
        int bp=colorRow(R_playColors,x+20,y+72,cw-40);
        int ny=y+72+bp*36+20;
        label(x+20,ny,L"Anterior / Próximo");
        int bn=colorRow(R_navColors,x+20,ny+26,cw-40);
        int usedH=(ny+26+bn*36+18)-y;
        g_setSections.back().first.bottom=g_setSections.back().first.top+usedH;
        y+=usedH+20;
    };
    auto sectRunner=[&](int x,int& y,int cw){
        sect(x,y,cw,220,L"LED CORREDOR (LINHA)");
        R_setRunnerToggle={x+20,y+54,x+150,y+86};
        slider(Z_RUNNER_SPEED,x+170,y+62,cw-250,0,200);
        label(x+20,y+108,L"Cor da linha");
        int rr=colorRow(R_runColors,x+20,y+132,cw-40);
        g_setSections.back().first.bottom=g_setSections.back().first.top+(132+rr*36+18);
        y+=132+rr*36+18+20;
    };
    auto sectEq=[&](int x,int& y,int cw){
        sect(x,y,cw,478,L"EQUALIZADOR");
        R_setEqOn={x+20,y+50,x+260,y+84};
        R_setEqReset={x+270,y+50,x+370,y+84};
        for(int k=0;k<8;k++) slider(Z_EQ_BASE+k,x+20,y+122+k*44,cw-110,-12,12);
        y+=498;
    };
    auto sectOnline=[&](int x,int& y,int cw){
        // linha de status das ferramentas (y+50), 2 linhas de botoes, caminho dos downloads, "procurar de novo"
        sect(x,y,cw,236,L"ONLINE");
        int bw=(cw-50)/2;
        R_setOnMode={x+20,y+74,x+20+bw,y+108}; R_setOnFmt={x+30+bw,y+74,x+cw-20,y+108};
        R_setOnSrc={x+20,y+116,x+20+bw,y+150}; R_setOnFolder={x+30+bw,y+116,x+cw-20,y+150};
        R_setOnRecheck={x+20,y+186,x+20+std::min(230,bw),y+218};
        y+=256;
    };
    auto sectShortcuts=[&](int x,int& y,int cw){
        // uma linha por acao: rotulo | tecla (clique = capturar) | FOCO/GLOBAL; embaixo, restaurar + dicas
        int rows=HK_COUNT; int hh=54+rows*34+100;
        sect(x,y,cw,hh,L"ATALHOS");
        R_shortcutsBox={x,y,x+cw,y+hh};
        int ky=y+52; int keyW=std::min(190,(cw-40)/3), scW=84;
        for(int a=0;a<HK_COUNT;a++){ int ry=ky+a*34; R_hkScope[a]={x+cw-20-scW,ry,x+cw-20,ry+28}; R_hkKey[a]={R_hkScope[a].left-10-keyW,ry,R_hkScope[a].left-10,ry+28}; }
        int by=ky+rows*34+12; R_hkReset={x+20,by,x+220,by+34};
        y+=hh+20;
    };
    if(wide){
        int colW=(R-L-40)/2, xr=L+colW+40;
        int y=top;
        sectLibrary(L,y,colW); sectMode(L,y,colW); sectThemes(L,y,colW); sectEffects(L,y,colW);
        sectColors(L,y,colW); sectBackground(L,y,colW); sectPlayback(L,y,colW); sectShortcuts(L,y,colW);
        int ry=top;
        sectOnline(xr,ry,colW); sectScales(xr,ry,colW); sectLed(xr,ry,colW); sectButtons(xr,ry,colW); sectRunner(xr,ry,colW); sectEq(xr,ry,colW);
        g_setContentH=(y>ry?y:ry)-py;
    } else {
        int cw=R-L;
        int cy=top;
        sectLibrary(L,cy,cw); sectOnline(L,cy,cw); sectMode(L,cy,cw); sectThemes(L,cy,cw); sectEffects(L,cy,cw);
        sectColors(L,cy,cw); sectScales(L,cy,cw); sectLed(L,cy,cw); sectButtons(L,cy,cw); sectRunner(L,cy,cw);
        sectBackground(L,cy,cw); sectPlayback(L,cy,cw); sectEq(L,cy,cw); sectShortcuts(L,cy,cw);
        g_setContentH=cy-py;
    }
    if(g_setScroll>g_setContentH-(ph-76)) g_setScroll=std::max(0,g_setContentH-(ph-76));
    if(g_setScroll<0) g_setScroll=0;
}

static void BuildLayout(){
    int w=g_winW,h=g_winH;
    R_titlebar={0,0,w,44}; R_close={0,0,0,0}; R_min={0,0,0,0};
    R_wavePanel={0,0,0,0}; R_shapeTgl={0,0,0,0}; R_listBtn={0,0,0,0}; R_autoTgl={0,0,0,0}; R_sortBtn={0,0,0,0}; R_folderBtn={0,0,0,0}; R_volIcon={0,0,0,0};
    R_cardRects.clear(); R_cardCoverButtons.clear(); R_cardSeekRects.clear(); R_themeCircles.clear();
    R_rowUp.clear(); R_rowDown.clear();
    R_libBar=R_tabTracks=R_tabPlaylists=R_searchBox=R_searchClear=R_plBack=R_plNew={0,0,0,0};
    R_tabOnline=R_plAdd=R_plMode=R_pickDone=R_pickCancel=R_onlineInfo={0,0,0,0};
    R_plCards.clear(); R_plPlay.clear(); R_plShuf.clear();
    g_visible.clear(); for(size_t i=0;i<g_tracks.size();++i) if(TrackMatchesSearch(g_tracks[i])) g_visible.push_back((int)i);
    g_gridCols=0; g_contentH=0;
    bool manual=(g_cfg.sortMode==L"manual");
    int chrome=g_customChrome?SI(72):0;   // espaco dos botoes fechar/minimizar (Windows)
    if(g_customChrome){ R_close={w-SI(40),SI(10),w-SI(10),SI(40)}; R_min={w-SI(74),SI(10),w-SI(44),SI(40)}; }
    if(g_cfg.displayMode==L"vertical"){
        int cw=std::min(430,std::max(320,w-24)); int left=(w-cw)/2; int top=56;
        int art=std::min(205,std::max(150,(int)(h*.30f*g_cfg.verticalScale/100.0f)));
        R_art={left+(cw-art)/2,top,left+(cw+art)/2,top+art};
        R_verticalCoverButton={R_art.right-34,R_art.top+8,R_art.right-8,R_art.top+34};
        R_seek={left+26,R_art.bottom+88,left+cw-26,R_art.bottom+112};
        int B=R_art.bottom+112;
        R_prev={left+84,B+36,left+124,B+76}; R_play={left+cw/2-32,B+30,left+cw/2+32,B+94};
        R_next={left+cw-124,B+36,left+cw-84,B+76}; R_shuffle={left+34,B+44,left+66,B+70}; R_repeat={left+cw-66,B+44,left+cw-34,B+70};
        R_volIcon={left+34,B+101,left+58,B+125};
        R_vol={left+70,B+110,left+cw-70,B+116};
        R_heart={left+12,(LONG)S(58),left+44,(LONG)S(90)};
        R_gear={w-56-chrome,(LONG)S(56),w-16-chrome,(LONG)S(94)};
        if(g_customChrome){ R_gear={w-56-chrome+SI(10),SI(10),w-16-chrome+SI(10),SI(46)}; }
        // Cabecalho do vertical: [AUTO] [QUAD|CD] ... [engrenagem/fechar]. Nada se
        // sobrepoe: o toggle e centralizado quando cabe, senao encosta no AUTO e
        // encolhe; sem espaco nenhum, o AUTO some (fica nas configuracoes).
        {
            int rightLimit=g_customChrome?(int)R_gear.left-SI(6):w-SI(10);
            int autoW=(int)S(62), tglW=(int)S(190);
            R_autoTgl={SI(10),(LONG)S(8),SI(10)+autoW,(LONG)S(46)};
            int tx=(w-tglW)/2, minTx=(int)R_autoTgl.right+SI(8);
            if(tx<minTx) tx=minTx;
            if(tx+tglW>rightLimit) tglW=rightLimit-tx;
            if(tglW<(int)S(120)){
                R_autoTgl={0,0,0,0};
                tglW=std::min((int)S(190),rightLimit-SI(10)); tx=std::max(SI(10),(w-tglW)/2);
                if(tx+tglW>rightLimit) tx=std::max(SI(10),rightLimit-tglW);
            }
            R_shapeTgl={tx,(LONG)S(6),tx+tglW,(LONG)S(48)};
        }
        R_library={0,0,0,0}; R_modeSquare=R_modeCd=R_modeVertical={0,0,0,0};
        g_listScroll=0;
    } else {
        int headerH=SI(56), margin=SI(24);
        R_heart={w-SI(96)-chrome,SI(12),w-SI(60)-chrome,SI(50)}; R_gear={w-SI(58)-chrome,SI(10),w-SI(14)-chrome,SI(54)};
        R_listBtn={w-SI(152)-chrome,SI(10),w-SI(100)-chrome,SI(54)};
        R_shapeTgl={SI(92),SI(8),SI(92)+(int)S(170),SI(52)};
        R_autoTgl={R_shapeTgl.right+SI(10),SI(8),R_shapeTgl.right+SI(10)+(int)S(74),SI(52)};
        R_sortBtn={R_autoTgl.right+SI(10),SI(8),R_autoTgl.right+SI(10)+(int)S(170),SI(52)};
        R_folderBtn={R_sortBtn.right+SI(10),SI(8),R_sortBtn.right+SI(10)+(int)S(g_view==2?160.f:100.f),SI(52)};   // playlist aberta: "PASTA DA PLAYLIST"
        int limit=R_listBtn.left-SI(10);
        if(R_folderBtn.right>limit){ R_folderBtn.right=std::max((int)R_folderBtn.left,limit); }
        if(R_folderBtn.right-R_folderBtn.left<SI(40)){ R_folderBtn={0,0,0,0}; if(R_sortBtn.right>limit) R_sortBtn.right=std::max((int)R_sortBtn.left,limit); }
        R_modeSquare=R_modeCd=R_modeVertical={0,0,0,0}; R_themeCircles.clear();
        float ps=g_cfg.playerScale/100.0f;
        int availH=h-headerH-SI(28);
        int overhead=SI(44)+SI(30)+SI(26)+SI(40)+SI(36)+SI(66)+SI(26)+SI(20);
        int art=(int)(SI(300)*ps);
        art=std::max(SI(150),std::min(art,std::min(availH-overhead,(int)(w*.55f))));
        g_panelArt=art;
        int pad=SI(22), panelW=art+pad*2, panelY=headerH+SI(6);
        R_playerPanel={margin,panelY,margin+panelW,panelY+art+overhead};
        R_art={margin+pad,panelY+pad,margin+pad+art,panelY+pad+art};
        int x0=R_art.left;
        int ay=R_art.bottom+SI(30);
        int wy=ay+SI(34), wb=wy+SI(38);
        R_wavePanel={x0,wy,x0+art,wb};
        int sy2=wb+SI(18);
        R_seek={x0,sy2-9,x0+art,sy2+9};
        int tcy=sy2+SI(54);
        int ccx=x0+art/2;
        R_play={ccx-SI(32),tcy-SI(32),ccx+SI(32),tcy+SI(32)};
        R_prev={ccx-SI(80)-SI(21),tcy-SI(20),ccx-SI(80)+SI(21),tcy+SI(20)};
        R_next={ccx+SI(80)-SI(21),tcy-SI(20),ccx+SI(80)+SI(21),tcy+SI(20)};
        R_shuffle={ccx-SI(148)-SI(17),tcy-SI(15),ccx-SI(148)+SI(17),tcy+SI(15)};
        R_repeat={ccx+SI(148)-SI(17),tcy-SI(15),ccx+SI(148)+SI(17),tcy+SI(15)};
        R_volIcon={x0,tcy+SI(38),x0+SI(22),tcy+SI(60)};
        R_vol={x0+SI(30),tcy+SI(46),x0+art-SI(44),tcy+SI(52)};
        R_runnerSlider={R_playerPanel.right-SI(22),R_playerPanel.bottom-SI(22),R_playerPanel.right,R_playerPanel.bottom};
        int gx=R_playerPanel.right+SI(18);
        int gw=w-margin-gx;
        if(gw>=SI(280)){
            // barra da biblioteca: abas MUSICAS/PLAYLISTS (ou voltar + nome da playlist) e a busca
            int top=headerH+SI(6), barH=SI(40);
            R_libBar={gx,top,w-margin,top+barH};
            int by0=top+SI(4), by1=top+barH-SI(4);
            int sx;
            if(g_pickMode&&g_view==0){   // marcando musicas da biblioteca para uma playlist
                int dw=std::min((int)S(190),gw/3), cw2=std::min((int)S(120),gw/4);
                R_pickDone={gx,by0,gx+dw,by1}; R_pickCancel={R_pickDone.right+SI(8),by0,R_pickDone.right+SI(8)+cw2,by1};
                sx=(int)R_pickCancel.right+SI(12);
            } else if(g_view==2){        // playlist aberta: voltar, + ADICIONAR, busca e (se tiver online) o modo
                int bw=std::min((int)S(140),gw/4), aw=std::min((int)S(130),gw/4);
                R_plBack={gx,by0,gx+bw,by1}; R_plAdd={R_plBack.right+SI(8),by0,R_plBack.right+SI(8)+aw,by1};
                sx=(int)R_plAdd.right+SI(12);
                bool hasOnline=false;
                if(const Playlist* op=OpenPlaylistPtr()){ hasOnline=!op->link.empty(); for(auto& e:op->entries) if(!e.url.empty()){ hasOnline=true; break; } }
                if(hasOnline&&w-margin-sx>=SI(320)){ int mw=(int)S(124); R_plMode={w-margin-mw,by0,w-margin,by1}; }
            } else {                     // abas MUSICAS / PLAYLISTS / ONLINE (encolhem se faltar espaco)
                int tabW=(int)S(104), onW=(int)S(92), gap=SI(8);
                int need=tabW*2+onW+gap*2;
                if(need>gw-SI(60)){ float k=(float)std::max(SI(150),gw-SI(60))/(float)need; tabW=(int)(tabW*k); onW=(int)(onW*k); }
                R_tabTracks={gx,by0,gx+tabW,by1}; R_tabPlaylists={R_tabTracks.right+gap,by0,R_tabTracks.right+gap+tabW,by1};
                R_tabOnline={R_tabPlaylists.right+gap,by0,R_tabPlaylists.right+gap+onW,by1};
                sx=(int)R_tabOnline.right+SI(12);
            }
            int sEnd=(R_plMode.right>R_plMode.left)?(int)R_plMode.left-SI(8):w-margin;
            if(g_view==1){ R_plNew={std::max(sx,w-margin-(int)S(180)),by0,w-margin,by1}; }
            else if(sEnd-sx>=SI(140)){ R_searchBox={sx,by0,sEnd,by1}; R_searchClear={R_searchBox.right-SI(34),by0,R_searchBox.right,by1}; }
            int gridTop=top+barH+SI(8);
            R_library={gx,gridTop,w-margin,h-margin};
            int libH=R_library.bottom-R_library.top;
            if(g_view==1){
                // cards de playlists: "Todas as musicas" + cada playlist + "nova"
                int cols=gw>=SI(760)?3:(gw>=SI(500)?2:1); g_gridCols=cols;
                int gapX=SI(20),gapY=SI(18),cardH=SI(190); int cardW=(gw-(cols-1)*gapX)/cols;
                int n=(int)g_playlists.size()+2;
                g_contentH=((n+cols-1)/cols)*(cardH+gapY);
                g_listScroll=std::max(0,std::min(g_listScroll,std::max(0,g_contentH-libH)));
                for(int k=0;k<n;k++){
                    int row=k/cols,col=k%cols; int x=gx+col*(cardW+gapX),y=gridTop+row*(cardH+gapY)-g_listScroll;
                    if(y>h||y+cardH<gridTop-SI(40)){ R_plCards.push_back({0,0,0,0}); R_plPlay.push_back({0,0,0,0}); R_plShuf.push_back({0,0,0,0}); continue; }
                    R_plCards.push_back({x,y,x+cardW,y+cardH});
                    if(k==n-1){ R_plPlay.push_back({0,0,0,0}); R_plShuf.push_back({0,0,0,0}); }
                    else { int bw2=(cardW-SI(40))/2; R_plPlay.push_back({x+SI(16),y+cardH-SI(50),x+SI(16)+bw2,y+cardH-SI(16)}); R_plShuf.push_back({x+SI(24)+bw2,y+cardH-SI(50),x+cardW-SI(16),y+cardH-SI(16)}); }
                }
            } else {
                R_cardRects.assign(g_tracks.size(),RECT{0,0,0,0}); R_cardCoverButtons.assign(g_tracks.size(),RECT{0,0,0,0}); R_cardSeekRects.assign(g_tracks.size(),RECT{0,0,0,0});
                R_rowUp.assign(g_tracks.size(),RECT{0,0,0,0}); R_rowDown.assign(g_tracks.size(),RECT{0,0,0,0});
                if(g_cfg.listMode!=0){
                    int rowH=SI(64);
                    g_gridCols=1; g_contentH=(int)g_visible.size()*rowH;
                    g_listScroll=std::max(0,std::min(g_listScroll,std::max(0,g_contentH-libH)));
                    for(size_t vi=0;vi<g_visible.size();++vi){
                        size_t i=(size_t)g_visible[vi];
                        int y=gridTop+(int)vi*rowH-g_listScroll;
                        if(y>h||y+rowH<gridTop) continue;
                        RECT rr={gx,y,R_library.right,y+rowH-SI(8)};
                        R_cardRects[i]=rr;
                        if(manual){ R_rowUp[i]={rr.right-SI(74),rr.top+SI(6),rr.right-SI(44),rr.top+SI(28)}; R_rowDown[i]={rr.right-SI(74),rr.top+SI(30),rr.right-SI(44),rr.top+SI(52)}; }
                    }
                } else {
                    int cols=gw>=SI(760)?3:(gw>=SI(500)?2:1);
                    g_gridCols=cols;
                    int gapX=SI(20),gapY=SI(18),cardH=SI(225);
                    int cardW=(gw-(cols-1)*gapX)/cols;
                    g_contentH=(int)(((int)g_visible.size()+cols-1)/cols)*(cardH+gapY);
                    g_listScroll=std::max(0,std::min(g_listScroll,std::max(0,g_contentH-libH)));
                    for(size_t vi=0;vi<g_visible.size();++vi){
                        size_t i=(size_t)g_visible[vi];
                        int row=(int)vi/cols,col=(int)vi%cols;
                        int x=gx+col*(cardW+gapX),y=gridTop+row*(cardH+gapY)-g_listScroll;
                        if(y>h||y+cardH<gridTop-SI(40)) continue;
                        R_cardRects[i]={x,y,x+cardW,y+cardH};
                        R_cardCoverButtons[i]={x+cardW-SI(42),y+SI(16),x+cardW-SI(14),y+SI(44)};
                        // so a linha da barra de seek (nao invade os botoes de transporte)
                        R_cardSeekRects[i]={x+SI(180),y+SI(140),x+cardW-SI(22),y+SI(160)};
                        if(manual){ R_rowUp[i]={x+cardW-SI(42),y+SI(50),x+cardW-SI(14),y+SI(72)}; R_rowDown[i]={x+cardW-SI(42),y+SI(76),x+cardW-SI(14),y+SI(98)}; }
                    }
                }
            }
            // lista vazia: texto + botao no meio da area da lista (antes o texto ficava por cima da barra)
            if(g_view!=1&&g_tracks.empty()){ int cx=(gx+w-margin)/2, bw2=std::min((int)S(300),gw-SI(20)); int ey=gridTop+std::min(SI(150),libH/3); R_onlineInfo={cx-bw2/2,ey,cx+bw2/2,ey+SI(40)}; }
        } else { R_library={0,0,0,0}; }
    }
    LayoutSettings(w,h);
}
static RECT TrackRowRect(int i){ int vi=-1; for(size_t k=0;k<g_visible.size();++k) if(g_visible[k]==i){ vi=(int)k; break; } if(vi<0) return {0,0,0,0}; return {R_library.left,R_library.top+vi*SI(ROW_H)-g_listScroll,R_library.right,R_library.top+vi*SI(ROW_H)-g_listScroll+SI(ROW_H-4)}; }
static int MaxScroll(){
    if(R_library.right-R_library.left<=0) return 0;
    return std::max(0,(int)(g_contentH-(R_library.bottom-R_library.top)));
}
// Geometria do seletor de imagem da web (cabe em qualquer tamanho de janela).
static void LayoutWebPick(int w,int h){
    WebPick& wb=WP();
    int bw=std::min(760,w-40),bh=std::min(620,h-40),bx=(w-bw)/2,by=(h-bh)/2;
    wb.box={bx,by,bx+bw,by+bh};
    wb.btnClose={(LONG)(bx+bw-S(44)),(LONG)(by+S(10)),(LONG)(bx+bw-S(12)),(LONG)(by+S(42))};
    wb.qbox={(LONG)(bx+S(16)),(LONG)(by+S(12)),(LONG)(bx+bw-S(206)),(LONG)(by+S(48))};
    wb.btnSearch={(LONG)(bx+bw-S(198)),(LONG)(by+S(12)),(LONG)(bx+bw-S(52)),(LONG)(by+S(48))};
    size_t n;
    {std::lock_guard<std::mutex> lk(wb.m);n=wb.res.size();}
    int gridW=bw-(int)S(32);
    int cols=gridW/(int)S(170); if(cols<1)cols=1; if(cols>6)cols=6;
    int cellW=(gridW-(cols-1)*(int)S(10))/cols,cellH=(int)S(140);
    int gridBottom=by+bh-(int)S(64);
    wb.cells.clear();
    for(size_t i=0;i<n;++i){
        int row=(int)(i/cols),col=(int)(i%cols);
        int cx=bx+(int)S(16)+col*(cellW+(int)S(10));
        int cy=by+(int)S(60)+row*(cellH+(int)S(10))-wb.scroll;
        if(cy>gridBottom||cy+cellH<by+(int)S(56)){wb.cells.push_back({0,0,0,0});continue;}
        wb.cells.push_back({cx,cy,cx+cellW,cy+cellH});
    }
    wb.btnUse={(LONG)(bx+bw-S(252)),(LONG)(by+bh-S(50)),(LONG)(bx+bw-S(132)),(LONG)(by+bh-S(14))};
    wb.btnCancel={(LONG)(bx+bw-S(124)),(LONG)(by+bh-S(50)),(LONG)(bx+bw-S(16)),(LONG)(by+bh-S(14))};
}
// Pilula de downloads (canto inferior direito, modo normal): calculada a cada quadro.
static bool LayoutActivity(int w,int h,int& waiting,float& pct,std::wstring& title){
    R_activity={0,0,0,0};
    if(g_cfg.displayMode==L"vertical"||g_showSettings||!DownloadActivity(waiting,pct,title)) return false;
    int aw=std::min((int)S(360),w-SI(40));
    R_activity={w-SI(16)-aw,h-SI(54),w-SI(16),h-SI(18)};
    return true;
}
// Geometria da tela de busca online: busca, fontes e resultados em linhas com ▶ ⬇ +.
static RECT R_onList;
static void LayoutOnline(int w,int h){
    OnlineUI& u=OU();
    int bw=std::min((int)S(860),w-24), bh=std::min((int)S(660),h-24), bx=(w-bw)/2, by=(h-bh)/2;
    u.box={bx,by,bx+bw,by+bh};
    u.btnClose={bx+bw-SI(46),by+SI(12),bx+bw-SI(12),by+SI(46)};
    int qy=by+SI(56);
    u.btnSearch={bx+bw-SI(16)-(int)S(120),qy,bx+bw-SI(16),qy+SI(40)};
    u.qbox={bx+SI(16),qy,u.btnSearch.left-SI(8),qy+SI(40)};
    int sy=qy+SI(50), sw=std::min((int)S(150),(bw-SI(48))/3);
    for(int i=0;i<3;i++) u.src[i]={bx+SI(16)+i*(sw+SI(8)),sy,bx+SI(16)+i*(sw+SI(8))+sw,sy+SI(32)};
    int listTop=sy+SI(44), listBot=by+bh-SI(62), rowH=SI(58);
    R_onList={bx+SI(8),listTop,bx+bw-SI(8),listBot};
    size_t n; { std::lock_guard<std::mutex> lk(u.m); n=u.res.size(); }
    int maxSc=std::max(0,(int)n*rowH-(listBot-listTop)); u.scroll=std::max(0,std::min(u.scroll,maxSc));
    u.rows.assign(n,RECT{0,0,0,0}); u.bPlay.assign(n,RECT{0,0,0,0}); u.bDl.assign(n,RECT{0,0,0,0}); u.bAdd.assign(n,RECT{0,0,0,0});
    for(size_t i=0;i<n;++i){
        int y=listTop+(int)i*rowH-u.scroll; if(y+rowH<=listTop||y>=listBot) continue;
        RECT r={bx+SI(16),y,bx+bw-SI(16),y+rowH-SI(6)}; u.rows[i]=r;
        int bs=SI(38), cy=(r.top+r.bottom)/2;
        u.bAdd[i]={r.right-SI(8)-bs,cy-bs/2,r.right-SI(8),cy+bs/2};
        u.bDl[i]={u.bAdd[i].left-SI(6)-bs,cy-bs/2,u.bAdd[i].left-SI(6),cy+bs/2};
        u.bPlay[i]={u.bDl[i].left-SI(6)-bs,cy-bs/2,u.bDl[i].left-SI(6),cy+bs/2};
    }
    u.btnAddAll=n>0?RECT{bx+bw-SI(16)-(int)S(260),by+bh-SI(50),bx+bw-SI(16),by+bh-SI(14)}:RECT{0,0,0,0};
}
// Geometria do editor de texto (artista / nome do arquivo).
static void LayoutEditor(int w,int h){
    int bw=std::min(g_editMode>=4?640:480,w-40), bh=180, bx=(w-bw)/2, by=(h-bh)/2;   // link: caixa mais larga
    R_editBox={bx,by,bx+bw,by+bh};
    R_editSave={bx+bw-238,by+bh-56,bx+bw-128,by+bh-18};
    R_editCancel={bx+bw-118,by+bh-56,bx+bw-20,by+bh-18};
}
