#include "system.h"
#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "drivers/rtc/rtc.h"
#include "mm/memory.h"
#include "fs/vfs.h"      
#include "fs/fat/fat32.h"
#include <stdint.h>
#include "drivers/keyboard/keyboardDriver.h"
#include "utils/utils.h"
#include "multiboot.h"
#include "global.h"
#include "drivers/video/vesa.h"
#include "task/task.h"
#include "drivers/file/ATA/ATA.h"
#include "utils/sysconfig.h"

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
    {"fetch",         cmd_colortest,     "Show OS info"},
    
    {"kmalloc",       cmd_kmalloc,       "Allocate memory (kmalloc 256)"},
    {"kfree",         cmd_kfree,         "Free memory (kfree <addr>)"},
    {"heapdump",      cmd_heapdump,      "Dump heap state"},

    {"ls",            cmd_ls,            "Show all files"},
    {"cd",            cmd_cd,            "Change directory"},
    {"mkdir",         cmd_mkdir,         "Make directory"},
    {"cat",           cmd_cat,           "Show file (cat <file>)"},
    {"exec",          cmd_exec,          "Executable file (exec <file>)"},
    {"mkfile",        cmd_mkfile,        "Make file (mkfile <file name> <text>)"},
    {"rm",            cmd_rm,            "Remove file (rm <file name>)"},
    {"kcfgup",        cmd_readsystemcfg, "Update system configs"},
    {"tasklist",      cmd_tasklist,      "Show task list"},
    {"kill",          cmd_kill,          "Kill process"},
    {"writemode",     cmd_writemode,     "Enable Read/Write disk mode"},
    {"disks",         cmd_disks,         "List all connected drives"},
    {"use",           cmd_use,           "Switch active drive (Obsolete)"},

    {"lsblk",         cmd_lsblk,         "List block devices"},
    {"mount",         cmd_mount,         "Mount a filesystem"},
    {"umount",        cmd_umount,        "Unmount a filesystem"},
    
    {NULL, NULL, NULL}
};

void cmd_lsblk(char **tokens) {
    int sz = vfs_get_size("/proc/disks");
    if (sz <= 0) {
        printf("lsblk: failed to read /proc/disks\n");
        return;
    }
    uint8_t* buf = kmalloc(sz + 1);
    vfs_read("/proc/disks", buf);
    buf[sz] = '\0';
    
    set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    printf("\n=== Block Devices ===\n");
    set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    printf("%s\n", buf);
    kfree(buf);
}

void cmd_mount(char **tokens) {
    if (!tokens[1] || !tokens[2] || !tokens[3]) {
        printf("Usage: mount /dev/sdX /mnt/folder fs_type\n");
        return;
    }
    int res = vfs_mount(tokens[1], tokens[2], tokens[3]);
    if (res == 0) printf("Mounted %s to %s successfully.\n", tokens[1], tokens[2]);
    else printf("Mount failed with code: %d\n", res);
}

void cmd_umount(char **tokens) {
    if (!tokens[1]) {
        printf("Usage: umount /mnt/folder\n");
        return;
    }
    if (vfs_unmount(tokens[1]) == 0) printf("Unmounted successfully.\n");
    else printf("Unmount failed.\n");
}

void register_commands(void) {}

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
    if (tokens[1] == 0) { printf("\n"); return; }
    int i = 1;
    while(tokens[i] != 0) {
        printf(tokens[i]);
        if(tokens[i+1] != 0) printf(" ");
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
    char *s1 = tokens[1], *s2 = tokens[3];
    while(*s1 >= '0' && *s1 <= '9') { num1 = num1 * 10 + (*s1 - '0'); s1++; }
    while(*s2 >= '0' && *s2 <= '9') { num2 = num2 * 10 + (*s2 - '0'); s2++; }
    char op = tokens[2][0];
    int result = 0;
    if(op == '+') result = num1 + num2;
    else if(op == '-') result = num1 - num2;
    else if(op == '*') result = num1 * num2;
    else if(op == '/') result = num1 / num2;
    else { printf("Unknown operation! Use + - * /\n"); return; }
    printf("Result: "); printnumber(result); printf("\n");
}

