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
#include "drivers/video/graphics.h"
#include "drivers/video/vesa.h"
#include "multitask/task.h"
#include "drivers/file/ATA/ATA.h"

#define PROGRAM_LOAD_ADDRES 0x40000000


void* ptr;

typedef void (*program_entry_t)(int argc, char **argv);

command_t commands[] = {
    {"help",          cmd_help,          "Show this help message"},
    {"clear",         cmd_clear,         "Clear screen"},
    {"exit",          cmd_exit,          "Exit console"},
    
    {"time",          cmd_time,          "Show current time"},
    
    {"echo",          cmd_echo,          "Print text"},
    {"calc",          cmd_calc,          "Calculator (calc 10 + 5)"},
    {"ascii",         cmd_ascii,         "Show ASCII table"},
    
    {"setbgcolor",    cmd_setbgcolor,    "Set background color (0-15)"},
    {"settextcolor",  cmd_settextcolor,  "Set text color (0-15)"},
    {"fetch",     cmd_colortest,     "Show OS info"},
    
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
    {"kcfgup", cmd_readsystemcfg, "Update system configs"},
    {"tasklist", cmd_tasklist, "Show task list"},
    {"kill", cmd_kill, "Kill process"},
    {"writemode", cmd_writemode, "Enable Read/Write disk mode"},
    {"disks", cmd_disks, "List all connected drives"},
    {"use", cmd_use, "Switch active drive (use <ID>)"},
    
    {NULL, NULL, NULL}
};

void register_commands(void) {
    
}

void cmd_colortest(char **tokens) {
    unsigned long long total_mem_kb = (unsigned long long)mbi->mem_lower + mbi->mem_upper;
    char processor[13];
    get_cpu_vendor(processor);
    
    printf("                                               .+.-*          ");
    printf("OS:");
    printf("8086-OS V0.5");
    printf("\n    +-                                       .#     =-        ");
    printf("CPU: ");
    printf("%s", processor);
    printf("\n.   +   .*-                                 .#        .$      ");
    printf("RAM: %d MB", (int)(total_mem_kb / 1024));
    printf("\n  +:       =:                             #-           #.     ");
    printf("\n  -          .*    -===+=+-.  =:=       -*              #     ");
    printf("\n :.         .#-:             :*- .=    *:               $.    ");
    printf("\n *             #               .   :# $                 -:    ");
    printf("\n +              .#.                  +.                  -    ");
    printf("\n.-            :=                                         -    ");
    printf("\n.-          +=                                           =    ");
    printf("\n.:        =:                                            --    ");
    printf("\n.#       -++=:.:                                        *.    ");
    printf("\n $                                                      *     ");
    printf("\n -                                                      -     ");
    printf("\n  =                 .+@+           .@@@@@@=.           *      ");
    printf("\n  --            =#$+@@@@              @@@@$.=$:       :.      ");
    printf("\n   =.       -#$=. : @@@@           :$@@@@@@  :*      +        ");
    printf("\n    -:      $     @@@@@@           .@@@@@@$  :*     *         ");
    printf("\n  .$-::.    +=    +@@@@:             #@@@+   *         .==.   ");
    printf("\n    ==       :     .++$$*-                     :   --         ");
    printf("\n       *#   -+.                             .  +.   *         ");
    printf("\n       .-    --                                     ::        ");
    printf("\n       =                  +=+                    ++==:        ");
    printf("\n      -:-++#                                .+:*..+           ");
    printf("\n          $  .#$.                         :=                  ");
    printf("\n          .       ==--:.              +*$=*-                  ");
    printf("\n                   #==#@@            *$*+==#.                 ");
    printf("\n                  :+==+*$:.     .##++======*:                 ");
    printf("\n                  :+=======================#:                 ");
    printf("\n           =+++==-:#+==================+++: :-                ");
    printf("\n             =       -+*##*+===+***#*=:      =                ");
    printf("\n               @.            +-:::+           @               ");
    printf("\n             +              .-:::::=          ..              ");
    printf("\n               :*            +:::::-           +              ");
    printf("\n                -       -.    =*+*:             -             ");
    printf("\n               +.       *              =        -             ");
    printf("\n               #        $              =.       -:            ");
    for(int i = 0; i < 8; i++) {
        printf("%C%c%c", VGA_COLOR(i, 0), 0x0001, 0x0001);
    }
    printf("\n               .        +              :.        :            ");
    for(int i = 8; i < 16; i++) {
        printf("%C%c%c", VGA_COLOR(i, 0), 0x0001, 0x0001);
    }
    
    printf("\n");
}

