#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ata.h"
#include "fs.h"
#include "pfs.h"

#define SECTOR_SIZE 512u
#define SECTOR_COUNT 4096u
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
    assert(fs_write("../escape", "no") == -1);
    assert(fs_write(".", "no") == -1);
    assert(fs_write("..", "no") == -1);
    assert(fs_mkdir(".") == -1);
    assert(fs_mkdir("..") == -1);
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
    assert(pfs_write(".", "no") == -1);
    assert(pfs_write("..", "no") == -1);
    assert(pfs_mkdir(".") == -1);
    assert(pfs_mkdir("..") == -1);
    assert(pfs_write("docs/../escape", "no") == -1);
    /* The metadata and contents survive remounting the same mock disk. */
    assert(pfs_init() == 1);
    assert(pfs_read("note.txt", output, sizeof(output)) == 6);
    assert(strcmp(output, "inside") == 0);
}

static void test_legacy_dot_segment_entries_remain_mountable(void) {
    char output[16];
    make_valid_volume();
    memset(disk[1], 0, SECTOR_SIZE);
    /* Simulate metadata written by a pre-hardening BreadFS v1 release. */
    memcpy(disk[1], "../legacy", sizeof("../legacy"));
    disk[1][FS_NAME_MAX + 1u] = 4u;
    disk[1][FS_NAME_MAX + 3u] = 1u;
    assert(pfs_init() == 1);
    assert(pfs_ready() == 1);
    /* The legacy record is preserved but cannot be addressed through APIs. */
    assert(pfs_read("../legacy", output, sizeof(output)) == -1);
    assert(pfs_write("../new-file", "blocked") == -1);
    assert(pfs_write("normal.txt", "safe") == 4);
    memset(output, 0, sizeof(output));
    assert(pfs_read("normal.txt", output, sizeof(output)) == 4);
    assert(strcmp(output, "safe") == 0);
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


static void test_persistent_binary_files(void) {
    uint8_t source[1300], destination[1300];
    for (size_t i=0;i<sizeof(source);i++) source[i]=(uint8_t)((i*37u)^0xA5u);
    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    assert(pfs_init()==1);
    assert(pfs_write_binary("app.elf",source,sizeof(source))==(int)sizeof(source));
    memset(destination,0,sizeof(destination));
    assert(pfs_read_binary("app.elf",destination,sizeof(destination))==(int)sizeof(destination));
    assert(memcmp(source,destination,sizeof(source))==0);
    assert(pfs_read("app.elf",(char *)destination,sizeof(destination))==-1);
    assert(pfs_init()==1);
    memset(destination,0,sizeof(destination));
    assert(pfs_read_binary("app.elf",destination,sizeof(destination))==(int)sizeof(destination));
    assert(memcmp(source,destination,sizeof(source))==0);
    assert(pfs_move("app.elf","renamed.elf")==0);
    assert(pfs_read_binary("renamed.elf",destination,sizeof(destination))==(int)sizeof(destination));
    assert(pfs_remove("renamed.elf")==0);
    assert(pfs_read_binary("renamed.elf",destination,sizeof(destination))==-1);
    assert(pfs_write_binary("too-large",source,65536u)==-1);
    assert(pfs_write_binary("bad/name",source,sizeof(source))==-1);
}


static void test_duplicate_persistent_entries_are_rejected(void) {
    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    assert(pfs_init()==1);
    assert(pfs_write("alpha","one")==3);
    assert(pfs_write("beta","two")==3);
    const size_t entry_size=FS_NAME_MAX+1u+sizeof(uint16_t)+2u;
    memcpy(disk[1]+entry_size,disk[1],entry_size);
    assert(pfs_init()==0);
    assert(pfs_ready()==0);
}

static void test_invalid_persistent_metadata_is_rejected(void) {
    const size_t entry_size=FS_NAME_MAX+1u+sizeof(uint16_t)+2u;

    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    memcpy(disk[1],"bad",4);
    disk[1][FS_NAME_MAX+1u+sizeof(uint16_t)]=1u;
    disk[1][entry_size-1u]=3u; /* Unknown entry type. */
    assert(pfs_init()==0);
    assert(pfs_ready()==0);

    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    memcpy(disk[1],"bad name",9);
    disk[1][FS_NAME_MAX+1u+sizeof(uint16_t)]=1u;
    assert(pfs_init()==0);
    assert(pfs_ready()==0);

    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    memcpy(disk[1],"docs//",7);
    disk[1][FS_NAME_MAX+1u+sizeof(uint16_t)]=1u;
    disk[1][entry_size-1u]=1u; /* Directory entry. */
    assert(pfs_init()==0);
    assert(pfs_ready()==0);

    /* Unused slots must be completely zeroed, not carry stale metadata. */
    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    disk[1][0]='x';
    assert(pfs_init()==0);
    assert(pfs_ready()==0);

    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    disk[1][entry_size-1u]=1u;
    assert(pfs_init()==0);
    assert(pfs_ready()==0);
}

static void test_orphaned_persistent_child_is_rejected(void) {
    make_valid_volume();
    memset(disk[1],0,SECTOR_SIZE);
    assert(pfs_init()==1);
    assert(pfs_mkdir("docs")==0);
    assert(pfs_write("docs/note","inside")==6);
    /* Remove the parent directory entry directly, leaving its child behind. */
    memset(disk[1],0,FS_NAME_MAX+1u+sizeof(uint16_t)+2u);
    assert(pfs_init()==0);
    assert(pfs_ready()==0);
}

int main(void) {
    test_ram_filesystem_null_arguments();
    test_persistent_filesystem_null_arguments();
    test_ram_directories_and_moves();
    test_persistent_directories_and_moves();
    test_malformed_disk_name_is_rejected();
    test_legacy_dot_segment_entries_remain_mountable();
    test_persistent_binary_files();
    test_duplicate_persistent_entries_are_rejected();
    test_invalid_persistent_metadata_is_rejected();
    test_orphaned_persistent_child_is_rejected();
    puts("Filesystem host regression tests passed.");
    return 0;
}
