#include "system.h"
#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "drivers/rtc/rtc.h"
#include "memory/memory.h"
#include "programs/system/memory_viewer/memory_viewer.h"
#include "programs/system/disk_viewer/disk_viewer.h"
#include "fs/fat/fat32.h"
#include <stdint.h>
#include "drivers/keyboard/keyboardDriver.h"
#include "utils/utils.h"
#include "multiboot.h"
#include "utils/utils.h"
#include "global.h"
#include "drivers/video/bga/bga.h"

#define PROGRAM_LOAD_ADDRES 0x300000

//test var
void* ptr;

typedef void (*program_entry_t)(int argc, char **argv);

command_t commands[] = {
    {"help",          cmd_help,          "Show this help message"},
    {"clear",         cmd_clear,         "Clear screen"},
    {"exit",          cmd_exit,          "Exit console"},
    
    {"sysinfo",       cmd_sysinfo,       "System information"},
    {"time",          cmd_time,          "Show current time"},
    
    {"echo",          cmd_echo,          "Print text"},
    {"calc",          cmd_calc,          "Calculator (calc 10 + 5)"},
    {"ascii",         cmd_ascii,         "Show ASCII table"},
    
    {"setbgcolor",    cmd_setbgcolor,    "Set background color (0-15)"},
    {"settextcolor",  cmd_settextcolor,  "Set text color (0-15)"},
    {"fetch",     cmd_colortest,     "Show OS info"},
    {"banner",        cmd_banner,        "Show OS banner"},
    
    {"memview",       cmd_memview,       "Memory viewer"},
    {"kmalloc",       cmd_kmalloc,       "Allocate memory (kmalloc 256)"},
    {"kfree",         cmd_kfree,         "Free memory (kfree <addr>)"},
    {"heapdump",      cmd_heapdump,      "Dump heap state"},

    {"diskviewer",      cmd_disk_viewer,      "View disk"},
    {"ls",      cmd_ls,      "Show all files"},
    {"cat",      cmd_cat,      "Show file (cat <file>)"},
    {"exec",      cmd_exec,      "Executable file (exec <file>)"},
    {"mkfile",      cmd_mkfile,      "Make file (mkfile <file name> <text>)"},
    {"rm",      cmd_rm,      "Remove file (rm <file name>)"},
    {"readsystemcfg", cmd_readsystemcfg, "Update system configs"},
    
    {NULL, NULL, NULL}
};

void register_commands(void) {
    
}

void cmd_colortest(char **tokens) {
    unsigned long long total_mem_kb = (unsigned long long)mbi->mem_lower + mbi->mem_upper;
    char processor[13];
    get_cpu_vendor(processor);
    printf("            .-\"\"\"-.");
    printf("     OS:");
    printf("8086-OS V0.5");
    printf("\n           '       \\");
    printf("     CPU: ");
    printf("%s", processor);
    printf("\n          |,.  ,-.  |");
    printf("     RAM: %d MB", (int)(total_mem_kb / 1024));
    printf("\n          |()L( ()| |");
    printf("     ");
    for(int i = 0; i < 8; i++) {
        printf("%C%c", VGA_COLOR(i, i), 0xDB);
    }
    printf("\n          |,'  `\".| |");
    printf("     ");
    for(int i = 8; i < 16; i++) {
        printf("%C%c", VGA_COLOR(i, i), 0xDB);
    }
    printf("\n          |.___.',| `");
    printf("\n         .j `--\"' `  `.");
    printf("\n        / '        '   \\");
    printf("\n       / /          `   `.");
    printf("\n      / /            `    .");
    printf("\n     / /              l   |");
    printf("\n    . ,               |   |");
    printf("\n    ,\"`.             .|   |");
    printf("\n _.'   ``.          | `..-'l");
    printf("\n|       `.`,        |      `.");
    printf("\n|         `.    __.j         )");
    printf("\n|__        |--\"\"___|      ,-'");
    printf("\n   `\"--...,+\"\"\"\"   `._,.-'");
    
    print("\n");
}

