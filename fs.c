#include "fs.h"

struct fs_entry {
    char name[FS_NAME_MAX + 1u];
    char data[FS_DATA_MAX + 1u];
    uint8_t used;
};

static struct fs_entry entries[FS_MAX_FILES];
static size_t used_count;

static int same(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}

static size_t length(const char *s) {
    size_t n = 0;
    while (s[n]) ++n;
    return n;
}

static int valid_name(const char *name) {
    if (!name) return 0;
    size_t n = length(name);
    if (n == 0 || n > FS_NAME_MAX) return 0;
    for (size_t i = 0; i < n; ++i) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'))
            return 0;
    }
    return 1;
}

static int find(const char *name) {
    if (!name) return -1;
    for (size_t i = 0; i < FS_MAX_FILES; ++i)
        if (entries[i].used && same(entries[i].name, name)) return (int)i;
    return -1;
}

void fs_init(void) {
    for (size_t i = 0; i < FS_MAX_FILES; ++i) entries[i].used = 0;
    used_count = 0;
    (void)fs_write("welcome.txt",
        "BreadOS RAM filesystem is working.\n"
        "Use ls, cat, write, and rm in the shell.\n");
}

size_t fs_count(void) { return used_count; }

const char *fs_name(size_t index) {
    for (size_t i = 0; i < FS_MAX_FILES; ++i) {
        if (entries[i].used) {
            if (index == 0) return entries[i].name;
            --index;
        }
    }
    return (const char *)0;
}

int fs_read(const char *name, char *out, size_t capacity) {
    int i = find(name);
    if (i < 0 || !out || capacity == 0) return -1;
    size_t n = length(entries[i].data);
    if (n >= capacity) n = capacity - 1u;
    for (size_t j = 0; j < n; ++j) out[j] = entries[i].data[j];
    out[n] = 0;
    return (int)n;
}

int fs_write(const char *name, const char *data) {
    if (!name || !data || !valid_name(name)) return -1;
    size_t n = length(data);
    if (n > FS_DATA_MAX) return -2;
    int i = find(name);
    if (i < 0) {
        for (size_t j = 0; j < FS_MAX_FILES; ++j)
            if (!entries[j].used) { i = (int)j; break; }
        if (i < 0) return -3;
        entries[i].used = 1;
        size_t name_len = length(name);
        for (size_t j = 0; j <= name_len; ++j) entries[i].name[j] = name[j];
        ++used_count;
    }
    for (size_t j = 0; j <= n; ++j) entries[i].data[j] = data[j];
    return (int)n;
}

int fs_remove(const char *name) {
    int i = find(name);
    if (i < 0) return -1;
    entries[i].used = 0;
    entries[i].name[0] = 0;
    entries[i].data[0] = 0;
    --used_count;
    return 0;
}
