#include "drivers/file/ATA/ATA.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "global.h"

void ata_wait_bsy(void){
    while(inb(ATA_STATUS) & ATA_SR_BSY);
}

void ata_wait_drq(void){
    while(!(inb(ATA_STATUS) & (ATA_SR_DRQ | ATA_SR_ERR)));
}

void ata_wait_io() {
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
    inb(ATA_STATUS);
}

int ata_read_sector(uint32_t lba, uint8_t *buffer, uint8_t drive) {
    if(inb(ATA_STATUS) == 0xFF) {
        return 0; 
    }

    uint8_t drive_head = 0xE0;
    if (drive == ATA_SLAVE) {
        drive_head = 0xF0;
    }

    outb(ATA_DRIVE_HEAD, drive_head | ((lba >> 24) & 0x0F));
    
    ata_wait_io();

    outb(ATA_ERROR, 0x00);  
    outb(ATA_SECTOR_CNT, 1);
    
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    while(inb(ATA_STATUS) & ATA_SR_BSY);
    
    uint8_t status = inb(ATA_STATUS);
    if(status & ATA_SR_ERR) {
        print("ATA Error!\n");
        return 0;
    }

    while(!(inb(ATA_STATUS) & ATA_SR_DRQ));

    uint16_t* ptr = (uint16_t*)buffer;
    for(int i = 0; i < 256; i++){
        ptr[i] = inw(ATA_DATA);
    }
    
    return 1;
}

void ata_write_sector(uint32_t lba, uint8_t *buffer){
    if(isReadMode == 1){
        print_info("ERROR", "System is booted in read-only mode", VGA_COLOR_RED, VGA_COLOR_LIGHT_GREY);
        return;
    }
    ata_wait_bsy();

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_ERROR, 0x00);
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);

    ata_wait_bsy();

    uint16_t* ptr = (uint16_t*)buffer;
    for(int i = 0; i < 256; i++){
        outw(ATA_DATA, ptr[i]);

        __asm__ volatile("nop");
        __asm__ volatile("nop");
        __asm__ volatile("nop");
    }

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
    if(status == 0) {
        memset(ds->name, 0, 256);
        strcpy(ds->name, "NO DISK");
        return;
    }
    
    while(inb(ATA_STATUS) & ATA_SR_BSY);
    
    if (inb(ATA_STATUS) & ATA_SR_ERR) {
        memset(ds->name, 0, 256);
        strcpy(ds->name, "ERROR/CD-ROM");
        return;
    }
    
    while(!(inb(ATA_STATUS) & ATA_SR_DRQ));
    
    for(int i = 0; i < 256; i++) {
        ds->buffer[i] = inw(ATA_DATA);
    }
    
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
    while(len > 0 && ds->name[len-1] == ' ') {
        ds->name[--len] = '\0';
    }
    
    if(ds->name[0] == '\0') {
        strcpy(ds->name, drive == ATA_MASTER ? "ATA Master" : "ATA Slave");
    }
}