void cmd_banner(char **tokens) {
    print("\n");
    print("========================================\n");
    print("     8086-OS Operating System          \n");
    print("     Version 1.0.0                     \n");
    print("     Type 'help' for commands          \n");
    print("========================================\n");
    print("\n");
}

void cmd_sysinfo(char **tokens) {
    print("\nSystem Information:\n");
    print("==================\n");
    print("OS Name: 8086-OS\n");
    print("Version: 1.0.0\n");
    print("Architecture: x86 (16-bit)\n");
    print("Video Mode: VGA Text Mode 80x25\n");
    print("Available Commands: ");
    int count = 0;
    for(int i = 0; commands[i].name != NULL; i++) count++;
    printnumber(count);
    print("\n\n");
}

void cmd_echo(char **tokens) {
    if (tokens[1] == 0) {
        print("\n");
        return;
    }
    
    int i = 1;
    while(tokens[i] != 0) {
        print(tokens[i]);
        if(tokens[i+1] != 0) {
            print(" ");
        }
        i++;
    }
    print("\n");
}

void cmd_calc(char **tokens) {
    if (!tokens[1] || !tokens[2] || !tokens[3]) {
        print("Usage: calc <num1> <op> <num2>\n");
        print("Operations: + - * /\n");
        return;
    }
    
    int num1 = 0, num2 = 0;
    char *s1 = tokens[1];
    char *s2 = tokens[3];
    
    while(*s1 >= '0' && *s1 <= '9') {
        num1 = num1 * 10 + (*s1 - '0');
        s1++;
    }
    
    while(*s2 >= '0' && *s2 <= '9') {
        num2 = num2 * 10 + (*s2 - '0');
        s2++;
    }
    
    char op = tokens[2][0];
    int result = 0;
    
    if(op == '+') {
        result = num1 + num2;
    } else if(op == '-') {
        result = num1 - num2;
    } else if(op == '*') {
        result = num1 * num2;
    } else if(op == '/') {
        result = num1 / num2;
    } else {
        print("Unknown operation! Use + - * /\n");
        return;
    }
    
    print("Result: ");
    printnumber(result);
    print("\n");
}

void cmd_time(char **tokens) {
    print("\nCurrent Time:\n");
    print("============\n");
    print("Time: ");
    printnumber(rtc_get_time().hour);
    print(":");
    printnumber(rtc_get_time().minute);
    print(":");
    printnumber(rtc_get_time().second);
    print("\nDate: ");
    printnumber(rtc_get_time().day);
    print("/");
    printnumber(rtc_get_time().month);
    print("/");
    printnumber(rtc_get_time().year);
    print("\n");
}

void cmd_ascii(char **tokens) {
    print("\nASCII Table (printable):\n");
    print("========================\n");
    
    for(int i = 32; i < 127; i++) {
        printnumber(i);
        print(": ");
        print_char((char)i);
        print("  ");
        
        if((i - 31) % 8 == 0) {
            print("\n");
        }
    }
    print("\n");
}

void cmd_box(char **tokens) {
    print("\n");
    print("+-------------------------------+\n");
    print("|   Welcome to 8086-OS Console! |\n");
    print("|   Type 'help' for commands!   |\n");
    print("+-------------------------------+\n");
    print("\n");
}

void cmd_settextcolor(char **tokens) {
    if (tokens[1] != 0) {
        char* color_str = tokens[1];
        int color = 0;
        
        while (*color_str >= '0' && *color_str <= '9') {
            color = color * 10 + (*color_str - '0');
            color_str++;
        }
        
        if (color >= 0 && color <= 15) {
            set_text_color(color);
            print("Text color set to ");
            printnumber(color);
            print("\n");
        } else {
            print("Invalid color! Use 0-15\n");
        }
    } else {
        print("Usage: settextcolor <0-15>\n");
    }
}

void str_copy_to_buffer(char* dest, char* src, uint16_t* offset) {
    while (*src != '\0') {
        dest[*offset] = *src;
        (*offset)++;
        src++;
    }
    dest[*offset] = '\0';
}

