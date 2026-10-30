#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ata.h"
#include "fs.h"
#include "pfs.h"

#define SECTOR_SIZE 512u
#define SECTOR_COUNT 32u
#define PFS_MAGIC 0x31534642u

static uint8_t disk[SECTOR_COUNT][SECTOR_SIZE];

int ata_init(void) { return 1; }
int ata_read_sector(uint32_t lba, void *buffer) {
    if (!buffer || lba >= SECTOR_COUNT) return -1;
    memcpy(buffer, disk[lba], SECTOR_SIZE);
    return 0;
}
int ata_write_sector(uint32_t lba, const void *buffer) {
    if (!buffer || lba >= SECTOR_COUNT) return -1;
    memcpy(disk[lba], buffer, SECTOR_SIZE);
    return 0;
}
uint32_t ata_sector_count(void) { return SECTOR_COUNT; }

static void make_valid_volume(void) {
    memset(disk, 0, sizeof(disk));
    uint32_t header[4] = { PFS_MAGIC, 1u, SECTOR_SIZE, FS_MAX_FILES };
    memcpy(disk[0], header, sizeof(header));
}

static void test_ram_filesystem_null_arguments(void) {
    char output[16];
    fs_init();
    assert(fs_read(NULL, output, sizeof(output)) == -1);
    assert(fs_remove(NULL) == -1);
    assert(fs_write(NULL, "data") == -1);
    assert(fs_write("file", NULL) == -1);
    assert(fs_read("welcome.txt", NULL, sizeof(output)) == -1);
    assert(fs_read("welcome.txt", output, 0) == -1);
}

static void test_persistent_filesystem_null_arguments(void) {
    char output[16];
    make_valid_volume();
    assert(pfs_init() == 1);
    assert(pfs_read(NULL, output, sizeof(output)) == -1);
    assert(pfs_read("file", NULL, sizeof(output)) == -1);
    assert(pfs_read("file", output, 0) == -1);
    assert(pfs_remove(NULL) == -1);
    assert(pfs_write(NULL, "data") == -1);
    assert(pfs_write("file", NULL) == -1);
    assert(pfs_write("note.txt", "hello") == 5);
    memset(output, 0, sizeof(output));
    assert(pfs_read("note.txt", output, sizeof(output)) == 5);
    assert(strcmp(output, "hello") == 0);
}

static void test_ram_directories_and_moves(void) {
    char output[32];
    fs_init();
    assert(fs_mkdir("docs") == 0);
    assert(fs_write("docs", "must not shadow a directory") == -1);
    assert(fs_write("docs/note.txt", "inside") == 6);
    assert(fs_read("docs/note.txt", output, sizeof(output)) == 6);
    assert(strcmp(output, "inside") == 0);
    assert(fs_rmdir("docs") == -2); /* Non-empty directories are protected. */
    assert(fs_move("docs/note.txt", "note.txt") == 0);
    assert(fs_read("note.txt", output, sizeof(output)) == 6);
    assert(strcmp(output, "inside") == 0);
    assert(fs_rmdir("docs") == 0);
    assert(fs_mkdir("bad/name/child") == -1); /* Parent must exist. */
    assert(fs_mkdir(".") == -1);
    assert(fs_mkdir("..") == -1);
    assert(fs_mkdir("docs/..") == -1);
    assert(fs_write("../escape", "no") == -1);
    assert(fs_write("docs/../escape", "no") == -1);
}

static void test_persistent_directories_and_moves(void) {
    char output[32];
    make_valid_volume();
    memset(disk[1], 0, SECTOR_SIZE);
    assert(pfs_init() == 1);
    assert(pfs_mkdir("docs") == 0);
    assert(pfs_write("docs", "must not shadow a directory") == -1);
    assert(pfs_write("docs/note.txt", "inside") == 6);
    assert(pfs_read("docs/note.txt", output, sizeof(output)) == 6);
    assert(strcmp(output, "inside") == 0);
    assert(pfs_rmdir("docs") == -2);
    assert(pfs_move("docs/note.txt", "note.txt") == 0);
    assert(pfs_read("note.txt", output, sizeof(output)) == 6);
    assert(strcmp(output, "inside") == 0);
    assert(pfs_rmdir("docs") == 0);
    assert(pfs_mkdir(".") == -1);
    assert(pfs_mkdir("..") == -1);
    assert(pfs_mkdir("docs/..") == -1);
    assert(pfs_write("../escape", "no") == -1);
    assert(pfs_write("docs/../escape", "no") == -1);
    /* The metadata and contents survive remounting the same mock disk. */
    assert(pfs_init() == 1);
    assert(pfs_read("note.txt", output, sizeof(output)) == 6);
    assert(strcmp(output, "inside") == 0);
}

static void test_malformed_disk_name_is_rejected(void) {
    make_valid_volume();
    /* First entry has no NUL inside its fixed-width on-disk name field. */
    memset(disk[1], 0, SECTOR_SIZE);
    memset(disk[1], 'A', FS_NAME_MAX + 1u);
    disk[1][FS_NAME_MAX + 1u] = 1u; /* size low byte */
    disk[1][FS_NAME_MAX + 3u] = 1u; /* used */
    assert(pfs_init() == 0);
    assert(pfs_ready() == 0);
}

int main(void) {
    test_ram_filesystem_null_arguments();
    test_persistent_filesystem_null_arguments();
    test_ram_directories_and_moves();
    test_persistent_directories_and_moves();
    test_malformed_disk_name_is_rejected();
    puts("Filesystem host regression tests passed.");
    return 0;
}
