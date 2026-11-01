#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "clipboard.h"

int main(void) {
    clipboard_clear();
    clipboard_set("first",0);
    clipboard_set("secret",1);
    assert(strcmp(clipboard_current(),"secret")==0);
    assert(clipboard_history_count()==1u);
    assert(strcmp(clipboard_history_at(0),"first")==0);
    clipboard_set("second",0);
    assert(clipboard_history_count()==2u);
    assert(clipboard_restore(1)==0);
    assert(strcmp(clipboard_current(),"first")==0);
    for(unsigned i=0;i<20;i++) {
        char value[16];value[0]='v';value[1]=(char)('A'+i);value[2]=0;
        clipboard_set(value,0);
    }
    assert(clipboard_history_count()==BREADOS_CLIPBOARD_HISTORY);
    assert(clipboard_history_at(16)==0);
    clipboard_clear();
    assert(!clipboard_current()[0] && clipboard_history_count()==0);
    puts("Clipboard history regression tests passed.");
    return 0;
}
