#ifndef ATA_H
#define ATA_H

#include <stdint.h>
#define ATA_PRIMARY_IO 0x1F0
#define ATA_SECONDARY_IO 0x170
#define ATA_PRIMARY_CTRL 0x3F6
#define ATA_SECONDARY_CTRL 0x376

#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_FEATURES 0x01
#define ATA_REG_SECCOUNT0 0x02
#define ATA_REG_LBA0 0x03
#define ATA_REG_LBA1 0x04
#define ATA_REG_LBA2 0x05
#define ATA_REG_HDDEVSEL 0x06
#define ATA_REG_COMMAND 0x07
#define ATA_REG_STATUS 0x07

#define ATA_REG_CONTROL 0x00
#define ATA_REG_ALTSTATUS 0x00

#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_STATUS_BSY 0x80
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERR 0x01

#define ATA_MASTER 0xA0
#define ATA_SLAVE 0xB0

#define ATA_SECTOR_SIZE 512

int ata_init(void);
int ata_drive_exists(uint32_t device_id);
uint32_t ata_get_drive_size(uint32_t device_id);
int ata_read_sectors(uint32_t device_id, uint32_t lba, uint32_t count, uint8_t* buffer);
int ata_write_sectors(uint32_t device_id, uint32_t lba, uint32_t count, const uint8_t* buffer);
void ata_list_devices(void);

#endif