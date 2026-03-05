#ifndef ATA_H
#define ATA_H

#include <stdint.h>

#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECTOR_CNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7

#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_IDENTIFY 0xEC

#define ATA_PRIMARY_IO    0x1F0
#define ATA_SECONDARY_IO  0x170

#define ATA_MASTER  0
#define ATA_SLAVE   1

#define MAX_SYS_DRIVES 8

typedef enum {
    DRIVE_TYPE_NONE,
    DRIVE_TYPE_ATA,
    DRIVE_TYPE_AHCI
} drive_type_t;

typedef struct {
    drive_type_t type;
    uint8_t ata_id;
    void* ahci_port;
    char name[64];
} drive_info_t;

struct disk_struct {
    uint16_t buffer[256];
    char name[256];
};

extern drive_info_t sys_drives[MAX_SYS_DRIVES];
extern int sys_drive_count;
extern int active_drive_index;

void disk_manager_init(void);
int disk_select(int index);

int disk_read_sector(uint32_t lba, uint8_t *buffer);
int disk_write_sector(uint32_t lba, uint8_t *buffer);

int ata_read_sector(uint32_t lba, uint8_t *buffer, uint8_t drive);
void ata_write_sector(uint32_t lba, uint8_t *buffer);
void ata_identify(uint8_t drive, struct disk_struct *ds);

#endif