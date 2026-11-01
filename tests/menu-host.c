#include <assert.h>
#include <stdio.h>
#include "menu.h"

int main(void) {
    assert(shell_menu_count()>=20u);
    assert(shell_menu_matches(shell_menu_at(0),"system"));
    int theme_entry_found=0;
    for(size_t i=0;i<shell_menu_count();i++)
        if(shell_menu_matches(shell_menu_at(i),"THEME")) theme_entry_found=1;
    assert(theme_entry_found);
    assert(shell_menu_matches(shell_menu_at(10),"file"));
    assert(!shell_menu_matches(shell_menu_at(0),"bluetooth"));
    assert(shell_menu_matches(shell_menu_at(0),""));
    int privacy_found=0;
    for(size_t i=0;i<shell_menu_count();i++) {
        const struct shell_menu_entry *entry=shell_menu_at(i);
        if(shell_menu_matches(entry,"privacy")) privacy_found=1;
    }
    assert(privacy_found);
    int heap_found=0;
    for(size_t i=0;i<shell_menu_count();i++) {
        const struct shell_menu_entry *entry=shell_menu_at(i);
        if(shell_menu_matches(entry,"heapcheck")) heap_found=1;
    }
    assert(heap_found);
    puts("Native command palette regression tests passed.");
    return 0;
}