void int_to_buffer(char* dest, int n, uint16_t* offset) {
    if (n == 0) {
        dest[(*offset)++] = '0';
        dest[*offset] = '\0';
        return;
    }
    
    char temp[10];
    int i = 0;
    while (n > 0) {
        temp[i++] = (n % 10) + '0';
        n /= 10;
    }
    
    while (i > 0) {
        dest[(*offset)++] = temp[--i];
    }
    dest[*offset] = '\0';
}

void cmd_help(char **tokens) {
    clear_screen();
    int page = 1;
    if (tokens[1] != 0) {
        char* page_str = tokens[1];
        page = 0;
        while (*page_str >= '0' && *page_str <= '9') {
            page = page * 10 + (*page_str - '0');
            page_str++;
        }
        if (page == 0) page = 1;
    }

    int max_commands_in_page = 10;
    int length = 0;
    while(commands[length].name != NULL) length++;
    
    int max_pages = (length + max_commands_in_page - 1) / max_commands_in_page;
    if (max_pages == 0) max_pages = 1;

    if (page > max_pages || page < 1) {
        print("Error: Page must be between 1 and ");
        printnumber(max_pages);
        print("\n");
        return;
    }

    static char line_buffers[14][80]; 
    char* lines_ptr[15];

    int start_index = (page - 1) * max_commands_in_page;
    int end_index = start_index + max_commands_in_page;
    int current_line = 0;

    for (int i = start_index; i < end_index && i < length; i++) {
        uint16_t offset = 0;
        
        str_copy_to_buffer(line_buffers[current_line], commands[i].name, &offset);
        str_copy_to_buffer(line_buffers[current_line], " - ", &offset);
        str_copy_to_buffer(line_buffers[current_line], commands[i].description, &offset);
        
        lines_ptr[current_line] = line_buffers[current_line];
        current_line++;
    }

    if (max_pages > 1) {
        line_buffers[current_line][0] = '\0'; 
        lines_ptr[current_line] = line_buffers[current_line];
        current_line++;

        uint16_t offset = 0;
        str_copy_to_buffer(line_buffers[current_line], "Nav: ", &offset);
        
        if (page > 1) {
            str_copy_to_buffer(line_buffers[current_line], "prev: help ", &offset);
            int_to_buffer(line_buffers[current_line], page - 1, &offset);
            if (page < max_pages) {
                str_copy_to_buffer(line_buffers[current_line], " | ", &offset);
            }
        }
        
        if (page < max_pages) {
            str_copy_to_buffer(line_buffers[current_line], "next: help ", &offset);
            int_to_buffer(line_buffers[current_line], page + 1, &offset);
        }
        
        lines_ptr[current_line] = line_buffers[current_line];
        current_line++;
    }

    lines_ptr[current_line] = NULL;

    char title_buffer[40];
    uint16_t title_offset = 0;
    str_copy_to_buffer(title_buffer, "Help (Page ", &title_offset);
    int_to_buffer(title_buffer, page, &title_offset);
    str_copy_to_buffer(title_buffer, " of ", &title_offset);
    int_to_buffer(title_buffer, max_pages, &title_offset);
    str_copy_to_buffer(title_buffer, ")", &title_offset);
    
    draw_text_box_ex(lines_ptr, title_buffer, 
                     1, 1, 2, 2,
                     0x07, 0x0F, 0x0E,
                     0);
}

void cmd_setbgcolor(char** tokens){
    if (tokens[1] != 0) {
        char* color_str = tokens[1];
        int color = 0;
        
        while (*color_str >= '0' && *color_str <= '9') {
            color = color * 10 + (*color_str - '0');
            color_str++;
        }
        
        if (color >= 0 && color <= 15) {
            set_background_color(color);
            print("Background color set to ");
            printnumber(color);
            print("\n");
        } else {
            print("Invalid color! Use 0-15\n");
        }
    } else {
        print("Usage: setbgcolor <0-15>\n");
    }
}

void cmd_clear(char **tokens) {
    clear_screen();
}

void cmd_exit(char **tokens) {
    console.should_exit = 1;
    print("Exiting console...\n");
}

void cmd_memview(char **tokens) {
    memoryViewer.main();
}

