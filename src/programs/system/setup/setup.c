#include "programs/system/setup/setup.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "drivers/file/ATA/ATA.h"
#include "fs/fat/fat32.h"
#include "utils/utils.h"
#include "memory/memory.h"

struct disk_struct ds;
char master[256];
char slave[256];

Config *cfg;

static int main(void){
    //read cfg
    int file_size = fat32_get_file_size("kernel.cfg");

    if(file_size <= 0){
        panic("kernel.cfg not found!");
    }

    uint8_t *file_buffer = (uint8_t*)kmalloc(file_size + 512);

    for(int i=0; i<file_size; i++) file_buffer[i] = 0;

    fat32_read_file("kernel.cfg", file_buffer);

    cfg = config_parse((char *)file_buffer);

    char *isfirststart = config_get_value(cfg, "is_first_start");

    if(isfirststart){
        if(strcmp(isfirststart, "false") == 0){
            return 0;
        }
    } else {
        panic("kernel.cfg damaged");
    }

    //setup

    clear_screen();

    print_header(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "SETUP");
    print_header(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "Welcome to 8086-OS!");
    print_header(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "DISKS");
    ata_identify(ATA_MASTER, &ds);
    strcpy(master, ds.name);
    ata_identify(ATA_SLAVE, &ds);
    strcpy(slave, ds.name);
    print("\n");
    draw_text_box_ex((char*[]){
        master,
        slave,
        NULL
    }, "Disks list", 1, 1, 1, 1, VGA_COLOR_LIGHT_GREY, VGA_COLOR_LIGHT_GREY, VGA_COLOR_WHITE, 1);
    print("\n");
    draw_text_box_ex((char*[]){
        "1. read only (OS can`t write data into disks),",
        "2. read/write (OS can write data into disks),",
        "(default 1)",
        NULL
    }, "Please select a OS disk driver type", 1, 1, 1, 1, VGA_COLOR_LIGHT_GREY, VGA_COLOR_LIGHT_GREY, VGA_COLOR_WHITE, 1);
    isReadMode = 1;
    char c = getch();

    if(c == '1') isReadMode = 1;
    if(c == '2') isReadMode = 0;

    config_set_value(cfg, "is_first_start", "false");

    if (isReadMode) 
        config_set_value(cfg, "is_read_only_mode", "true");
    else 
        config_set_value(cfg, "is_read_only_mode", "false");

    if (isReadMode == 0) {
        print("\nSaving configuration...\n");
        config_save("kernel.cfg", cfg);
    } else {
        print("\nRead-only mode selected. Config NOT updated.\n");
        for(volatile int i=0; i<50000000; i++);
    }

    clear_screen();

    //update config
    
    config_free(cfg);
    kfree(file_buffer);

    return 0;
}

Setup setup = {
    .main = main
};