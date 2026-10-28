#ifndef BREADOS_ATA_H
#define BREADOS_ATA_H
#include <stdint.h>

/* Primary-master ATA PIO. Returns 1 when an LBA-capable disk is available. */
int ata_init(void);
int ata_read_sector(uint32_t lba, void *buffer);
int ata_write_sector(uint32_t lba, const void *buffer);
uint32_t ata_sector_count(void);

#endif
