#include <stdint.h>
#include "ata.h"

#define ATA_DATA       0x1F0u
#define ATA_ERROR      0x1F1u
#define ATA_FEATURES   0x1F1u
#define ATA_SECTOR_CNT 0x1F2u
#define ATA_LBA0       0x1F3u
#define ATA_LBA1       0x1F4u
#define ATA_LBA2       0x1F5u
#define ATA_DRIVE      0x1F6u
#define ATA_STATUS     0x1F7u
#define ATA_COMMAND    0x1F7u
#define ATA_ALTSTATUS  0x3F6u
#define ATA_CONTROL    0x3F6u
#define ATA_BSY        0x80u
#define ATA_DRQ        0x08u
#define ATA_DF         0x20u
#define ATA_ERR        0x01u
#define ATA_READ       0x20u
#define ATA_WRITE      0x30u
#define ATA_IDENTIFY   0xECu
#define ATA_FLUSH      0xE7u
#define ATA_LBA28_MAX  0x10000000u
#define ATA_TIMEOUT    1000000u

static int available;
static uint32_t sectors;

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}
static void delay_400ns(void) {
    (void)inb(ATA_ALTSTATUS);
    (void)inb(ATA_ALTSTATUS);
    (void)inb(ATA_ALTSTATUS);
    (void)inb(ATA_ALTSTATUS);
}
static int wait_not_busy(uint8_t *last_status) {
    for (uint32_t i = 0; i < ATA_TIMEOUT; ++i) {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0 || status == 0xFFu) return -1;
        if (!(status & ATA_BSY)) {
            if (last_status) *last_status = status;
            return (status & (ATA_ERR | ATA_DF)) ? -1 : 0;
        }
    }
    return -1;
}
static int wait_data_request(void) {
    for (uint32_t i = 0; i < ATA_TIMEOUT; ++i) {
        uint8_t status = inb(ATA_STATUS);
        if (status == 0 || status == 0xFFu) return -1;
        if (!(status & ATA_BSY)) {
            if (status & (ATA_ERR | ATA_DF)) return -1;
            if (status & ATA_DRQ) return 0;
        }
    }
    return -1;
}
int ata_init(void) {
    available = 0;
    sectors = 0;
    outb(ATA_DRIVE, 0xA0u); /* primary channel, master, legacy mode */
    delay_400ns();
    outb(ATA_CONTROL, 0x02u); /* disable ATA device interrupts; polling only */
    outb(ATA_SECTOR_CNT, 0);
    outb(ATA_LBA0, 0);
    outb(ATA_LBA1, 0);
    outb(ATA_LBA2, 0);
    outb(ATA_COMMAND, ATA_IDENTIFY);
    uint8_t status = inb(ATA_STATUS);
    if (status == 0 || status == 0xFFu) return 0;
    if (wait_not_busy(&status) != 0) return 0;
    if (inb(ATA_LBA1) != 0 || inb(ATA_LBA2) != 0) return 0; /* ATAPI/unknown */
    if (wait_data_request() != 0) return 0;

    uint16_t identify[256];
    for (uint32_t i = 0; i < 256; ++i) identify[i] = inw(ATA_DATA);
    sectors = ((uint32_t)identify[61] << 16) | identify[60];
    if (sectors == 0) return 0;
    available = 1;
    return 1;
}
static int begin_lba28(uint32_t lba) {
    if (!available || lba >= sectors || lba >= ATA_LBA28_MAX) return -1;
    if (wait_not_busy(0) != 0) return -1;
    outb(ATA_DRIVE, (uint8_t)(0xE0u | ((lba >> 24) & 0x0Fu)));
    delay_400ns();
    outb(ATA_FEATURES, 0);
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA0, (uint8_t)lba);
    outb(ATA_LBA1, (uint8_t)(lba >> 8));
    outb(ATA_LBA2, (uint8_t)(lba >> 16));
    return 0;
}
int ata_read_sector(uint32_t lba, void *buffer) {
    if (!buffer || begin_lba28(lba) != 0) return -1;
    outb(ATA_COMMAND, ATA_READ);
    if (wait_data_request() != 0) return -1;
    uint16_t *words = (uint16_t *)buffer;
    for (uint32_t i = 0; i < 256; ++i) words[i] = inw(ATA_DATA);
    return wait_not_busy(0);
}
int ata_write_sector(uint32_t lba, const void *buffer) {
    if (!buffer || begin_lba28(lba) != 0) return -1;
    outb(ATA_COMMAND, ATA_WRITE);
    if (wait_data_request() != 0) return -1;
    const uint16_t *words = (const uint16_t *)buffer;
    for (uint32_t i = 0; i < 256; ++i) outw(ATA_DATA, words[i]);
    if (wait_not_busy(0) != 0) return -1;
    outb(ATA_COMMAND, ATA_FLUSH);
    return wait_not_busy(0);
}
uint32_t ata_sector_count(void) { return sectors; }
