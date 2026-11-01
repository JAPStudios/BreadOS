#include <assert.h>
#include <stdio.h>
#include "menu.h"

static int has_command(const char *query) {
    for(size_t i=0;i<shell_menu_count();i++)
        if(shell_menu_matches(shell_menu_at(i),query)) return 1;
    return 0;
}

int main(void) {
    assert(shell_menu_count()>=20u);
    assert(shell_menu_matches(shell_menu_at(0),"system"));
    assert(has_command("THEME"));
    assert(has_command("file"));
    assert(!shell_menu_matches(shell_menu_at(0),"bluetooth"));
    assert(shell_menu_matches(shell_menu_at(0),""));
    assert(has_command("privacy"));
    assert(has_command("heapcheck"));
    puts("Native command palette regression tests passed.");
    return 0;
}
