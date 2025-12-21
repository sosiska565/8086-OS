#include "programs/system/setup/setup.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"

static int main(void){
    clear_screen();

    print_header(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "SETUP");
    printn("Welcome to 8086-OS!\n");
    printn("Step 1\n");
    draw_text_box_ex((char*[]){
        "1. only read,",
        "2. read/write,",
        "(default 1)",
        NULL
    }, "Please select a OS type: ", 1, 1, 1, 0, VGA_COLOR_LIGHT_GREY, VGA_COLOR_LIGHT_GREY, VGA_COLOR_WHITE);
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