void cmd_echo(char **tokens) {
    if (tokens[1] == 0) {
        printf("\n");
        return;
    }
    
    int i = 1;
    while(tokens[i] != 0) {
        printf(tokens[i]);
        if(tokens[i+1] != 0) {
            printf(" ");
        }
        i++;
    }
    printf("\n");
}

void cmd_calc(char **tokens) {
    if (!tokens[1] || !tokens[2] || !tokens[3]) {
        printf("Usage: calc <num1> <op> <num2>\n");
        printf("Operations: + - * /\n");
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
        printf("Unknown operation! Use + - * /\n");
        return;
    }
    
    printf("Result: ");
    printnumber(result);
    printf("\n");
}

void cmd_time(char **tokens) {
    printf("\nCurrent Time:\n");
    printf("============\n");
    printf("Time: ");
    printf("%d", rtc_get_time().hour);
    printf(":");
    printf("%d", rtc_get_time().minute);
    printf(":");
    printf("%d", rtc_get_time().second);
    printf("\nDate: ");
    printf("%d", rtc_get_time().day);
    printf("/");
    printf("%d", rtc_get_time().month);
    printf("/");
    printf("%d", rtc_get_time().year);
    printf("\n");
}

void cmd_ascii(char **tokens) {
    printf("\nASCII Table (printable):\n");
    printf("========================\n");
    
    for(int i = 32; i < 127; i++) {
        printf("%d", i);
        printf(": ");
        printf("%c", (char)i);
        printf("  ");
        
        if((i - 31) % 8 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void cmd_box(char **tokens) {
    printf("\n");
    printf("+-------------------------------+\n");
    printf("|   Welcome to 8086-OS Console! |\n");
    printf("|   Type 'help' for commands!   |\n");
    printf("+-------------------------------+\n");
    printf("\n");
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
            printf("Text color set to ");
            printf("%d", color);
            printf("\n");
        } else {
            printf("Invalid color! Use 0-15\n");
        }
    } else {
        printf("Usage: settextcolor <0-15>\n");
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
    int page = 1;
    
    if (tokens[1] != NULL) {
        char* page_str = tokens[1];
        page = 0;
        while (*page_str >= '0' && *page_str <= '9') {
            page = page * 10 + (*page_str - '0');
            page_str++;
        }
        if (page == 0) page = 1;
    }

    int length = 0;
    while(commands[length].name != NULL) length++;
    
    int max_commands_in_page = 10;
    int max_pages = (length + max_commands_in_page - 1) / max_commands_in_page;
    if (max_pages == 0) max_pages = 1;

    if (page > max_pages || page < 1) {
        printf("Error: Page must be between 1 and %d\n", max_pages);
        return;
    }

    printf("=== Help (Page %d of %d) ===\n\n", page, max_pages);
    
    int start_index = (page - 1) * max_commands_in_page;
    int end_index = start_index + max_commands_in_page;
    
    for (int i = start_index; i < end_index && i < length; i++) {
        printf("%s - %s\n", commands[i].name, commands[i].description);
    }
    
    if (max_pages > 1) {
        printf("\n");
        printf("Navigation: ");
        
        if (page > 1) {
            printf("prev: help %d", page - 1);
            if (page < max_pages) {
                printf(" | ");
            }
        }
        
        if (page < max_pages) {
            printf("next: help %d", page + 1);
        }
        
        printf("\n");
    }
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
            printf("Background color set to ");
            printnumber(color);
            printf("\n");
        } else {
            printf("Invalid color! Use 0-15\n");
        }
    } else {
        printf("Usage: setbgcolor <0-15>\n");
    }
}

void cmd_clear(char **tokens) {
    clear_screen();
}

