#include "programs/system/setup/setup.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "drivers/file/ATA/ATA.h"
#include "fs/fat/fat32.h"
#include "utils/utils.h"
#include "memory/memory.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"

struct disk_struct ds;
char master[256];
char slave[256];

Config *cfg;

void diskStage(void){
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
            return;
        }
    } else {
        panic("kernel.cfg damaged");
    }

    //setup

    clear_screen();
    
    ata_identify(ATA_MASTER, &ds);
    strcpy(master, ds.name);
    ata_identify(ATA_SLAVE, &ds);
    strcpy(slave, ds.name);
    
    Window setupWin;
    draw_window(
        &setupWin, 0,
        0, 500, 300,
        VGA32_COLOR_CYAN, VGA32_COLOR_DARK_GREY,
        1
    );
    set_current_output_window(&setupWin);
    printf("\n");
    printf("\n");
    printf("\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen("Welcome to setup 8086-OS!")) / 2; i++){
        printf(" ");
    }
    printf("Welcome to setup 8086-OS!");
    printf("\n");
    printf("\n");
    printf("\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen("---Disks list---")) / 2; i++){
        printf(" ");
    }
    printf("---Disks list---");
    printf("\n");
    printf("\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen(master)) / 2; i++){
        printf(" ");
    }
    printf(master);
    printf("\n");
    printf("\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen(slave)) / 2; i++){
        printf(" ");
    }
    printf(slave);
    printf("\n");
    printf("\n");
    printf("\n");
    printf("\n");
    printf("\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen("Please select the OS mode")) / 2; i++){
        printf(" ");
    }
    printf("Please select the OS mode\n\n\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen("1. read-only mode(OS is not allowed to write to the disk)")) / 2; i++){
        printf(" ");
    }
    printf("1. read-only mode(OS is not allowed to write to the disk)\n\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen("2. read/write mode(OS has permission to write to the disk)")) / 2; i++){
        printf(" ");
    }
    printf("2. read/write mode(OS has permission to write to the disk)\n\n");
    for(int i = 0; i < ((setupWin.width / 8) - strlen("(default 1)")) / 2; i++){
        printf(" ");
    }
    printf("(default 1)");

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
        config_save("kernel.cfg", cfg);
    } else {
        for(volatile int i=0; i<50000000; i++);
    }

    clear_screen();

    //update config
    
    set_current_output_window(0);
    config_free(cfg);
    kfree(file_buffer);
}

static int main(void){
    diskStage();
    return 0;
}

Setup setup = {
    .main = main
};