#include "programs/system/setup/setup.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "drivers/file/ATA/ATA.h"

struct disk_struct ds;
char master[256];
char slave[256];

static int main(void){
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

    clear_screen();

    return 0;
}

Setup setup = {
    .main = main
};