#include "drivers/file/ATA/ATA.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "global.h"
#include "drivers/AHCI/AHCI.h"
#include "fs/fat/fat32.h"
#include "utils/utils.h"

drive_info_t sys_drives[MAX_SYS_DRIVES];
int sys_drive_count = 0;
int active_drive_index = 0;

int ata_wait_bsy(void){
    int timeout = 10000000; 
    while((inb(ATA_STATUS) & ATA_SR_BSY) && --timeout);
    return timeout > 0;
}

int ata_wait_drq(void){
    int timeout = 10000000;
    while(!(inb(ATA_STATUS) & (ATA_SR_DRQ | ATA_SR_ERR)) && --timeout);
    return timeout > 0;
}

void ata_wait_io() {
    inb(ATA_STATUS); inb(ATA_STATUS); inb(ATA_STATUS); inb(ATA_STATUS);
}

void disk_manager_init(void) {
    klog("[ATA] Probing IDE Master & Slave drives...");
    sys_drive_count = 0;
    active_drive_index = 0;

    struct disk_struct ds;
    
    ata_identify(ATA_MASTER, &ds);
    if (strcmp(ds.name, "NO DISK") != 0 && strcmp(ds.name, "ERROR") != 0 && strcmp(ds.name, "ERROR/CD-ROM") != 0) {
        sys_drives[sys_drive_count].type = DRIVE_TYPE_ATA;
        sys_drives[sys_drive_count].ata_id = ATA_MASTER;
        strcpy(sys_drives[sys_drive_count].name, "ATA Master - ");
        strcat(sys_drives[sys_drive_count].name, ds.name);
        
        char msg[128] = "[ATA] Mapped Master: ";
        strcat(msg, ds.name);
        klog(msg);
        sys_drive_count++;
    }

    ata_identify(ATA_SLAVE, &ds);
    if (strcmp(ds.name, "NO DISK") != 0 && strcmp(ds.name, "ERROR") != 0 && strcmp(ds.name, "ERROR/CD-ROM") != 0) {
        sys_drives[sys_drive_count].type = DRIVE_TYPE_ATA;
        sys_drives[sys_drive_count].ata_id = ATA_SLAVE;
        strcpy(sys_drives[sys_drive_count].name, "ATA Slave - ");
        strcat(sys_drives[sys_drive_count].name, ds.name);
        
        char msg[128] = "[ATA] Mapped Slave: ";
        strcat(msg, ds.name);
        klog(msg);
        sys_drive_count++;
    }
    klog("[ATA] Drive mapping complete.");
}

int disk_select(int index) {
    if (index >= 0 && index < sys_drive_count) {
        active_drive_index = index;
        fat32_init();
        return 1;
    }
    return 0;
}

int disk_read_sector(uint32_t lba, uint8_t *buffer) {
    if (sys_drive_count == 0) return 0;
    drive_info_t *drv = &sys_drives[active_drive_index];

    if (drv->type == DRIVE_TYPE_AHCI) {
        return ahci_read_sector((HBA_PORT*)drv->ahci_port, lba, 0, 1, buffer);
    } else if (drv->type == DRIVE_TYPE_ATA) {
        return ata_read_sector(lba, buffer, drv->ata_id);
    }
    return 0;
}

int disk_write_sector(uint32_t lba, uint8_t *buffer) {
    if(isReadMode == 1) return 0;
    if (sys_drive_count == 0) return 0;

    drive_info_t *drv = &sys_drives[active_drive_index];

    if (drv->type == DRIVE_TYPE_AHCI) {
        return ahci_write_sector((HBA_PORT*)drv->ahci_port, lba, 0, 1, buffer);
    } else if (drv->type == DRIVE_TYPE_ATA) {
        ata_write_sector(lba, buffer);
        return 1;
    }
    return 0;
}

int ata_read_sector(uint32_t lba, uint8_t *buffer, uint8_t drive) {
    if(inb(ATA_STATUS) == 0xFF) return 0; 
    uint8_t drive_head = (drive == ATA_SLAVE) ? 0xF0 : 0xE0;
    outb(ATA_DRIVE_HEAD, drive_head | ((lba >> 24) & 0x0F));
    ata_wait_io();
    outb(ATA_ERROR, 0x00);  
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);
    if (!ata_wait_bsy()) return 0; 
    if(inb(ATA_STATUS) & ATA_SR_ERR) return 0;
    if (!ata_wait_drq()) return 0; 
    uint16_t* ptr = (uint16_t*)buffer;
    for(int i = 0; i < 256; i++) ptr[i] = inw(ATA_DATA);
    return 1;
}

void ata_write_sector(uint32_t lba, uint8_t *buffer){
    uint8_t drive_flag = 0xE0 | ((lba >> 24) & 0x0F);
    outb(ATA_DRIVE_HEAD, drive_flag);
    ata_wait_io();
    outb(ATA_ERROR, 0x00);
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);
    if (!ata_wait_bsy()) return;
    if (!ata_wait_drq()) return;
    uint16_t* ptr = (uint16_t*)buffer;
    for(int i = 0; i < 256; i++){
        outw(ATA_DATA, ptr[i]);
        __asm__ volatile("nop\nnop\nnop");
    }
    outb(ATA_COMMAND, 0xE7); 
    ata_wait_bsy();
}

void ata_identify(uint8_t drive, struct disk_struct *ds) {
    uint8_t drive_flag = (drive == ATA_SLAVE) ? 0xF0 : 0xE0;
    outb(ATA_DRIVE_HEAD, drive_flag);
    outb(ATA_SECTOR_CNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    
    uint8_t status = inb(ATA_STATUS);
    if(status == 0 || status == 0xFF) { strcpy(ds->name, "NO DISK"); return; }
    if (!ata_wait_bsy()) { strcpy(ds->name, "ERROR"); return; }
    if (inb(ATA_STATUS) & ATA_SR_ERR) { strcpy(ds->name, "ERROR/CD-ROM"); return; }
    if (!ata_wait_drq()) { strcpy(ds->name, "ERROR"); return; }
    for(int i = 0; i < 256; i++) ds->buffer[i] = inw(ATA_DATA);
    
    int name_idx = 0;
    for(int word_idx = 27; word_idx <= 46 && name_idx < 40; word_idx++) {
        uint16_t word = ds->buffer[word_idx];
        char high = (word >> 8) & 0xFF;
        char low  = word & 0xFF;
        if(name_idx < 39) ds->name[name_idx++] = high;
        if(name_idx < 39) ds->name[name_idx++] = low;
    }
    ds->name[name_idx] = '\0';
    
    int len = strlen(ds->name);
    while(len > 0 && ds->name[len-1] == ' ') ds->name[--len] = '\0';
    if(ds->name[0] == '\0') strcpy(ds->name, drive == ATA_MASTER ? "ATA Master" : "ATA Slave");
}