#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "global.h"
#include "programs/system/console/system.h"
#include "utils/utils.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "drivers/video/graphics.h"
#include "multitask/task.h"

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

void draw_console_cursor(Window *win, int state) {
    uint32_t color = state ? 0xFFFFFFFF : win->bg_color;
    if (win->cursor_y < win->height) {
        draw_rect_filled(win->x + win->cursor_x, win->y + win->cursor_y, 8, 8, color);
        vesa_render_rect(win->x + win->cursor_x, win->y + win->cursor_y, 8, 8);
    }
}

int console_main(void) {
    keyboard_flush();
    Window *my_win = wm_create_window(VGA32_COLOR_BLACK);
    current_task->window = my_win; 
    current_task->owns_window = 1; 
    wm_set_focused_window(my_win);
    printf("\n");
    
    int local_should_exit = 0;

    while(!local_should_exit) { 
        current_task->window = my_win;
        printf("%s> ", path);
        draw_console_cursor(my_win, 1);

        char command[CMD_MAX_LEN];
        memset(command, 0, CMD_MAX_LEN);
        int pos = 0;
        history_browse_idx = history_count;

        while(1) {
            draw_console_cursor(my_win, 1);
            uint8_t scancode = wait_scancode();
            draw_console_cursor(my_win, 0);

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
                    while (pos > 0 && (command[pos] & 0xC0) == 0x80) pos--;
                    command[pos] = '\0';
                    printf("\b \b");
                }
            }
            else if (scancode == 0x48) {
                if (history_browse_idx > 0) {
                    
                    int visual_len = 0;
                    for(int i=0; i<pos; i++) if((command[i]&0xC0) != 0x80) visual_len++;
                    clear_current_line(visual_len);
                    history_browse_idx--;
                    strcpy(command, history[history_browse_idx]);
                    pos = strlen(command);
                    printf(command);
                }
            }
            else if (scancode == 0x50) {
                if (history_browse_idx < history_count) {
                    int visual_len = 0;
                    for(int i=0; i<pos; i++) if((command[i]&0xC0) != 0x80) visual_len++;
                    clear_current_line(visual_len);
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
                unsigned int c = scancode_to_char_layout(scancode);
                if (c != 0) {
                    char tmp[4];
                    int bytes = utf8_encode(c, tmp);
                    if (pos + bytes < CMD_MAX_LEN - 1) {
                        for(int i=0; i<bytes; i++) command[pos++] = tmp[i];
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