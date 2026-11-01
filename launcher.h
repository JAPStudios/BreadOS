#ifndef BREADOS_LAUNCHER_H
#define BREADOS_LAUNCHER_H
#include <stddef.h>
#include <stdint.h>
struct bread_launcher_entry { const char *name; const char *category; const char *description; uint8_t app_id; };
size_t bread_launcher_search(const char *query, size_t *indices, size_t capacity);
const struct bread_launcher_entry *bread_launcher_entry_at(size_t index);
size_t bread_launcher_entry_count(void);
#endif
