#include "programs/system/memory_viewer/memory_viewer.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"

static int main(void){
    disable_cursor();

    uint32_t address = 0x100000;
    int bytes_to_show = 20;
    int bytes_per_line = 17;

    char *bottominfo = "[W] up, [S] down, [A] multy up, [D] multy down, [C] change bytes per line, [Q] quit";

    while(1){
        clear_screen();

        print_header(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, "MEMORY VIEWER V1.0 ");

        for(int line = 0; line < 22; line++) {
            uint32_t current_addr = address + line * bytes_per_line;
            
            set_text_color(VGA_COLOR_DARK_GREY);
            char hex_chars[] = "0123456789ABCDEF";
            for(int j = 3; j >= 0; j--) {
                print_char(hex_chars[(current_addr >> (j*4)) & 0x0F]);
            }
            print(": ");
            
            set_text_color(VGA_COLOR_LIGHT_GREEN);
            for(int i = 0; i < bytes_per_line; i++) {
                uint8_t value = *((uint8_t*)(current_addr + i));
                print_char(hex_chars[(value >> 4) & 0x0F]);
                print_char(hex_chars[value & 0x0F]);
                print(" ");
            }

            set_text_color(VGA_COLOR_LIGHT_BLUE);
            print(" | ");
            for(int i = 0; i < bytes_per_line; i++) {
                uint8_t value = *((uint8_t*)(current_addr + i));
                set_text_color(VGA_COLOR_LIGHT_MAGENTA);
                print_char(value >= 32 && value <= 126 ? value : '.');
            }
            print("\n");
        }

        print_footer(VGA_COLOR_BLUE, VGA_COLOR_YELLOW, bottominfo);

        char c = getch();

        if(c == 'W' || c == 'w') address--;
        if(c == 'S' || c == 's') address++;
        if(c == 'A' || c == 'a') address -= bytes_per_line;
        if(c == 'D' || c == 'd') address += bytes_per_line;
        if(c == 'C' || c == 'c') {
            char buffer[10];
            int i = 0;
            
            enable_cursor();
            print("Bytes per line: ");
            gets(buffer, sizeof(buffer));
            
            int n = 0;
            while(buffer[i] >= '0' && buffer[i] <= '9') {
                n = n * 10 + (buffer[i] - '0');
                i++;
            }
            
            bytes_per_line = n;
            disable_cursor();
        }
        if(c == 'Q' || c == 'q') {
            clear_screen();
            return 0;
        };
    }

    return 0;
}

memory_viewer_t memoryViewer = {
    .main = main
};