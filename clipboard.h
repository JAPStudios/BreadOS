#ifndef BREADOS_CLIPBOARD_H
#define BREADOS_CLIPBOARD_H
#include <stddef.h>

#define BREADOS_CLIPBOARD_HISTORY 16u
#define BREADOS_CLIPBOARD_TEXT_MAX 127u

/* Sensitive entries become the current clipboard but are never retained. */
void clipboard_set(const char *text,int sensitive);
const char *clipboard_current(void);
size_t clipboard_history_count(void);
/* Index zero is the newest retained item. */
const char *clipboard_history_at(size_t index);
int clipboard_restore(size_t index);
void clipboard_clear(void);

#endif
