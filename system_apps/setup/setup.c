#include "system_apps/setup/setup.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "drivers/file/ATA/ATA.h"
#include "fs/fat/fat32.h"
#include "utils/utils.h"
#include "mm/memory.h"
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "system_apps/console/system.h"
#include "utils/sysconfig.h"
#include "task/task.h"

Window *setupWin;
Config *cfg = 0; 

void printInterface(void){
    setupWin = wm_create_window(VGA32_COLOR_BLUE);
    current_task->window = setupWin; 
    current_task->owns_window = 1; 
    set_current_output_window(setupWin);
    
    printf("\n\n\n");
    printf("     Welcome to setup 8086-OS!\n\n\n");
    
    printf("     --- Connected Disks ---\n\n");
    for (int i = 0; i < sys_drive_count; i++) {
        printf("     [%d] %s\n", i, sys_drives[i].name);
    }
    if (sys_drive_count == 0) {
        printf("     No disks detected!\n");
    }
    
    printf("\n\n     Please select the OS mode\n\n");
    printf("     1. Read-only mode (SAFE MODE, no disk write access)\n\n");
    printf("     2. Read/write mode (Try to load config)\n\n");
    printf("     (default 1)");
}

void diskStage(void){

    printInterface();
    vesa_render_buffer();

    char c = getch();

    if(c == '2') {
        isReadMode = 0;
    } else {
        isReadMode = 1;
    }

    if(isReadMode == 0){
        int file_size = fat32_get_file_size("kernel.cfg");

        if(file_size > 0){ 
            uint8_t *file_buffer = (uint8_t*)kmalloc(file_size + 512);
            
            if (!file_buffer) {
                printf("\n%CError: kmalloc failed! Not enough memory.%C", VGA32_COLOR_RED, VGA32_COLOR_WHITE);
                for(volatile int i=0; i<(50000000 * 20); i++); 
                goto exit_setup;
            }

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
            printf("\n%C%s", VGA32_COLOR_RED, "kernel.cfg not found or corrupted!");
            for(volatile int i=0; i<(50000000 * 20); i++); 
            goto exit_setup;
        }

        printf("\nLoading system config and wallpaper... This takes a few seconds.\n");
        sysconfig_reload();
    }

exit_setup:
    if (cfg) config_free(cfg);
    set_current_output_window(0);
    wm_close_window(setupWin);
}

static void main(int argc, char **argv){
    keyboard_flush();
    diskStage();
}

Setup setup = {
    .main = main
};