void cmd_time(char **tokens) {
    printf("\nCurrent Time:\n");
    printf("Time: %d:%d:%d\n", rtc_get_time().hour, rtc_get_time().minute, rtc_get_time().second);
    printf("Date: %d/%d/%d\n", rtc_get_time().day, rtc_get_time().month, rtc_get_time().year);
}

void cmd_ascii(char **tokens) {
    printf("\nASCII Table:\n");
    for(int i = 32; i < 127; i++) {
        printf("%d: %c  ", i, (char)i);
        if((i - 31) % 8 == 0) printf("\n");
    }
    printf("\n");
}

void cmd_settextcolor(char **tokens) {
    if (tokens[1] != 0) {
        int color = strtn(tokens[1]);
        if (color >= 0 && color <= 15) {
            set_text_color(color);
            printf("Text color set to %d\n", color);
        } else printf("Invalid color! Use 0-15\n");
    } else printf("Usage: settextcolor <0-15>\n");
}

void cmd_setbgcolor(char** tokens){
    if (tokens[1] != 0) {
        int color = strtn(tokens[1]);
        if (color >= 0 && color <= 15) {
            set_background_color(color);
            printf("Background color set to %d\n", color);
        } else printf("Invalid color! Use 0-15\n");
    } else printf("Usage: setbgcolor <0-15>\n");
}

void cmd_help(char **tokens) {
    int page = 1;
    if (tokens[1] != NULL) page = strtn(tokens[1]);
    if (page == 0) page = 1;
    int length = 0;
    while(commands[length].name != NULL) length++;
    int max_commands_in_page = 10;
    int max_pages = (length + max_commands_in_page - 1) / max_commands_in_page;
    if (max_pages == 0) max_pages = 1;
    if (page > max_pages || page < 1) { printf("Error: Page must be between 1 and %d\n", max_pages); return; }
    printf("=== Help (Page %d of %d) ===\n\n", page, max_pages);
    int start_index = (page - 1) * max_commands_in_page;
    int end_index = start_index + max_commands_in_page;
    for (int i = start_index; i < end_index && i < length; i++) {
        printf("%s - %s\n", commands[i].name, commands[i].description);
    }
}

void cmd_clear(char **tokens) { clear_screen(); }
void cmd_exit(char **tokens) { console.should_exit = 1; printf("Exiting console...\n"); }
void cmd_kmalloc(char **tokens) { if (!tokens[1]) return; ptr = kmalloc(strtn(tokens[1])); }
void cmd_kfree(char **tokens) { kfree(ptr); }
void cmd_heapdump(char **tokens) { heap_dump(); }

void cmd_ls(char **tokens) {
    char target_path[64];
    if (tokens[1]) strcpy(target_path, tokens[1]);
    else strcpy(target_path, "."); 
    
    
    vfs_dirent_t entry; 
    int index = 0;
    while (1) {
        int ret;
        
        __asm__ volatile("int $0x80" : "=a"(ret) : "a"(89), "b"(target_path), "c"(index), "d"(&entry));
        if (ret == 0) break; 
        if (entry.type == 1) printf("[DIR]  %s\n", entry.name);
        else if (entry.type == 2) printf("[DEV]  %s\n", entry.name);
        else printf("[FILE] %s \t%d bytes\n", entry.name, entry.size);
        index++;
    }
}

void cmd_cd(char **tokens) {
    if (!tokens[1]) { printf("Usage: cd <path>\n"); return; }
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(12), "b"(tokens[1]));
    if (ret == -1) printf("cd: Directory not found or not a directory.\n");
}

void cmd_mkdir(char **tokens) {
    if (!tokens[1]) { printf("Usage: mkdir <name>\n"); return; }
    int ret;
    __asm__ volatile("int $0x80" : "=a"(ret) : "a"(39), "b"(tokens[1]));
    if (ret == -1) printf("mkdir: Failed to create directory.\n");
    else if (ret == -2) printf("mkdir: Disk is full.\n");
    else printf("mkdir: Directory created successfully.\n");
}

