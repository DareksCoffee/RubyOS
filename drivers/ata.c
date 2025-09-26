#include "ata.h"
#include "../kernel/logger.h"
#include "../kernel/memory.h"
#include "../cpu/ports.h"
#include <string.h>

typedef struct {
    uint16_t io_base;
    uint16_t ctrl_base;
    uint8_t drive;
    uint32_t sectors;
    int exists;
} ata_device_t;

static ata_device_t ata_devices[4] = {0};

static int ata_wait_bsy(uint16_t io_base) {
    int timeout = 500000;
    while ((inb(io_base + ATA_REG_STATUS) & ATA_STATUS_BSY) && timeout--) {
        for (volatile int i = 0; i < 1000; i++);
    }
    return timeout > 0 ? 0 : -1;
}

static int ata_wait_drq(uint16_t io_base) {
    int timeout = 500000;
    while (!(inb(io_base + ATA_REG_STATUS) & ATA_STATUS_DRQ) && timeout--) {
        for (volatile int i = 0; i < 1000; i++);
    }
    return timeout > 0 ? 0 : -1;
}

static void ata_delay(uint16_t io_base) {
    for (int i = 0; i < 14; i++) {
        inb(io_base + ATA_REG_ALTSTATUS);
    }
}

static int ata_identify(uint32_t device_id) {
    if (device_id >= 4) return -1;
    
    ata_device_t* dev = &ata_devices[device_id];
    uint16_t buffer[256];
    
    outb(dev->io_base + ATA_REG_HDDEVSEL, dev->drive);
    ata_delay(dev->io_base);
    
    outb(dev->io_base + ATA_REG_SECCOUNT0, 0);
    outb(dev->io_base + ATA_REG_LBA0, 0);
    outb(dev->io_base + ATA_REG_LBA1, 0);
    outb(dev->io_base + ATA_REG_LBA2, 0);
    outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    if (inb(dev->io_base + ATA_REG_STATUS) == 0) {
        return -1;
    }
    
    if (ata_wait_bsy(dev->io_base) != 0) {
        log(LOG_ERROR, "ATA identify timeout waiting for BSY clear");
        return -1;
    }
    
    if (inb(dev->io_base + ATA_REG_STATUS) & ATA_STATUS_ERR) {
        return -1;
    }
    
    if (ata_wait_drq(dev->io_base) != 0) {
        log(LOG_ERROR, "ATA identify timeout waiting for DRQ");
        return -1;
    }
    
    insw(dev->io_base + ATA_REG_DATA, buffer, 256);
    
    dev->sectors = ((uint32_t)buffer[61] << 16) | buffer[60];
    dev->exists = 1;
    
    return 0;
}

int ata_init(void) {
    log(LOG_DEBUG, "Initializing ATA driver");
    
    ata_devices[0].io_base = ATA_PRIMARY_IO;
    ata_devices[0].ctrl_base = ATA_PRIMARY_CTRL;
    ata_devices[0].drive = ATA_MASTER;
    ata_devices[0].exists = 0;
    
    ata_devices[1].io_base = ATA_PRIMARY_IO;
    ata_devices[1].ctrl_base = ATA_PRIMARY_CTRL;
    ata_devices[1].drive = ATA_SLAVE;
    ata_devices[1].exists = 0;
    
    ata_devices[2].io_base = ATA_SECONDARY_IO;
    ata_devices[2].ctrl_base = ATA_SECONDARY_CTRL;
    ata_devices[2].drive = ATA_MASTER;
    ata_devices[2].exists = 0;
    
    ata_devices[3].io_base = ATA_SECONDARY_IO;
    ata_devices[3].ctrl_base = ATA_SECONDARY_CTRL;
    ata_devices[3].drive = ATA_SLAVE;
    ata_devices[3].exists = 0;
    
    int found_drives = 0;
    for (int i = 0; i < 4; i++) {
        if (ata_identify(i) == 0) {
            log(LOG_OK, "Found ATA drive %d: %d sectors", i, ata_devices[i].sectors);
            found_drives++;
        }
    }
    
    if (found_drives == 0) {
        log(LOG_ERROR, "No ATA drives found");
        return -1;
    }
    
    log(LOG_OK, "ATA driver initialized, found %d drives", found_drives);
    return 0;
}

int ata_drive_exists(uint32_t device_id) {
    if (device_id >= 4) return 0;
    return ata_devices[device_id].exists;
}

uint32_t ata_get_drive_size(uint32_t device_id) {
    if (device_id >= 4 || !ata_devices[device_id].exists) return 0;
    return ata_devices[device_id].sectors;
}

