#include "fs.h"

struct fs_entry {
    char name[FS_NAME_MAX + 1u]; /* Directories are stored with a trailing '/'. */
    char data[FS_DATA_MAX + 1u];
    uint8_t used;
    uint8_t is_dir;
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
    if (!s) return 0;
    while (s[n]) ++n;
    return n;
}
static int valid_path(const char *path) {
    if (!path) return 0;
    size_t n = length(path);
    if (!n || n > FS_NAME_MAX || path[0] == '/' || path[n - 1u] == '/') return 0;
    size_t component = 0;
    size_t component_start = 0;
    for (size_t i = 0; i <= n; ++i) {
        if (i == n || path[i] == '/') {
            if (!component ||
                (component == 1u && path[component_start] == '.') ||
                (component == 2u && path[component_start] == '.' &&
                 path[component_start + 1u] == '.'))
                return 0;
            component = 0;
            component_start = i + 1u;
        } else {
            char c = path[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'))
                return 0;
            ++component;
        }
    }
    return 1;
}
static int find(const char *name) {
    if (!name) return -1;
    for (size_t i = 0; i < FS_MAX_FILES; ++i)
        if (entries[i].used && same(entries[i].name, name)) return (int)i;
    return -1;
}
static int find_dir(const char *path) {
    char name[FS_NAME_MAX + 1u];
    size_t n = length(path);
    if (!valid_path(path) || n >= FS_NAME_MAX) return -1;
    for (size_t i = 0; i < n; ++i) name[i] = path[i];
    name[n] = '/';
    name[n + 1u] = 0;
    int i = find(name);
    return i >= 0 && entries[i].is_dir ? i : -1;
}
static int parent_exists(const char *path) {
    size_t n = length(path);
    for (size_t i = 0; i < n; ++i) {
        if (path[i] == '/') {
            char parent[FS_NAME_MAX + 1u];
            for (size_t j = 0; j < i; ++j) parent[j] = path[j];
            parent[i] = 0;
            if (find_dir(parent) < 0) return 0;
        }
    }
    return 1;
}
static int free_slot(void) {
    for (size_t i = 0; i < FS_MAX_FILES; ++i)
        if (!entries[i].used) return (int)i;
    return -1;
}
void fs_init(void) {
    for (size_t i = 0; i < FS_MAX_FILES; ++i) entries[i].used = 0;
    used_count = 0;
    (void)fs_write("welcome.txt",
        "BreadOS RAM filesystem is working.\n"
        "Commands: ls, mkdir, write, cat, mv, rm, rmdir.\n");
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
int fs_read(const char *path, char *out, size_t capacity) {
    if (!valid_path(path) || !out || capacity == 0) return -1;
    int i = find(path);
    if (i < 0 || entries[i].is_dir) return -1;
    size_t n = length(entries[i].data);
    if (n >= capacity) n = capacity - 1u;
    for (size_t j = 0; j < n; ++j) out[j] = entries[i].data[j];
    out[n] = 0;
    return (int)n;
}
int fs_write(const char *path, const char *data) {
    if (!path || !data || !valid_path(path) || !parent_exists(path)) return -1;
    size_t n = length(data);
    if (n > FS_DATA_MAX) return -2;
    int i = find(path);
    if (i >= 0 && entries[i].is_dir) return -1;
    if (find_dir(path) >= 0) return -1;
    if (i < 0) {
        i = free_slot();
        if (i < 0) return -3;
        size_t path_len = length(path);
        for (size_t j = 0; j <= path_len; ++j) entries[i].name[j] = path[j];
        entries[i].used = 1;
        entries[i].is_dir = 0;
        ++used_count;
    }
    for (size_t j = 0; j <= n; ++j) entries[i].data[j] = data[j];
    return (int)n;
}
int fs_remove(const char *path) {
    if (!valid_path(path)) return -1;
    int i = find(path);
    if (i < 0 || entries[i].is_dir) return -1;
    entries[i].used = 0;
    entries[i].name[0] = 0;
    entries[i].data[0] = 0;
    --used_count;
    return 0;
}
int fs_mkdir(const char *path) {
    size_t n = length(path);
    if (!valid_path(path) || n >= FS_NAME_MAX || !parent_exists(path)) return -1;
    if (find(path) >= 0 || find_dir(path) >= 0) return -1;
    int i = free_slot();
    if (i < 0) return -2;
    for (size_t j = 0; j < n; ++j) entries[i].name[j] = path[j];
    entries[i].name[n] = '/';
    entries[i].name[n + 1u] = 0;
    entries[i].data[0] = 0;
    entries[i].is_dir = 1;
    entries[i].used = 1;
    ++used_count;
    return 0;
}
int fs_rmdir(const char *path) {
    int i = find_dir(path);
    if (i < 0) return -1;
    size_t n = length(entries[i].name);
    for (size_t j = 0; j < FS_MAX_FILES; ++j) {
        if (entries[j].used && (int)j != i) {
            size_t k = 0;
            while (k < n && entries[j].name[k] == entries[i].name[k]) ++k;
            if (k == n) return -2; /* Directory is not empty. */
        }
    }
    entries[i].used = 0;
    entries[i].name[0] = 0;
    --used_count;
    return 0;
}
int fs_move(const char *source, const char *destination) {
    if (!valid_path(source) || !valid_path(destination) ||
        !parent_exists(destination)) return -1;
    int src = find(source);
    if (src < 0 || entries[src].is_dir || find(destination) >= 0 ||
        find_dir(destination) >= 0) return -1;
    size_t n = length(destination);
    for (size_t j = 0; j <= n; ++j) entries[src].name[j] = destination[j];
    return 0;
}
