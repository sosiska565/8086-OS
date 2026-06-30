/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/system_apps/console/console.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */



#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "system_apps/console/system.h"
#include "utils/utils.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "task/task.h"

#define HISTORY_SIZE 10
#define CMD_MAX_LEN 100

char history[HISTORY_SIZE][CMD_MAX_LEN];
int history_count = 0;
int history_browse_idx = 0;

int utf8_encode(unsigned int code, char *buffer) {
    if (code < 0x80) {
        buffer[0] = code;
        return 1;
    } else if (code < 0x800) {
        buffer[0] = 0xC0 | (code >> 6);
        buffer[1] = 0x80 | (code & 0x3F);
        return 2;
    } else {
        buffer[0] = 0xE0 | (code >> 12);
        buffer[1] = 0x80 | ((code >> 6) & 0x3F);
        buffer[2] = 0x80 | (code & 0x3F);
        return 3;
    }
}

void add_to_history(const char* cmd) {
    if (cmd[0] == '\0') return;

    if (history_count > 0 && strcmp(history[history_count - 1], cmd) == 0) {
        history_browse_idx = history_count;
        return;
    }

    if (history_count < HISTORY_SIZE) {
        strcpy(history[history_count], cmd);
        history_count++;
    } else {
        for (int i = 1; i < HISTORY_SIZE; i++) {
            strcpy(history[i-1], history[i]);
        }
        strcpy(history[HISTORY_SIZE-1], cmd);
    }
    history_browse_idx = history_count;
}

void clear_current_line(int len) {
    for (int i = 0; i < len; i++) {
        printf("\b \b");
    }
}

void execute_command(char **tokens) {
    if(!tokens[0]) return;
    for(int i = 0; commands[i].name != NULL; i++) {
        if(strcmp(tokens[0], commands[i].name) == 0) {
            commands[i].handler(tokens);
            return;
        }
    }
    printf("Unknown command: %s\n", tokens[0]);
}

int console_main(void) {
    keyboard_flush();
    
    printf("Welcome to 8086-OS Shell\n");
    
    int local_should_exit = 0;

    while(!local_should_exit) {
        char cwd_buf[64];
    __asm__ volatile("int $0x80" : : "a"(79), "b"(cwd_buf));
    printf("%s> ", cwd_buf);

        char command[CMD_MAX_LEN];
        memset(command, 0, CMD_MAX_LEN);
        int pos = 0;
        history_browse_idx = history_count;

        while(1) {
            uint8_t scancode = wait_scancode();

            if (scancode == 0x1C) { 
                command[pos] = '\0';
                printf("\n");
                add_to_history(command);
                if (strcmp(command, "exit") == 0) local_should_exit = 1;
                break;
            }
            else if (scancode == 0x0E) { 
                if (pos > 0) {
                    pos--;
                    command[pos] = '\0';
                    printf("\b \b");
                }
            }
            else {
                if (scancode & 0x80) continue;
                unsigned int c = scancode_to_char_layout(scancode);
                if (c != 0) {
                    if (pos < CMD_MAX_LEN - 1) {
                        command[pos++] = (char)c;
                        printf("%c", c);
                    }
                }
            }
        }
        
        if(command[0] == '\0' || local_should_exit) continue;
        char **tokens = parse_str(command, ' ');
        execute_command(tokens);
    }
    return 0;
}

Console console = {
    .main = console_main,
    .should_exit = 0
};
