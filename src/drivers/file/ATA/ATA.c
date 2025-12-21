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

void ata_read_sector(uint32_t lba, uint8_t *buffer){
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_ERROR, 0x00);
    outb(ATA_SECTOR_CNT, 1);

    outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    ata_wait_bsy();
    ata_wait_drq();

    uint16_t* ptr = (uint16_t*)buffer;
    for(int i = 0; i < 256; i++){
        ptr[i] = inw(ATA_DATA);
    }
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

void ata_identify(uint8_t drive){
    uint8_t drive_flag = (drive == ATA_SLAVE) ? 0xF0 : 0xE0;
    outb(ATA_DRIVE_HEAD, drive_flag);
    
    outb(ATA_SECTOR_CNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    
    uint8_t status = inb(ATA_STATUS);
    if(status == 0) return;
    
    while(inb(ATA_STATUS) & ATA_SR_BSY);
    
    if (inb(ATA_STATUS) & ATA_SR_ERR) {
        print("Error identifying drive (might be CD-ROM)\n");
        return;
    }
    
    while(!(inb(ATA_STATUS) & ATA_SR_DRQ));
    
    uint16_t buffer[256];
    for(int i=0; i<256; i++) {
        buffer[i] = inw(ATA_DATA);
    }
    
    print("Disk Model: ");
    for(int i = 27; i < 47; i++) {
        uint16_t w = buffer[i];
        char c1 = (w >> 8) & 0xFF;
        char c2 = w & 0xFF;
        print_char(c1);
        print_char(c2);
    }
    print("\n");
}