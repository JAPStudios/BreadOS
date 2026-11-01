#ifndef BREADOS_MENU_H
#define BREADOS_MENU_H
#include <stddef.h>

struct shell_menu_entry {
    const char *category;
    const char *command;
    const char *description;
};

size_t shell_menu_count(void);
const struct shell_menu_entry *shell_menu_at(size_t index);
int shell_menu_matches(const struct shell_menu_entry *entry,const char *query);

#endif
