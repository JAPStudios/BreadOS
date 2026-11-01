#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "theme.h"

static uint32_t last_bg,last_fg;
void video_console_clear(uint32_t bg,uint32_t fg) { last_bg=bg; last_fg=fg; }

int main(void) {
    assert(theme_count()==23u);
    assert(theme_find("NORD")!=0);
    assert(theme_find("catppuccin")!=0);
    assert(theme_find("not-a-theme")==0);
    assert(theme_apply("tokyo-night")==0);
    assert(strcmp(theme_current()->name,"tokyo-night")==0);
    assert(last_bg==0x1A1B26u && last_fg==0xA9B1D6u);
    assert(theme_apply("invalid")==-1);
    assert(strcmp(theme_current()->name,"tokyo-night")==0);
    puts("Native theme regression tests passed.");
    return 0;
}
