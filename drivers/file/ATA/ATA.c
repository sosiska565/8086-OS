#include "drivers/file/ATA/ATA.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "global.h"
#include "drivers/AHCI/AHCI.h"
#include "fs/fat/fat32.h"
#include "utils/utils.h"
#include "mm/memory.h"
#include "drivers/timer/timer.h"
#include "fs/cache.h"

uint32_t ramdisk_start = 0;
uint32_t ramdisk_size = 0;

drive_info_t sys_drives[MAX_SYS_DRIVES];
int sys_drive_count = 0;

static int ata_identify_ext(uint16_t base, uint8_t drive_id, struct disk_struct *ds) {
    uint8_t drive_head = (drive_id % 2 == 0) ? 0xA0 : 0xB0;
    outb(base + 6, drive_head);
    sleep(5); 

    outb(base + 2, 0); outb(base + 3, 0); outb(base + 4, 0); outb(base + 5, 0);
    outb(base + 7, ATA_CMD_IDENTIFY);
    sleep(5);

    uint8_t status = inb(base + 7);
    if(status == 0 || status == 0xFF) return 0; 

    int timeout = 2000;
    while((inb(base + 7) & ATA_SR_BSY) && timeout > 0) { sleep(1); timeout--; }
    if (timeout <= 0) return 0;

    uint8_t lba_mid = inb(base + 4); uint8_t lba_high = inb(base + 5);
    if (lba_mid != 0 || lba_high != 0) return 0; 

    timeout = 2000;
    while(!(inb(base + 7) & ATA_SR_DRQ) && timeout > 0) {
        if (inb(base + 7) & ATA_SR_ERR) return 0;
        sleep(1); timeout--;
    }
    if (timeout <= 0) return 0;

    for(int i = 0; i < 256; i++) ds->buffer[i] = inw(base + 0);

    int name_idx = 0;
    for(int word_idx = 27; word_idx <= 46 && name_idx < 40; word_idx++) {
        uint16_t word = ds->buffer[word_idx];
        ds->name[name_idx++] = (word >> 8) & 0xFF; ds->name[name_idx++] = word & 0xFF;
    }
    ds->name[name_idx] = '\0';
    int len = strlen(ds->name);
    while(len > 0 && ds->name[len-1] == ' ') ds->name[--len] = '\0';
    if(ds->name[0] == '\0') strcpy(ds->name, "Unknown ATA Drive");
    return 1;
}

void disk_manager_init(void) {
    klog("[ATA] Probing IDE Master & Slave drives (Primary & Secondary)...");
    sys_drive_count = 0;
    struct disk_struct ds;
    const char* channel_names[] = {"Primary Master", "Primary Slave", "Secondary Master", "Secondary Slave"};
    uint16_t bases[] = {0x1F0, 0x1F0, 0x170, 0x170};

    for (int i = 0; i < 4; i++) {
        if (ata_identify_ext(bases[i], i, &ds)) {
            if (sys_drive_count >= MAX_SYS_DRIVES) break;
            sys_drives[sys_drive_count].type = DRIVE_TYPE_ATA;
            sys_drives[sys_drive_count].ata_id = i; 
            char full_name[64];
            strcpy(full_name, channel_names[i]); strcat(full_name, " - "); strcat(full_name, ds.name);
            strcpy(sys_drives[sys_drive_count].name, full_name);
            sys_drive_count++;
        }
    }
}

