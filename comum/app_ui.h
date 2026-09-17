#pragma once
// Estilo da interface (Windows e Linux): a pessoa escolhe nas configuracoes.
//   CLASSICO - o visual original: painel com contorno da cor do tema, LED,
//              corredor de luz, cards com transporte e onda.
//   LIMPO    - visual sobrio: cinza neutro, superficies lisas encostadas na
//              janela, a cor do tema so no que esta tocando/ativo. Sem LED.
//   SPOTIFY  - o mesmo visual limpo, mas com o LED e o corredor ligados.
// A paleta abaixo e a unica fonte de cor "de fundo" das duas cascas.
#include "platform.h"

// Versao mostrada nas configuracoes (mude junto com app.rc, win_diag.h e os build*.sh).
static const wchar_t* const REMIX_VERSAO = L"1.5.6";

enum : int { UI_CLASSICO = 0, UI_LIMPO = 1, UI_SPOTIFY = 2, UI_STYLE_COUNT = 3 };

struct UiPal {
    COLORREF bg;         // fundo da janela
    COLORREF bar;        // cabecalho e coluna do player (so nos estilos novos)
    COLORREF surface;    // card / linha / caixa
    COLORREF surfaceHi;  // card selecionado ou sob o mouse
    COLORREF border;     // contorno / divisoria discreta
    COLORREF borderHi;   // contorno em destaque
    COLORREF text;       // texto principal
    COLORREF textDim;    // texto secundario
    COLORREF textFaint;  // rotulos pequenos
};
inline const UiPal& UiPalFor(int style) {
    static const UiPal classico = {
        RGB(5,7,18), RGB(5,7,18), RGB(9,12,25), RGB(18,21,34),
        RGB(40,44,65), RGB(60,64,88), RGB(235,236,242), RGB(145,147,160), RGB(120,124,150) };
    static const UiPal limpo = {
        RGB(18,18,18), RGB(10,10,10), RGB(28,28,28), RGB(42,42,42),
        RGB(40,40,40), RGB(64,64,64), RGB(255,255,255), RGB(179,179,179), RGB(138,138,138) };
    static const UiPal spotify = {
        RGB(18,18,18), RGB(0,0,0), RGB(24,24,24), RGB(40,40,40),
        RGB(40,40,40), RGB(64,64,64), RGB(255,255,255), RGB(179,179,179), RGB(138,138,138) };
    return style == UI_CLASSICO ? classico : (style == UI_SPOTIFY ? spotify : limpo);
}
inline const wchar_t* UiStyleName(int style) {
    return style == UI_CLASSICO ? L"CLÁSSICO" : (style == UI_SPOTIFY ? L"SPOTIFY + LED" : L"LIMPO");
}
// Cantos dos estilos novos: card/campo 6, botao 4 (o classico usa os raios antigos).
static const int UI_R_CARD = 6, UI_R_PILL = 4;
