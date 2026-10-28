#include <stddef.h>
#include <stdint.h>
#include "ata.h"
#include "fs.h"
#include "pfs.h"

#define PFS_SECTOR_SIZE 512u
#define PFS_SUPER_MAGIC 0x31534642u /* "BFS1" */
#define PFS_VERSION 1u
#define PFS_DIRECTORY_LBA 1u
#define PFS_DATA_LBA 2u

struct pfs_superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t sector_size;
    uint32_t entry_count;
    uint8_t reserved[PFS_SECTOR_SIZE - 16u];
} __attribute__((packed));
typedef char pfs_superblock_must_be_one_sector[
    (sizeof(struct pfs_superblock) == PFS_SECTOR_SIZE) ? 1 : -1];

struct pfs_entry {
    char name[FS_NAME_MAX + 1u];
    uint16_t size;
    uint8_t used;
    uint8_t reserved;
} __attribute__((packed));
typedef char pfs_directory_must_fit_one_sector[
    (sizeof(struct pfs_entry) * FS_MAX_FILES <= PFS_SECTOR_SIZE) ? 1 : -1];

static struct pfs_entry directory[FS_MAX_FILES];
static int ready;

static size_t string_length(const char *s) {
    size_t n = 0;
    while (s[n]) ++n;
    return n;
}
static int same(const char *a, const char *b) {
    while (*a && *b && *a == *b) { ++a; ++b; }
    return *a == *b;
}
static void copy_bytes(void *destination, const void *source, size_t count) {
    uint8_t *d = (uint8_t *)destination;
    const uint8_t *s = (const uint8_t *)source;
    for (size_t i = 0; i < count; ++i) d[i] = s[i];
}
static void zero_bytes(void *destination, size_t count) {
    uint8_t *d = (uint8_t *)destination;
    for (size_t i = 0; i < count; ++i) d[i] = 0;
}
static int valid_name(const char *name) {
    if (!name) return 0;
    size_t n = string_length(name);
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
    for (size_t i = 0; i < FS_MAX_FILES; ++i)
        if (directory[i].used && same(directory[i].name, name)) return (int)i;
    return -1;
}
static int write_directory(void) {
    uint8_t sector[PFS_SECTOR_SIZE];
    zero_bytes(sector, sizeof(sector));
    copy_bytes(sector, directory, sizeof(directory));
    return ata_write_sector(PFS_DIRECTORY_LBA, sector);
}
static int valid_entry(const struct pfs_entry *entry) {
    if (entry->used > 1u) return 0;
    if (!entry->used) return 1;
    if (entry->size > FS_DATA_MAX || !valid_name(entry->name)) return 0;
    return 1;
}
int pfs_init(void) {
    ready = 0;
    zero_bytes(directory, sizeof(directory));
    if (!ata_init()) return 0;
    uint8_t sector[PFS_SECTOR_SIZE];
    if (ata_read_sector(0, sector) != 0) return 0;
    struct pfs_superblock super;
    copy_bytes(&super, sector, sizeof(super));
    if (super.magic != PFS_SUPER_MAGIC) {
        int blank = 1;
        for (size_t i = 0; i < sizeof(sector); ++i)
            if (sector[i] != 0) { blank = 0; break; }
        if (!blank) return -1; /* Never overwrite an unrecognized disk. */
        zero_bytes(directory, sizeof(directory));
        if (write_directory() != 0) return 0;
        zero_bytes(&super, sizeof(super));
        super.magic = PFS_SUPER_MAGIC;
        super.version = PFS_VERSION;
        super.sector_size = PFS_SECTOR_SIZE;
        super.entry_count = FS_MAX_FILES;
        if (ata_write_sector(0, &super) != 0) return 0;
        ready = 1;
        if (pfs_write("welcome.txt",
            "BreadOS persistent filesystem is working.\n"
            "Files on this ATA disk survive reboot.\n") < 0) {
            ready = 0;
            return 0;
        }
        return 2;
    }
    if (super.version != PFS_VERSION ||
        super.sector_size != PFS_SECTOR_SIZE ||
        super.entry_count != FS_MAX_FILES) return -1;
    if (ata_read_sector(PFS_DIRECTORY_LBA, sector) != 0) return 0;
    copy_bytes(directory, sector, sizeof(directory));
    for (size_t i = 0; i < FS_MAX_FILES; ++i)
        if (!valid_entry(&directory[i])) {
            zero_bytes(directory, sizeof(directory));
            return 0;
        }
    ready = 1;
    return 1;
}
int pfs_ready(void) { return ready; }
size_t pfs_count(void) {
    size_t count = 0;
    if (!ready) return 0;
    for (size_t i = 0; i < FS_MAX_FILES; ++i)
        if (directory[i].used) ++count;
    return count;
}
const char *pfs_name(size_t index) {
    if (!ready) return (const char *)0;
    for (size_t i = 0; i < FS_MAX_FILES; ++i) {
        if (directory[i].used) {
            if (index == 0) return directory[i].name;
            --index;
        }
    }
    return (const char *)0;
}
int pfs_read(const char *name, char *out, size_t capacity) {
    int slot = find(name);
    if (!ready || slot < 0 || !out || capacity == 0) return -1;
    uint8_t sector[PFS_SECTOR_SIZE];
    if (ata_read_sector(PFS_DATA_LBA + (uint32_t)slot, sector) != 0) return -1;
    size_t n = directory[slot].size;
    if (n >= capacity) n = capacity - 1u;
    copy_bytes(out, sector, n);
    out[n] = 0;
    return (int)n;
}
int pfs_write(const char *name, const char *data) {
    if (!ready || !name || !data || !valid_name(name)) return -1;
    size_t n = string_length(data);
    if (n > FS_DATA_MAX) return -2;
    int slot = find(name);
    if (slot < 0) {
        for (size_t i = 0; i < FS_MAX_FILES; ++i)
            if (!directory[i].used) { slot = (int)i; break; }
    }
    if (slot < 0) return -3;
    uint8_t sector[PFS_SECTOR_SIZE];
    zero_bytes(sector, sizeof(sector));
    copy_bytes(sector, data, n);
    if (ata_write_sector(PFS_DATA_LBA + (uint32_t)slot, sector) != 0) return -4;
    struct pfs_entry previous = directory[slot];
    if (previous.used == 0) zero_bytes(&directory[slot], sizeof(directory[slot]));
    size_t name_len = string_length(name);
    copy_bytes(directory[slot].name, name, name_len);
    directory[slot].name[name_len] = 0;
    directory[slot].size = (uint16_t)n;
    directory[slot].used = 1;
    if (write_directory() != 0) {
        directory[slot] = previous;
        return -4;
    }
    return (int)n;
}
int pfs_remove(const char *name) {
    if (!ready) return -1;
    int slot = find(name);
    if (slot < 0) return -1;
    struct pfs_entry previous = directory[slot];
    zero_bytes(&directory[slot], sizeof(directory[slot]));
    if (write_directory() != 0) {
        directory[slot] = previous;
        return -1;
    }
    return 0;
}
