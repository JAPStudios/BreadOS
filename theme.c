#include "theme.h"
#include "video.h"

static const struct bread_theme themes[] = {
    {"catppuccin-latte",0xEFF1F5u,0x4C4F69u,0x1E66F5u},
    {"catppuccin",0x1E1E2Eu,0xCDD6F4u,0x89B4FAu},
    {"ethereal",0x060B1Eu,0xFFCEADu,0x7D82D9u},
    {"everforest",0x2D353Bu,0xD3C6AAu,0x7FBBB3u},
    {"flexoki-light",0xFFFCF0u,0x100F0Fu,0x205EA6u},
    {"gruvbox",0x282828u,0xD4BE98u,0x7DAEA3u},
    {"hackerman",0x0B0C16u,0xDDF7FFu,0x82FB9Cu},
    {"jade-horse",0x142620u,0xDFEDE5u,0x8BCBB1u},
    {"kanagawa",0x1F1F28u,0xDCD7BAu,0xDCD7BAu},
    {"last-horizon",0x0C0B0Cu,0xFAFCFBu,0xB59790u},
    {"lumon",0x16242Du,0xD6E2EEu,0x8BC9EBu},
    {"lupine",0xFAFAFAu,0x212121u,0x3264EBu},
    {"matte-black",0x121212u,0xBEBEBEu,0xE68E0Du},
    {"miasma",0x222222u,0xC2C2B0u,0x78824Bu},
    {"nord",0x2E3440u,0xD8DEE9u,0x81A1C1u},
    {"retro-82",0x05182Eu,0xF6DCACu,0xFAA968u},
    {"ristretto",0x2C2525u,0xE6D9DBu,0xF38D70u},
    {"rose-pine",0xFAF4EDu,0x575279u,0x56949Fu},
    {"solitude",0x101315u,0xCACCCCu,0x798186u},
    {"starship",0x1C1C1Eu,0xF5F5F7u,0xFF9F0Au},
    {"tokyo-night",0x1A1B26u,0xA9B1D6u,0x7AA2F7u},
    {"vantablack",0x000000u,0xFFFFFFu,0x8D8D8Du},
    {"white",0xFFFFFFu,0x000000u,0x6E6E6Eu}
};

static size_t active_theme=14u;

static int same_name(const char *left,const char *right) {
    if (!left || !right) return 0;
    while (*left && *right) {
        char a=*left++, b=*right++;
        if (a>='A' && a<='Z') a=(char)(a-'A'+'a');
        if (b>='A' && b<='Z') b=(char)(b-'A'+'a');
        if (a!=b) return 0;
    }
    return *left==*right;
}

size_t theme_count(void) {
    return sizeof(themes)/sizeof(themes[0]);
}

const struct bread_theme *theme_at(size_t index) {
    return index<theme_count() ? &themes[index] : 0;
}

const struct bread_theme *theme_find(const char *name) {
    if (!name || !*name) return 0;
    for (size_t i=0;i<theme_count();i++)
        if (same_name(name,themes[i].name)) return &themes[i];
    return 0;
}

const struct bread_theme *theme_current(void) {
    return &themes[active_theme];
}

int theme_apply(const char *name) {
    for (size_t i=0;i<theme_count();i++) {
        if (same_name(name,themes[i].name)) {
            active_theme=i;
            video_console_clear(themes[i].background,themes[i].foreground);
            return 0;
        }
    }
    return -1;
}
