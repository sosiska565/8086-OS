#include "system.h"
#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "drivers/rtc/rtc.h"
#include "memory/memory.h"
#include "programs/system/memory_viewer/memory_viewer.h"
#include "programs/system/disk_viewer/disk_viewer.h"

//test var
void* ptr;

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
    {"colortest",     cmd_colortest,     "Show all colors"},
    {"banner",        cmd_banner,        "Show OS banner"},
    
    {"memview",       cmd_memview,       "Memory viewer"},
    {"kmalloc",       cmd_kmalloc,       "Allocate memory (kmalloc 256)"},
    {"kfree",         cmd_kfree,         "Free memory (kfree <addr>)"},
    {"heapdump",      cmd_heapdump,      "Dump heap state"},

    {"diskviewer",      cmd_disk_viewer,      "View disk"},
    
    {NULL, NULL, NULL}
};

void register_commands(void) {
    
}

void cmd_colortest(char **tokens) {
    print("\nVGA Color Table:\n");
    print("================\n");
    
    const char* color_names[] = {
        "Black", "Blue", "Green", "Cyan", 
        "Red", "Magenta", "Brown", "Light Grey",
        "Dark Grey", "Light Blue", "Light Green", "Light Cyan",
        "Light Red", "Light Magenta", "Yellow", "White"
    };
    
    for(int i = 0; i < 16; i++) {
        printnumber(i);
        print(": ");
        print_colored("████ ", VGA_COLOR(VGA_COLOR_BLACK, i));
        print(color_names[i]);
        print("\n");
    }
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
        if(num2 == 0) {
            print("Error: Division by zero!\n");
            return;
        }
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

void cmd_box(void) {
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

void cmd_help(char **tokens){
    int page = 1;
    if(tokens[1] != 0) {
        char* page_str = tokens[1];
        page = 0;
        while(*page_str >= '0' && *page_str <= '9') {
            page = page * 10 + (*page_str - '0');
            page_str++;
        }
    }

    int max_commands_in_page = 10;
    int max_pages = 0;
    int length = 0;

    for(; commands[length].name != NULL; length++);
    max_pages = (length / max_commands_in_page) + 1;

    if(page > max_pages || page < 1) {
        print("Error: Page must be between 1 and ");
        printnumber(max_pages);
        print("\n");
        return;
    }

    int start_index = (page - 1) * max_commands_in_page;
    int end_index = start_index + max_commands_in_page;

    print("\n=== Help (Page ");
    printnumber(page);
    print(" of ");
    printnumber(max_pages);
    print(") ===\n\n");

    for(int i = start_index; i < end_index && i < length; i++) {
        print(commands[i].name);
        print(" - ");
        print(commands[i].description);
        print("\n");
    }

    print("\nNavigation: ");
    if(page > 1) {
        print("help ");
        printnumber(page - 1);
        print(" - Previous page | ");
    }
    if(page < max_pages) {
        print("help ");
        printnumber(page + 1);
        print(" - Next page");
    }
    print("\n");
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

void cmd_disk_viewer(void){
    disk_viewer.main();
}