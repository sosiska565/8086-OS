#include "programs/system/memory_viewer/memory_viewer.h"

int memory_viewer_program(void){
    disable_cursor();

    uint32_t address = 0x100000;
    int bytes_to_show = 20;
    int bytes_per_line = 17;

    int len_name = 0;
    int len_bottominfo = 0;

    char *name = "MEMORY VIEWER 1.0";
    for(; name[len_name] != '\0'; len_name++);
    int padding_size_name = (80 - len_name) / 2;

    char *bottominfo = "[W] up, [S] down, [A] multy up, [D] multy down, [C] change bytes per line, [Q] quit";
    for(; bottominfo[len_bottominfo] != '\0'; len_bottominfo++);
    int padding_size_bottominfo = (80 - len_bottominfo);
    int free_space = 25 - 16;

    while(1){
        clear_screen();

        set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE);
        for(int i = 0; i < padding_size_name; i++){
            print_char(' ');
        }
        print(name);
        for(int i = 0; i <= padding_size_name; i++){
            print_char(' ');
        }
        set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

        

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

        set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE);
        print(bottominfo);
        for(int i = 0; i < padding_size_bottominfo; i++){
            print_char(' ');
        }
        set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

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