void cmd_kmalloc(char **tokens) {
    if (!tokens[1]) {
        print("Usage: kmalloc <size>\n");
        return;
    }
    
    int size = strtn(tokens[1]);
    
    ptr = kmalloc(size);
}

void cmd_kfree(char **tokens) {
    kfree(ptr);
}

void cmd_heapdump(char **tokens) {
    heap_dump();
}

void cmd_disk_viewer(char **tokens){
    disk_viewer.main();
}

void cmd_ls(char **tokens){
    fat32_ls();
}

void cmd_cat(char **tokens){
    if(!tokens[1]) {
        print("Usage: cat <filename>\n");
        return;
    }

    int file_size = fat32_get_file_size(tokens[1]);

    if(file_size <= 0){
        print("File not found or empty.\n");
        return;
    }

    uint8_t *file_buffer = (uint8_t*)kmalloc(file_size + 512);

    for(int i=0; i<file_size; i++) file_buffer[i] = 0;
    
    fat32_read_file(tokens[1], file_buffer);

    for(int i = 0; i < file_size; i++) {
        char c = (char)file_buffer[i];
        print_char(c);
    }
    print("\n");

    kfree(file_buffer);
}

int is_executable(char* filename) {
    int len = 0;
    while(filename[len]) len++;
    
    if (len < 4) return 0;
    
    char* ext = filename + len - 4;
    
    if (ext[0] != '.') return 0;
    if (ext[1] != 'B' && ext[1] != 'b') return 0;
    if (ext[2] != 'I' && ext[2] != 'i') return 0;
    if (ext[3] != 'N' && ext[3] != 'n') return 0;
    
    return 1;
}

void cmd_exec(char **tokens){
    keyboard_flush();
    if(!tokens[1]){
        print("Usage: exec <filename>\n");
        return;
    }

    if (!is_executable(tokens[1])) {
        print("Error: File is not executable (must be .bin)\n");
        return;
    }

    uint8_t* load_addr = (uint8_t*)PROGRAM_LOAD_ADDRES;
    
    for(int i = 0; i < 65536; i++) load_addr[i] = 0;

    int bytes_read = fat32_read_file(tokens[1], load_addr);

    if(bytes_read > 0){
        program_entry_t program = (program_entry_t)PROGRAM_LOAD_ADDRES;
        int argc = 0;
        while (tokens[1 + argc] != 0) {
            argc++;
        }

        program(argc, &tokens[1]);

        keyboard_flush();
    } else {
        print("File not found!\n");
    }
}

void cmd_mkfile(char **tokens) {
    if (!tokens[1]) {
        print("Usage: mkfile <name> <text>\n");
        return;
    }
    
    char* filename = tokens[1];
    
    if (fat32_write_file(filename, "", 0) == 1) {
        print("File created successfully!\n");
    } else {
        print("Error creating file.\n");
    }
}

void cmd_rm(char **tokens){
    if(fat32_delete_file(tokens[1]) != 1) print("File not removed\n");
}

void cmd_readsystemcfg(char **tokens) {
    int file_size = fat32_get_file_size("kernel.cfg");

    if(file_size <= 0){
        panic("kernel.cfg not found!");
        return;
    }

    uint8_t *file_buffer = (uint8_t*)kmalloc(file_size + 512);

    for(int i=0; i<file_size; i++) file_buffer[i] = 0;

    fat32_read_file("kernel.cfg", file_buffer);

    Config *cfg = config_parse(file_buffer);

    if(strcmp(config_get_value(cfg, "is_read_only_mode"), "true") == 0){
        isReadMode = 1;
    } else {
        isReadMode = 0;
    }

    if(strcmp(config_get_value(cfg, "graphic_mode"), "true") == 0){
        set_video_mode(VIDEO_MODE_GRAPHICS);
        clear_screen_bga(0x00000000);
        graphic_mode = 2;
    } else {
        set_video_mode(VIDEO_MODE_TEXT);
        graphic_mode = 1;
    }

    print("read mode: ");
    printnumber(isReadMode);
    print("\n");

    config_free(cfg);
    kfree(file_buffer);
}