#include "../kernel/fs.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST_BLOCKS 64u

typedef struct {
    uint8_t bytes[TEST_BLOCKS][FS_BLOCK_SIZE];
} memory_disk_t;

static int disk_read(void *context, uint32_t block, uint8_t out[FS_BLOCK_SIZE]) {
    memory_disk_t *disk = (memory_disk_t *)context;
    if (block >= TEST_BLOCKS) return -1;
    memcpy(out, disk->bytes[block], FS_BLOCK_SIZE);
    return 0;
}

static int disk_write(void *context, uint32_t block, const uint8_t data[FS_BLOCK_SIZE]) {
    memory_disk_t *disk = (memory_disk_t *)context;
    if (block >= TEST_BLOCKS) return -1;
    memcpy(disk->bytes[block], data, FS_BLOCK_SIZE);
    return 0;
}

int main(void) {
    memory_disk_t disk = {0};
    fs_block_device_t device = { &disk, TEST_BLOCKS, disk_read, disk_write };
    fs_t fs;
    size_t n = 0;
    char output[FS_BLOCK_SIZE + 1] = {0};
    const char payload[] = "BreadOS filesystem data survives a remount.";

    assert(fs_format(&device) == 0);
    assert(fs_mount(&fs, &device) == 0);
    assert(fs_create(&fs, "notes.txt") == 0);
    assert(fs_write(&fs, "notes.txt", payload, sizeof payload) == 0);
    assert(fs_read(&fs, "notes.txt", output, sizeof output, &n) == 0);
    assert(n == sizeof payload && memcmp(output, payload, n) == 0);

    /* Remount from the same backing blocks: no in-memory snapshot is reused. */
    fs_t remounted;
    assert(fs_mount(&remounted, &device) == 0);
    memset(output, 0, sizeof output);
    assert(fs_read(&remounted, "notes.txt", output, sizeof output, &n) == 0);
    assert(n == sizeof payload && memcmp(output, payload, n) == 0);

    assert(fs_copy(&remounted, "notes.txt", "copy.txt") == 0);
    assert(fs_move(&remounted, "copy.txt", "renamed.txt") == 0);
    assert(fs_read(&remounted, "renamed.txt", output, sizeof output, &n) == 0);
    assert(n == sizeof payload && memcmp(output, payload, n) == 0);
    assert(fs_read(&remounted, "copy.txt", output, sizeof output, &n) != 0);
    assert(fs_delete(&remounted, "notes.txt") == 0);
    assert(fs_read(&remounted, "notes.txt", output, sizeof output, &n) != 0);
    assert(fs_read(&remounted, "renamed.txt", output, sizeof output, &n) == 0);
    assert(fs_create(&remounted, "renamed.txt") != 0);
    assert(fs_create(&remounted, "../escape") != 0);
    assert(fs_write(&remounted, "renamed.txt", payload, FS_BLOCK_SIZE + 1u) != 0);

    puts("filesystem tests passed");
    return 0;
}