int ata_read_sectors(uint32_t device_id, uint32_t lba, uint32_t count, uint8_t* buffer) {
    if (device_id >= 4 || !ata_devices[device_id].exists || count == 0 || !buffer) {
        return -1;
    }
    
    ata_device_t* dev = &ata_devices[device_id];
    
    if (lba + count > dev->sectors) {
        log(LOG_ERROR, "Read beyond drive capacity");
        return -1;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_lba = lba + i;
        
        outb(dev->io_base + ATA_REG_HDDEVSEL, dev->drive | 0x40 | ((current_lba >> 24) & 0x0F));
        ata_delay(dev->io_base);
        
        outb(dev->io_base + ATA_REG_FEATURES, 0);
        outb(dev->io_base + ATA_REG_SECCOUNT0, 1);
        outb(dev->io_base + ATA_REG_LBA0, current_lba & 0xFF);
        outb(dev->io_base + ATA_REG_LBA1, (current_lba >> 8) & 0xFF);
        outb(dev->io_base + ATA_REG_LBA2, (current_lba >> 16) & 0xFF);
        outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_READ_SECTORS);
        
        if (ata_wait_bsy(dev->io_base) != 0) {
            log(LOG_ERROR, "ATA read timeout waiting for BSY clear (sector %d)", current_lba);
            return -1;
        }
        
        if (inb(dev->io_base + ATA_REG_STATUS) & ATA_STATUS_ERR) {
            log(LOG_ERROR, "ATA read error for sector %d", current_lba);
            return -1;
        }
        
        if (ata_wait_drq(dev->io_base) != 0) {
            log(LOG_ERROR, "ATA read timeout waiting for DRQ (sector %d)", current_lba);
            return -1;
        }

        insw(dev->io_base + ATA_REG_DATA, buffer + i * ATA_SECTOR_SIZE, ATA_SECTOR_SIZE / 2);

        if (ata_wait_bsy(dev->io_base) != 0) {
            log(LOG_ERROR, "ATA read timeout waiting for BSY clear after data transfer (sector %d)", current_lba);
            return -1;
        }
    }
    
    return 0;
}

int ata_write_sectors(uint32_t device_id, uint32_t lba, uint32_t count, const uint8_t* buffer) {
    if (device_id >= 4 || !ata_devices[device_id].exists || count == 0 || !buffer) {
        return -1;
    }
    
    ata_device_t* dev = &ata_devices[device_id];
    
    if (lba + count > dev->sectors) {
        log(LOG_ERROR, "Write beyond drive capacity");
        return -1;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        uint32_t current_lba = lba + i;
        
        outb(dev->io_base + ATA_REG_HDDEVSEL, dev->drive | 0x40 | ((current_lba >> 24) & 0x0F));
        ata_delay(dev->io_base);
        
        outb(dev->io_base + ATA_REG_FEATURES, 0);
        outb(dev->io_base + ATA_REG_SECCOUNT0, 1);
        outb(dev->io_base + ATA_REG_LBA0, current_lba & 0xFF);
        outb(dev->io_base + ATA_REG_LBA1, (current_lba >> 8) & 0xFF);
        outb(dev->io_base + ATA_REG_LBA2, (current_lba >> 16) & 0xFF);
        outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_SECTORS);
        
        if (ata_wait_bsy(dev->io_base) != 0) {
            log(LOG_ERROR, "ATA write timeout waiting for BSY clear (sector %d)", current_lba);
            return -1;
        }
        
        if (inb(dev->io_base + ATA_REG_STATUS) & ATA_STATUS_ERR) {
            log(LOG_ERROR, "ATA write error for sector %d", current_lba);
            return -1;
        }
        
        if (ata_wait_drq(dev->io_base) != 0) {
            log(LOG_ERROR, "ATA write timeout waiting for DRQ (sector %d)", current_lba);
            return -1;
        }

        outsw(dev->io_base + ATA_REG_DATA, buffer + i * ATA_SECTOR_SIZE, ATA_SECTOR_SIZE / 2);

        if (ata_wait_bsy(dev->io_base) != 0) {
            log(LOG_ERROR, "ATA write timeout waiting for BSY clear after data transfer (sector %d)", current_lba);
            return -1;
        }
    }

    // Flush cache to ensure data is written to disk
    outb(dev->io_base + ATA_REG_HDDEVSEL, dev->drive | 0x40);
    ata_delay(dev->io_base);
    outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);

    if (ata_wait_bsy(dev->io_base) != 0) {
        log(LOG_ERROR, "ATA cache flush timeout after write");
        return -1;
    }

    return 0;
}

void ata_list_devices(void) {
    log(LOG_SYSTEM, "ATA Device List:");
    
    const char* bus_names[] = {"Primary", "Secondary"};
    const char* drive_names[] = {"Master", "Slave"};
    
    int found = 0;
    for (int i = 0; i < 4; i++) {
        if (ata_devices[i].exists) {
            int bus = i / 2;
            int drive = i % 2;
            uint32_t size_mb = (ata_devices[i].sectors * ATA_SECTOR_SIZE) / (1024 * 1024);
            
            log(LOG_SYSTEM, "  Device %d: %s %s - %d sectors (%d MB)", 
                i, bus_names[bus], drive_names[drive], ata_devices[i].sectors, size_mb);
            found++;
        }
    }
    
    if (found == 0) {
        log(LOG_SYSTEM, "  No ATA devices found");
    } else {
        log(LOG_SYSTEM, "Total devices found: %d", found);
    }
}