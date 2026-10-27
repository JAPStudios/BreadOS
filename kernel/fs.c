#include "fs.h"

#include <string.h>

#define FS_MAGIC 0x42524653u /* "BRFS" */
#define FS_VERSION 1u
#define FS_META_BLOCKS 3u
#define FS_ENTRY_BLOCKS 2u
#define FS_ENTRIES_PER_BLOCK (FS_BLOCK_SIZE / 32u)
#define FS_DATA_FIRST_BLOCK (FS_META_BLOCKS)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_count;
    uint32_t reserved;
    uint8_t padding[FS_BLOCK_SIZE - 16u];
} fs_superblock_t;

typedef struct {
    char name[24];
    uint16_t size;
    uint16_t data_block;
    uint8_t used;
    uint8_t reserved[3];
} fs_entry_t;

typedef char fs_entry_must_be_32_bytes[(sizeof(fs_entry_t) == 32u) ? 1 : -1];

static int device_valid(const fs_block_device_t *d) {
    return d && d->read && d->write && d->block_count > FS_DATA_FIRST_BLOCK &&
           d->block_count <= UINT16_MAX;
}

static int read_block(const fs_t *fs, uint32_t block, uint8_t out[FS_BLOCK_SIZE]) {
    if (block >= fs->device.block_count) return -1;
    return fs->device.read(fs->device.context, block, out);
}

static int write_block(const fs_t *fs, uint32_t block, const uint8_t data[FS_BLOCK_SIZE]) {
    if (block >= fs->device.block_count) return -1;
    return fs->device.write(fs->device.context, block, data);
}

static int load_entries(fs_t *fs, fs_entry_t entries[FS_MAX_FILES]) {
    uint8_t block[FS_BLOCK_SIZE];
    for (uint32_t b = 0; b < FS_ENTRY_BLOCKS; ++b) {
        if (read_block(fs, 1u + b, block) != 0) return -1;
        memcpy(entries + b * FS_ENTRIES_PER_BLOCK, block, FS_BLOCK_SIZE);
    }
    return 0;
}

static int save_entries(fs_t *fs, const fs_entry_t entries[FS_MAX_FILES]) {
    uint8_t block[FS_BLOCK_SIZE];
    for (uint32_t b = 0; b < FS_ENTRY_BLOCKS; ++b) {
        memcpy(block, entries + b * FS_ENTRIES_PER_BLOCK, FS_BLOCK_SIZE);
        if (write_block(fs, 1u + b, block) != 0) return -1;
    }
    return 0;
}

static int valid_name(const char *name) {
    if (!name || !name[0]) return 0;
    size_t n = 0;
    while (name[n]) {
        if (n >= FS_NAME_MAX) return 0;
        if (name[n] == '/' || name[n] == '\\') return 0;
        ++n;
    }
    return 1;
}

static int find_entry(const fs_entry_t entries[FS_MAX_FILES], const char *name) {
    for (int i = 0; i < (int)FS_MAX_FILES; ++i)
        if (entries[i].used && strncmp(entries[i].name, name, sizeof(entries[i].name)) == 0)
            return i;
    return -1;
}

static int free_entry(const fs_entry_t entries[FS_MAX_FILES]) {
    for (int i = 0; i < (int)FS_MAX_FILES; ++i)
        if (!entries[i].used) return i;
    return -1;
}

static int allocate_block(fs_t *fs, const fs_entry_t entries[FS_MAX_FILES]) {
    uint8_t block[FS_BLOCK_SIZE];
    for (uint32_t candidate = FS_DATA_FIRST_BLOCK; candidate < fs->device.block_count; ++candidate) {
        int in_use = 0;
        for (unsigned i = 0; i < FS_MAX_FILES; ++i)
            if (entries[i].used && entries[i].data_block == candidate) { in_use = 1; break; }
        if (!in_use) {
            /* Clear the block before assigning it to a new file. */
            memset(block, 0, sizeof block);
            if (write_block(fs, candidate, block) != 0) return -1;
            return (int)candidate;
        }
    }
    return -1;
}

int fs_format(const fs_block_device_t *device) {
    if (!device_valid(device)) return -1;
    uint8_t block[FS_BLOCK_SIZE];
    memset(block, 0, sizeof block);
    fs_superblock_t *sb = (fs_superblock_t *)block;
    sb->magic = FS_MAGIC;
    sb->version = FS_VERSION;
    sb->block_count = device->block_count;
    if (device->write(device->context, 0, block) != 0) return -1;
    memset(block, 0, sizeof block);
    if (device->write(device->context, 1, block) != 0) return -1;
    if (device->write(device->context, 2, block) != 0) return -1;
    return 0;
}

