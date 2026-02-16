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
char master[256] = "Not detected"; 
char slave[256] = "Not detected";

Window *setupWin;

Config *cfg = 0; 

void printInterface(void){
    ata_identify(ATA_MASTER, &ds);
    if (ds.name[0] != 0) strcpy(master, ds.name);
    
    ata_identify(ATA_SLAVE, &ds);
    if (ds.name[0] != 0) strcpy(slave, ds.name);

    setupWin = wm_create_window(VGA32_COLOR_BLUE);
    set_current_output_window(setupWin);
    
    printf("\n\n\n");
    printf("     Welcome to setup 8086-OS!\n\n\n");
    printf("     ---Disks list---\n\n");
    printf("     Master: %s\n", master);
    printf("     Slave:  %s\n\n\n", slave);
    printf("     Please select the OS mode\n\n");
    printf("     1. Read-only mode (SAFE MODE, no disk access)\n\n");
    printf("     2. Read/write mode (Try to load config)\n\n");
    printf("     (default 1)");
}

void diskStage(void){

    printInterface();

    char c = getch();

    if(c == '2') {
        isReadMode = 0;
    } else {
        isReadMode = 1;
    }

    if(isReadMode == 0){
        printf("\nMounting FAT32...");
        
        int file_size = fat32_get_file_size("kernel.cfg");
        printf("\nkernel file size: %d", file_size);

        if(file_size > 0){
            uint8_t *file_buffer = (uint8_t*)kmalloc(file_size + 512);
            for(int i=0; i<file_size; i++) file_buffer[i] = 0;

            fat32_read_file("kernel.cfg", file_buffer);
            cfg = config_parse((char *)file_buffer);

            char *isfirststart = config_get_value(cfg, "is_first_start");
            if(isfirststart){
                if(strcmp(isfirststart, "false") == 0){
                     kfree(file_buffer);
                     goto exit_setup;
                }
            }
            
            config_set_value(cfg, "is_first_start", "false");
            config_set_value(cfg, "is_read_only_mode", "false");
            config_save("kernel.cfg", cfg);
            
            kfree(file_buffer);
        } else {
            printf("\n%C%s", VGA32_COLOR_RED, "kernel.cfg not found!");
            for(volatile int i=0; i<(50000000 * 20); i++); 
        }
    } else {
        printf("\nSkipping disk access (Read-Only mode selected).");
    }

exit_setup:
    clear_screen();
    if (cfg) config_free(cfg);
    wm_close_window(setupWin);
    set_current_output_window(0);
}

static int main(void){
    clear_screen();
    keyboard_flush();
    diskStage();
    return 0;
}

Setup setup = {
    .main = main
};