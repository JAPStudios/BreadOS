#include <assert.h>
#include <stdio.h>
#include "menu.h"

int main(void) {
    assert(shell_menu_count()>=20u);
    assert(shell_menu_matches(shell_menu_at(0),"system"));
    assert(shell_menu_matches(shell_menu_at(16),"THEME"));
    assert(shell_menu_matches(shell_menu_at(10),"file"));
    assert(!shell_menu_matches(shell_menu_at(0),"bluetooth"));
    assert(shell_menu_matches(shell_menu_at(0),""));
    puts("Native command palette regression tests passed.");
    return 0;
}