int fs_mount(fs_t *fs, const fs_block_device_t *device) {
    if (!fs || !device_valid(device)) return -1;
    uint8_t block[FS_BLOCK_SIZE];
    if (device->read(device->context, 0, block) != 0) return -1;
    const fs_superblock_t *sb = (const fs_superblock_t *)block;
    if (sb->magic != FS_MAGIC || sb->version != FS_VERSION ||
        sb->block_count != device->block_count) return -1;
    fs->device = *device;
    fs->mounted = 1;
    return 0;
}

int fs_create(fs_t *fs, const char *name) {
    if (!fs || !fs->mounted || !valid_name(name)) return -1;
    fs_entry_t entries[FS_MAX_FILES];
    if (load_entries(fs, entries) != 0 || find_entry(entries, name) >= 0) return -1;
    int slot = free_entry(entries);
    int block = allocate_block(fs, entries);
    if (slot < 0 || block < 0) return -1;
    memset(&entries[slot], 0, sizeof entries[slot]);
    memcpy(entries[slot].name, name, strlen(name) + 1u);
    entries[slot].data_block = (uint16_t)block;
    entries[slot].used = 1;
    return save_entries(fs, entries);
}

int fs_read(fs_t *fs, const char *name, void *out, size_t capacity, size_t *bytes_read) {
    if (!fs || !fs->mounted || !valid_name(name) || (!out && capacity)) return -1;
    fs_entry_t entries[FS_MAX_FILES];
    if (load_entries(fs, entries) != 0) return -1;
    int slot = find_entry(entries, name);
    if (slot < 0 || capacity < entries[slot].size) return -1;
    uint8_t block[FS_BLOCK_SIZE];
    if (read_block(fs, entries[slot].data_block, block) != 0) return -1;
    if (entries[slot].size) memcpy(out, block, entries[slot].size);
    if (bytes_read) *bytes_read = entries[slot].size;
    return 0;
}

int fs_write(fs_t *fs, const char *name, const void *data, size_t size) {
    if (!fs || !fs->mounted || !valid_name(name) || size > FS_MAX_FILE_SIZE || (!data && size)) return -1;
    fs_entry_t entries[FS_MAX_FILES];
    if (load_entries(fs, entries) != 0) return -1;
    int slot = find_entry(entries, name);
    if (slot < 0) return -1;
    uint8_t block[FS_BLOCK_SIZE];
    memset(block, 0, sizeof block);
    if (size) memcpy(block, data, size);
    if (write_block(fs, entries[slot].data_block, block) != 0) return -1;
    entries[slot].size = (uint16_t)size;
    return save_entries(fs, entries);
}

int fs_move(fs_t *fs, const char *old_name, const char *new_name) {
    if (!fs || !fs->mounted || !valid_name(old_name) || !valid_name(new_name)) return -1;
    fs_entry_t entries[FS_MAX_FILES];
    if (load_entries(fs, entries) != 0) return -1;
    int source = find_entry(entries, old_name);
    if (source < 0 || find_entry(entries, new_name) >= 0) return -1;
    memset(entries[source].name, 0, sizeof entries[source].name);
    memcpy(entries[source].name, new_name, strlen(new_name) + 1u);
    return save_entries(fs, entries);
}

int fs_copy(fs_t *fs, const char *source_name, const char *destination_name) {
    if (!fs || !fs->mounted || !valid_name(source_name) || !valid_name(destination_name)) return -1;
    fs_entry_t entries[FS_MAX_FILES];
    if (load_entries(fs, entries) != 0) return -1;
    int source = find_entry(entries, source_name);
    if (source < 0 || find_entry(entries, destination_name) >= 0) return -1;
    int slot = free_entry(entries);
    if (slot < 0) return -1;
    int block = allocate_block(fs, entries);
    if (block < 0) return -1;
    uint8_t data[FS_BLOCK_SIZE];
    if (read_block(fs, entries[source].data_block, data) != 0 ||
        write_block(fs, (uint32_t)block, data) != 0) return -1;
    entries[slot] = entries[source];
    memset(entries[slot].name, 0, sizeof entries[slot].name);
    memcpy(entries[slot].name, destination_name, strlen(destination_name) + 1u);
    entries[slot].data_block = (uint16_t)block;
    return save_entries(fs, entries);
}

int fs_delete(fs_t *fs, const char *name) {
    if (!fs || !fs->mounted || !valid_name(name)) return -1;
    fs_entry_t entries[FS_MAX_FILES];
    if (load_entries(fs, entries) != 0) return -1;
    int slot = find_entry(entries, name);
    if (slot < 0) return -1;
    memset(&entries[slot], 0, sizeof entries[slot]);
    return save_entries(fs, entries);
}
