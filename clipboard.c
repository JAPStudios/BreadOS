#include "clipboard.h"
#include "secure_memory.h"

static char current[BREADOS_CLIPBOARD_TEXT_MAX+1u];
static char history[BREADOS_CLIPBOARD_HISTORY][BREADOS_CLIPBOARD_TEXT_MAX+1u];
static size_t next_slot,used_count;

static void copy_text(char *destination,const char *source) {
    size_t i=0;
    if(source) while(source[i] && i<BREADOS_CLIPBOARD_TEXT_MAX) {
        destination[i]=source[i];i++;
    }
    destination[i]=0;
}

void clipboard_set(const char *text,int sensitive) {
    copy_text(current,text);
    if(sensitive || !current[0]) return;
    if(used_count) {
        size_t newest=(next_slot+BREADOS_CLIPBOARD_HISTORY-1u)%BREADOS_CLIPBOARD_HISTORY;
        size_t i=0;
        while(history[newest][i] && current[i] && history[newest][i]==current[i]) i++;
        if(!history[newest][i] && !current[i]) return;
    }
    copy_text(history[next_slot],current);
    next_slot=(next_slot+1u)%BREADOS_CLIPBOARD_HISTORY;
    if(used_count<BREADOS_CLIPBOARD_HISTORY) used_count++;
}

const char *clipboard_current(void) { return current; }
size_t clipboard_history_count(void) { return used_count; }

const char *clipboard_history_at(size_t index) {
    if(index>=used_count) return 0;
    size_t slot=(next_slot+BREADOS_CLIPBOARD_HISTORY-1u-index)%BREADOS_CLIPBOARD_HISTORY;
    return history[slot];
}

int clipboard_restore(size_t index) {
    const char *entry=clipboard_history_at(index);
    if(!entry) return -1;
    copy_text(current,entry);
    return 0;
}

void clipboard_clear(void) {
    secure_memzero(current,sizeof(current));
    secure_memzero(history,sizeof(history));
    next_slot=0;used_count=0;
}
