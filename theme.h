#ifndef BREADOS_THEME_H
#define BREADOS_THEME_H
#include <stddef.h>
#include <stdint.h>

struct bread_theme {
    const char *name;
    uint32_t background;
    uint32_t foreground;
    uint32_t accent;
};

/* Native BreadOS palettes adapted from Omarchy's colors.toml theme data. */
size_t theme_count(void);
const struct bread_theme *theme_at(size_t index);
const struct bread_theme *theme_find(const char *name);
const struct bread_theme *theme_current(void);
int theme_apply(const char *name);

#endif
