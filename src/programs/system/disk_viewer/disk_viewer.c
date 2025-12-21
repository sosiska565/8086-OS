#include "programs/system/disk_viewer/disk_viewer.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "drivers/file/ATA/ATA.h"
#include <stdint.h>

int main(void){
    clear_screen();
    disable_cursor();

    int n = 0;

    uint8_t buffer[512];
    for(int i = 0; i < 512; i++) buffer[i] = 0;

    while(1){
        set_cursor_position(0, 0);
        clear_screen();
        print("\n");
        print_header(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "DISK VIEWER V1.0");
        print("Sector: ");
        printnumber(n);
        print("\n");

        ata_read_sector(n, buffer);
        
        for(int i = 0; i < 512; i++){
            print_char((char)buffer[i]);
        }

        print_footer(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "[Q] quit, [W] up, [S] down, [A] up 100, [D] down 100");

        char c = getch();

        if(c == 'q' || c == 'Q') {
            clear_screen();
            return 0;
        }
        if(c == 'w' || c == 'W') n--;
        if(c == 's' || c == 'S') n++;
        if(c == 'a' || c == 'A') n -= 100;
        if(c == 'd' || c == 'D') n += 100;
        if(n < 0) n = 0;
    }
}
 
Disk_viewer disk_viewer = {
    .main = main
};