void cmd_exit(char **tokens) {
    console.should_exit = 1;
    printf("Exiting console...\n");
}

void cmd_memview(char **tokens) {
    create_process((void (*)(int, char**))memoryViewer.main, 0, 0, "memview", kernel_dir);
}

void cmd_kmalloc(char **tokens) {
    if (!tokens[1]) {
        printf("Usage: kmalloc <size>\n");
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
    create_process((void (*)(int, char**))disk_viewer.main, 0, 0, "diskview", kernel_dir);
}

void cmd_ls(char **tokens){
    fat32_ls();
}

void cmd_cat(char **tokens){
    if(!tokens[1]) {
        printf("Usage: cat <filename>\n");
        return;
    }

    int file_size = fat32_get_file_size(tokens[1]);

    if(file_size <= 0){
        printf("File not found or empty.\n");
        return;
    }

    
    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 512);
    if (!file_buffer) {
        printf("Error: Not enough memory!\n");
        return;
    }

    for(int i=0; i<file_size; i++) file_buffer[i] = 0;
    
    fat32_read_file(tokens[1], file_buffer);

    for(int i = 0; i < file_size; i++) {
        char c = (char)file_buffer[i];
        printf("%c", c);
    }
    printf("\n");

    kfree_a(file_buffer); 
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
    if(!tokens[1]){
        printf("Usage: exec <filename>\n");
        return;
    }
    if (!is_executable(tokens[1])) {
        printf("File not is executable. Only '.bin' files.\n.");
        return;
    }

    int file_size = fat32_get_file_size(tokens[1]);
    if (file_size <= 0) {
        printf("File empty or not found.\n");
        return;
    }

    uint32_t alloc_size = file_size + 1024 * 1024; 
    
    uint32_t phys_addr = (uint32_t)kmalloc_a(alloc_size);
    if (!phys_addr) {
        printf("Error: Not enough memory to load executable!\n");
        return;
    }
    fast_memset((void*)phys_addr, 0, alloc_size / 4);
    
    int bytes_read = fat32_read_file(tokens[1], (uint8_t*)phys_addr);

    if(bytes_read > 0){
        keyboard_flush();
        int argc = 0;
        while (tokens[1 + argc] != 0) argc++;

        page_directory_t *app_pd = clone_page_directory();
        
        uint32_t size_aligned = (alloc_size + 4095) & ~4095;
        for(uint32_t i = 0; i < size_aligned; i += 4096) {
            paging_map_user(app_pd, phys_addr + i, PROGRAM_LOAD_ADDRES + i, 7);
        }

        int pid = create_process((void (*)(int, char**))PROGRAM_LOAD_ADDRES, argc, &tokens[1], tokens[1], app_pd);
        
        Task *t = ready_queue;
        do {
            if (t->id == pid) { t->app_phys_addr = phys_addr; break; }
            t = t->next;
        } while (t != ready_queue);

        if(strcmp(tokens[argc], "&") != 0) wait_process(pid);
    } else {
        printf("Error loading file.\n");
        kfree_a((void*)phys_addr);
    }
}

void cmd_mkfile(char **tokens) {
    if (!tokens[1]) {
        printf("Usage: mkfile <name> <text>\n");
        return;
    }
    
    char* filename = tokens[1];
    char text_buf[512];
    text_buf[0] = '\0';
    
    if (tokens[2]) {
        int i = 2;
        while(tokens[i]) {
            strcat(text_buf, tokens[i]);
            if (tokens[i+1]) strcat(text_buf, " ");
            i++;
        }
    }
    
    int res = fat32_write_file(filename, (uint8_t*)text_buf, strlen(text_buf));
    if (res == 1) {
        printf("File '%s' created successfully!\n", filename);
    } else if (res == -1) {
        printf("%CError: OS is in READ-ONLY mode! Type 'writemode' to enable.%C\n", VGA32_COLOR_RED, VGA32_COLOR_WHITE);
    } else {
        printf("%CError creating file. Code: %d%C\n", VGA32_COLOR_RED, res, VGA32_COLOR_WHITE);
    }
}