void cmd_cat(char **tokens){
    if(!tokens[1]) { printf("Usage: cat <filename>\n"); return; }
    
    char abs_path[256];
    get_absolute_path(current_task->cwd, tokens[1], abs_path);

    int file_size = vfs_get_size(abs_path); 
    if(file_size <= 0){ printf("File not found or empty.\n"); return; }
    
    uint8_t *file_buffer = (uint8_t*)kmalloc_a(file_size + 512);
    if (!file_buffer) { printf("Error: Not enough memory!\n"); return; }
    for(int i=0; i<file_size; i++) file_buffer[i] = 0;
    
    vfs_read(abs_path, file_buffer); 
    for(int i = 0; i < file_size; i++) { printf("%c", (char)file_buffer[i]); }
    printf("\n");

    kfree_a(file_buffer); 
}

int is_executable(char* filename) {
    int len = strlen(filename);
    if (len < 4) return 0;
    char* ext = filename + len - 4;
    if (ext[0] != '.') return 0;
    if (ext[1] != 'B' && ext[1] != 'b') return 0;
    if (ext[2] != 'I' && ext[2] != 'i') return 0;
    if (ext[3] != 'N' && ext[3] != 'n') return 0;
    return 1;
}

void cmd_exec(char **tokens){
    if(!tokens[1]){ printf("Usage: exec <filename>\n"); return; }

    char abs_path[256];
    get_absolute_path(current_task->cwd, tokens[1], abs_path);

    int file_size = vfs_get_size(abs_path); 
    if (file_size <= 0) { printf("File empty or not found.\n"); return; }

    uint32_t alloc_size = file_size + 1024 * 1024; 
    uint32_t phys_addr = (uint32_t)kmalloc_a(alloc_size);
    if (!phys_addr) { printf("Error: Not enough memory to load executable!\n"); return; }
    fast_memset((void*)phys_addr, 0, alloc_size / 4);
    
    int bytes_read = vfs_read(abs_path, (uint8_t*)phys_addr); 

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
    if (!tokens[1]) { printf("Usage: mkfile <name> <text>\n"); return; }
    
    char abs_path[256];
    get_absolute_path(current_task->cwd, tokens[1], abs_path);
    
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
    
    int res = vfs_write(abs_path, (uint8_t*)text_buf, strlen(text_buf)); 
    if (res > 0) { printf("File '%s' created successfully!\n", tokens[1]); } 
    else if (res == -1) { printf("%CError: Directory is Read-Only or Invalid!%C\n", VGA32_COLOR_RED, VGA32_COLOR_WHITE); } 
    else { printf("%CError creating file. Code: %d%C\n", VGA32_COLOR_RED, res, VGA32_COLOR_WHITE); }
}

void cmd_rm(char **tokens){
    if(!tokens[1]) { printf("Usage: rm <filename>\n"); return; }
    
    char abs_path[256];
    get_absolute_path(current_task->cwd, tokens[1], abs_path);
    
    if(vfs_delete(abs_path) != 1) printf("rm: Failed to remove file or directory.\n"); 
    else printf("rm: File removed successfully.\n");
}

void cmd_writemode(char **tokens) {
    isReadMode = !isReadMode;
    if (isReadMode == 0) { printf("%CRead/Write mode ENABLED! You can now create and save files.%C\n", VGA32_COLOR_GREEN, VGA32_COLOR_WHITE); }
    else { printf("%CRead-Only mode ENABLED! Disk writing disabled.%C\n", VGA32_COLOR_YELLOW, VGA32_COLOR_WHITE); }
}

void cmd_readsystemcfg(char **tokens) {
    printf("Reloading system configuration...\n");
    sysconfig_reload();
    printf("Done!\n");
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
    if(!tokens[1]){ printf("Usage: kill <PID>"); return; }
    if(!ready_queue) return;
    kill_task(strtn(tokens[1]));
}

void cmd_disks(char **tokens) {
    printf("--- Connected Physical Drives ---\n");
    if (sys_drive_count == 0) { printf("No physical drives found.\n"); return; }
    for (int i = 0; i < sys_drive_count; i++) {
        printf("   [%d] %s\n", i, sys_drives[i].name);
    }
}

void cmd_use(char **tokens) {
    printf("The 'use' command is obsolete in the new VFS architecture.\n");
    printf("Please use 'mount /dev/sdX /mnt/point fat32' from userland shell instead.\n");
}