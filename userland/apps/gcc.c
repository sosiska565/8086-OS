/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/gcc.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        set_color(COLOR_RED, COLOR_BLACK);
        printf("Error: Missing input files.\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        printf("Usage: gcc file1.c file2.c -o output.elf\n");
        return 1;
    }

    char* out_file = "a.elf";
    
    char* tcc_args[256]; 
    int tcc_argc = 0;

    tcc_args[tcc_argc++] = "tcc";
    tcc_args[tcc_argc++] = "-nostdlib";
    tcc_args[tcc_argc++] = "-I/include";
    tcc_args[tcc_argc++] = "-Wl,-Ttext=0x60000000";

    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            out_file = argv[i + 1];
            i++;
        } else {
            tcc_args[tcc_argc++] = argv[i];
        }
    }

    tcc_args[tcc_argc++] = "-o";
    tcc_args[tcc_argc++] = out_file;

    
    
    tcc_args[tcc_argc++] = "/lib/entry.o";

    
    vfs_dirent_t ent;
    int idx = 0;
    while (readdir("/lib", idx++, &ent) == 1) {
        if (ent.type == VFS_ATTR_FILE) {
            int len = strlen(ent.name);
            
            
            if (len > 2 && ent.name[len-2] == '.' && 
               (ent.name[len-1] == 'o' || ent.name[len-1] == 'O')) {
                
                
                if (strcmp(ent.name, "entry.o") == 0 || strcmp(ent.name, "ENTRY.O") == 0) continue; 
                
                
                char* obj_path = malloc(256);
                sprintf(obj_path, "/lib/%s", ent.name);
                
                tcc_args[tcc_argc++] = obj_path;
                
                
                if (tcc_argc >= 254) break;
            }
        }
    }

    tcc_args[tcc_argc++] = NULL;

    printf("Compiling to %s...\n", out_file);

    
    int fd_log = open("/gcc.log", O_WRONLY | O_CREAT | O_TRUNC);
    int pid = spawn_ext("/path/tcc.elf", tcc_args, -1, fd_log);

    if (pid < 0) {
        printf("Failed to execute TCC!\n");
        close(fd_log);
        return 1;
    }

    waitpid(pid);
    close(fd_log);

    
    int log_sz = get_file_size("/gcc.log");
    if (log_sz > 0) {
        set_color(COLOR_YELLOW, COLOR_BLACK);
        printf("Compiler output:\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        char* cat_args[] = {"cat", "/gcc.log", NULL};
        waitpid(spawn("/path/cat.elf", cat_args, NULL));
    }

    if (get_file_size(out_file) > 0) {
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("\nBuild successful! Run with: ./%s\n", out_file);
        set_color(COLOR_WHITE, COLOR_BLACK);
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("\nBuild failed. ;(\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
    }

    return 0;
}