void cmd_writemode(char **tokens) {
    isReadMode = 0;
    printf("%CRead/Write mode ENABLED! You can now create and save files.%C\n", VGA32_COLOR_GREEN, VGA32_COLOR_WHITE);
}

void cmd_rm(char **tokens){
    if(fat32_delete_file(tokens[1]) != 1) printf("File not removed\n");
}

void cmd_readsystemcfg(char **tokens) {
    int file_size = fat32_get_file_size("kernel.cfg");

    if(file_size <= 0){
        printf("Warning: kernel.cfg not found! Using default system config.\n");
        return;
    }

    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 512);
    if(!file_buffer) return;
    for(int i=0; i<file_size; i++) file_buffer[i] = 0;

    fat32_read_file("kernel.cfg", file_buffer);
    Config *cfg = config_parse((char *)file_buffer);

    isReadMode = (strcmp(config_get_value(cfg, "is_read_only_mode"), "true") == 0) ? 1 : 0;

    char *v;
    if((v = config_get_value(cfg, "taskbar_color"))) taskbar_color = (uint32_t)atoi(v, 16);
    if((v = config_get_value(cfg, "window_border_color"))) window_border_color = (uint32_t)atoi(v, 16);
    if((v = config_get_value(cfg, "window_active_border_color"))) window_active_border_color = (uint32_t)atoi(v, 16);

    
    if((v = config_get_value(cfg, "key_kill"))) key_kill = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_focus"))) key_focus = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_console"))) key_console = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_layout"))) key_layout = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_fullscreen"))) key_fullscreen = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_ws_left"))) key_ws_left = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_ws_right"))) key_ws_right = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_left"))) key_resize_left = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_right"))) key_resize_right = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_up"))) key_resize_up = (uint8_t)atoi(v, 16);
    if((v = config_get_value(cfg, "key_resize_down"))) key_resize_down = (uint8_t)atoi(v, 16);
    
    if((v = config_get_value(cfg, "wm_gaps"))) wm_gaps = atoi(v, 10);

    vesa_render_buffer();
    config_free(cfg);
    kfree_a(file_buffer);
}

void cmd_tasklist(char **tokens){
    if (!ready_queue) return;
    Task *t = ready_queue;
    
    printf("PID   State      Parent   Name\n");
    printf("---   -----      ------   ----\n");
    
    do {
        if (t->state != TASK_DEAD) {
            printf("%d", t->id);
            if(t->id < 10) printf("     ");
            else if(t->id < 100) printf("    ");
            else printf("   ");

            if (t->state == TASK_RUNNING) printf("RUN        ");
            else if (t->state == TASK_SLEEPING) printf("SLEEP      ");
            else printf("READY      ");
            
            if(t->parent_id == -1) printf("NONE     ");
            else {
                printf("%d", t->parent_id);
                if(t->parent_id < 10) printf("        ");
                else printf("       ");
            }

            printf("%s\n", t->name);
        }
        t = t->next;
    } while (t != ready_queue);
}

void cmd_kill(char **tokens){
    if(!tokens[1]){
        printf("Usage: kill <PID>");
        return;
    }

    if(!ready_queue) return;

    int pid = strtn(tokens[1]);

    kill_task(pid);
}

void cmd_disks(char **tokens) {
    printf("--- Connected Drives ---\n");
    if (sys_drive_count == 0) {
        printf("No drives found.\n");
        return;
    }

    for (int i = 0; i < sys_drive_count; i++) {
        if (i == active_drive_index) {
            printf("%C*> [%d] %s%C\n", VGA32_COLOR_GREEN, i, sys_drives[i].name, VGA32_COLOR_WHITE);
        } else {
            printf("   [%d] %s\n", i, sys_drives[i].name);
        }
    }
}

void cmd_use(char **tokens) {
    if (!tokens[1]) {
        printf("Usage: use <id>\n");
        printf("Run 'disks' to see available IDs.\n");
        return;
    }
    
    int idx = strtn(tokens[1]);
    if (disk_select(idx)) {
        printf("Switched to drive: %s\n", sys_drives[idx].name);
    } else {
        printf("%CError: Invalid drive ID!%C\n", VGA32_COLOR_RED, VGA32_COLOR_WHITE);
    }
}