int disk_read_sector(int drive_id, uint32_t lba, uint8_t *buffer) {
    if (drive_id < 0 || drive_id >= sys_drive_count) return 0;
    
    if (cache_read(drive_id, lba, buffer)) return 1;

    drive_info_t *drv = &sys_drives[drive_id];
    int res = 0;

    if (drv->type == DRIVE_TYPE_RAMDISK) {
        if (lba * 512 >= ramdisk_size) return 0;
        fast_memcpy(buffer, (void*)(ramdisk_start + (lba * 512)), 512);
        res = 1;
    } else if (drv->type == DRIVE_TYPE_AHCI) {
        res = ahci_read_sector((HBA_PORT*)drv->ahci_port, lba, 0, 1, buffer);
    } else if (drv->type == DRIVE_TYPE_ATA) {
        uint16_t base = (drv->ata_id < 2) ? 0x1F0 : 0x170; 
        uint8_t drive_head = (drv->ata_id % 2 == 0) ? 0xE0 : 0xF0; 
        if (inb(base + 7) == 0xFF) return 0;
        outb(base + 6, drive_head | ((lba >> 24) & 0x0F));
        sleep(1);
        outb(base + 1, 0x00); outb(base + 2, 1); outb(base + 3, (uint8_t)(lba & 0xFF));
        outb(base + 4, (uint8_t)((lba >> 8) & 0xFF)); outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));
        outb(base + 7, ATA_CMD_READ_PIO);
        
        int timeout = 2000;
        while ((inb(base + 7) & ATA_SR_BSY) && timeout > 0) { sleep(1); timeout--; }
        if (inb(base + 7) & ATA_SR_ERR) return 0;
        timeout = 2000;
        while (!(inb(base + 7) & ATA_SR_DRQ) && timeout > 0) { sleep(1); timeout--; }
        if (timeout <= 0) return 0;
        
        uint16_t* ptr = (uint16_t*)buffer;
        for(int i = 0; i < 256; i++) ptr[i] = inw(base + 0);
        res = 1;
    }
    
    if (res) cache_write_thru(drive_id, lba, buffer);
    return res;
}

int disk_write_sector(int drive_id, uint32_t lba, uint8_t *buffer) {
    if(isReadMode == 1) return 0;
    if (drive_id < 0 || drive_id >= sys_drive_count) return 0;
    drive_info_t *drv = &sys_drives[drive_id];
    int res = 0;

    if (drv->type == DRIVE_TYPE_RAMDISK) {
        if (lba * 512 >= ramdisk_size) return 0;
        fast_memcpy((void*)(ramdisk_start + (lba * 512)), buffer, 512);
        res = 1;
    } else if (drv->type == DRIVE_TYPE_AHCI) {
        res = ahci_write_sector((HBA_PORT*)drv->ahci_port, lba, 0, 1, buffer);
    } else if (drv->type == DRIVE_TYPE_ATA) {
        uint16_t base = (drv->ata_id < 2) ? 0x1F0 : 0x170;
        uint8_t drive_flag = ((drv->ata_id % 2 == 0) ? 0xE0 : 0xF0) | ((lba >> 24) & 0x0F);
        
        outb(base + 6, drive_flag);
        sleep(1);
        outb(base + 1, 0x00); outb(base + 2, 1); outb(base + 3, (uint8_t)(lba & 0xFF));
        outb(base + 4, (uint8_t)((lba >> 8) & 0xFF)); outb(base + 5, (uint8_t)((lba >> 16) & 0xFF));
        outb(base + 7, ATA_CMD_WRITE_PIO);
        
        int timeout = 2000;
        while ((inb(base + 7) & ATA_SR_BSY) && timeout > 0) { sleep(1); timeout--; }
        timeout = 2000;
        while (!(inb(base + 7) & ATA_SR_DRQ) && timeout > 0) { sleep(1); timeout--; }
        
        uint16_t* ptr = (uint16_t*)buffer;
        for(int i = 0; i < 256; i++){ outw(base + 0, ptr[i]); __asm__ volatile("nop\nnop\nnop"); }
        outb(base + 7, 0xE7); 
        timeout = 2000;
        while ((inb(base + 7) & ATA_SR_BSY) && timeout > 0) { sleep(1); timeout--; }
        res = 1;
    }
    
    if(res) cache_write_thru(drive_id, lba, buffer);
    return res;
}

int ata_read_sector(uint32_t lba, uint8_t *buffer, uint8_t drive) { return 0; }
void ata_write_sector(uint32_t lba, uint8_t *buffer) { }
void ata_identify(uint8_t drive, struct disk_struct *ds) { }