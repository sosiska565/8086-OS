#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "programs/system/console/system.h"
#include "utils/utils.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"

#define HISTORY_SIZE 10
#define CMD_MAX_LEN 100

char history[HISTORY_SIZE][CMD_MAX_LEN];
int history_count = 0;
int history_browse_idx = 0;

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
    //printf("\n");
    if(!tokens[0]) return;
    
    for(int i = 0; commands[i].name != NULL; i++) {
        if(strcmp(tokens[0], commands[i].name) == 0) {
            commands[i].handler(tokens);
            return;
        }
    }
    
    printf("Unknown command: %s\n", 
           tokens[0]);
}


int console_main(void) {
    char *empty_args[] = {NULL};
    
    init_gfx_console();
    clear_screen_vesa(0x00000000);
    
    cmd_colortest(empty_args);
    printf("\n");
    
    console.should_exit = 0;

    while(!console.should_exit) {
        keyboard_flush();
        
        printf("%s> ", path);

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
                break;
            }

            else if (scancode == 0x0E) {
                if (pos > 0) {
                    pos--;
                    command[pos] = '\0';
                    printf("\b \b");
                }
            }

            else if (scancode == 0x48) {
                if (history_browse_idx > 0) {
                    clear_current_line(pos);
                    history_browse_idx--;
                    strcpy(command, history[history_browse_idx]);
                    pos = strlen(command);
                    printf(command);
                }
            }

            else if (scancode == 0x50) {
                if (history_browse_idx < history_count) {
                    clear_current_line(pos);
                    history_browse_idx++;

                    if (history_browse_idx == history_count) {
                        pos = 0;
                        command[0] = '\0';
                    } else {
                        strcpy(command, history[history_browse_idx]);
                        pos = strlen(command);
                        printf(command);
                    }
                }
            }

            else {
                if (scancode & 0x80) continue;

                char c = scancode_to_char(scancode);
                
                if (c != 0 && pos < CMD_MAX_LEN - 1) {
                    command[pos] = c;
                    pos++;
                    printf("%c", c);
                }
            }
        }
        
        if(command[0] == '\0') continue;
        
        char **tokens = parse_str(command, ' ');
        execute_command(tokens);
    }
    
    return 0;
}

Console console = {
    .main = console_main,
    .should_exit = 0
};