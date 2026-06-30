/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/gterm.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

#define MAX_LINES 1000
#define MAX_COLS 200
#define CHAR_W 8
#define CHAR_H 8

#define C_BG        0x001E1E1E
#define C_TEXT      0x00CCCCCC
#define C_ERROR     0x00FF0000 

typedef struct {
    char c;
    uint32_t fg;
    uint32_t bg;
} TermChar;

TermChar *term_grid;
#define GRID(r, c) term_grid[(r) * MAX_COLS + (c)]

int abs_cursor_y = 0; 
int cursor_x = 0;
int max_drawn_y = 0; 

int term_cols = 80;
int term_rows = 24;
int scroll_offset = 0; 

char input_buffer[256];
int input_len = 0;
int input_pos = 0; 
int prompt_start_x = 0;
int prompt_start_y = 0;

#define MAX_HISTORY 16
char history[MAX_HISTORY][256];
int history_count = 0;
int hist_idx = 0;

#define MAX_BINARIES 64
char path_cmds[MAX_BINARIES][32];
int path_cmd_count = 0;
char env_path[256] = "/path";
const char* builtins[] = { "help", "cd", "pwd", "exit", "clear", "rehash", "mount", "umount", NULL };

static int active_child_pid = 0;
static int is_dragging_sb = 0;

static const uint32_t vga_to_rgb[] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

static int ansi_state = 0;
static char ansi_buf[64];
static int ansi_pos = 0;
static uint32_t term_fg_color = C_TEXT;
static uint32_t term_bg_color = C_BG;

void term_clear() {
    for (int r = 0; r < MAX_LINES; r++) {
        for (int c = 0; c < MAX_COLS; c++) {
            GRID(r, c).c = ' ';
            GRID(r, c).fg = C_TEXT;
            GRID(r, c).bg = C_BG;
        }
    }
    abs_cursor_y = 0; cursor_x = 0; scroll_offset = 0; max_drawn_y = 0;
}

void term_newline() {
    cursor_x = 0;
    abs_cursor_y++;
    if (abs_cursor_y > max_drawn_y) max_drawn_y = abs_cursor_y; 

    if (max_drawn_y >= MAX_LINES) {
        int shift = 100;
        memmove(&GRID(0, 0), &GRID(shift, 0), (MAX_LINES - shift) * MAX_COLS * sizeof(TermChar));
        for (int i = MAX_LINES - shift; i < MAX_LINES; i++) {
            for (int c = 0; c < MAX_COLS; c++) {
                GRID(i, c).c = ' ';
                GRID(i, c).bg = C_BG;
            }
        }
        abs_cursor_y -= shift;
        max_drawn_y -= shift;
        prompt_start_y -= shift;
    }
    if (scroll_offset > 0) scroll_offset = 0;
}

void term_putchar(char c, uint32_t fg) {
    if (ansi_state == 1) {
        if (c == '[') { ansi_state = 2; ansi_pos = 0; ansi_buf[0] = '\0'; }
        else ansi_state = 0;
        return;
    } else if (ansi_state == 2) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            ansi_buf[ansi_pos] = '\0';
            
            if (c == 'm') { 
                if (ansi_pos == 0) {
                    term_fg_color = C_TEXT; term_bg_color = C_BG;
                } else {
                    char *p = ansi_buf;
                    while (*p) {
                        int code = atoi(p);
                        if (code == 0) { term_fg_color = C_TEXT; term_bg_color = C_BG; }
                        else if (code >= 30 && code <= 37) term_fg_color = vga_to_rgb[code - 30];
                        else if (code >= 40 && code <= 47) term_bg_color = vga_to_rgb[code - 40];
                        else if (code >= 90 && code <= 97) term_fg_color = vga_to_rgb[code - 90 + 8];
                        else if (code >= 100 && code <= 107) term_bg_color = vga_to_rgb[code - 100 + 8];
                        
                        while (*p && *p != ';') p++;
                        if (*p == ';') p++;
                    }
                }
            } else if (c == 'J') {
                if (ansi_pos == 0 || strcmp(ansi_buf, "2") == 0) {
                    int start_y = max_drawn_y - term_rows + 1 - scroll_offset;
                    if (start_y < 0) start_y = 0;
                    for(int r = start_y; r < start_y + term_rows; r++) {
                        if (r >= MAX_LINES) break;
                        for(int col=0; col<term_cols; col++) {
                            GRID(r, col).c = ' '; GRID(r, col).bg = term_bg_color;
                        }
                    }
                }
            } else if (c == 'H' || c == 'f') {
                int r = 1, col = 1;
                if (ansi_pos > 0) {
                    char *semicolon = strchr(ansi_buf, ';');
                    if (semicolon) { *semicolon = '\0'; r = atoi(ansi_buf); col = atoi(semicolon + 1); } 
                    else r = atoi(ansi_buf);
                }

                if (r < 1) r = 1; if (col < 1) col = 1;
                if (r > term_rows) r = term_rows; if (col > term_cols) col = term_cols;

                int start_y = max_drawn_y - term_rows + 1 - scroll_offset;
                if (start_y < 0) start_y = 0;
                
                abs_cursor_y = start_y + r - 1;
                cursor_x = col - 1;

                if (abs_cursor_y > max_drawn_y) max_drawn_y = abs_cursor_y;
                if (abs_cursor_y >= MAX_LINES) abs_cursor_y = MAX_LINES - 1;
                if (cursor_x >= MAX_COLS) cursor_x = MAX_COLS - 1;
            } else if (c == 'A') { 
                int n = atoi(ansi_buf); if(n==0) n=1;
                abs_cursor_y -= n; if(abs_cursor_y < 0) abs_cursor_y = 0;
            } else if (c == 'B') { 
                int n = atoi(ansi_buf); if(n==0) n=1;
                abs_cursor_y += n; if(abs_cursor_y >= MAX_LINES) abs_cursor_y = MAX_LINES - 1;
                if (abs_cursor_y > max_drawn_y) max_drawn_y = abs_cursor_y;
            } else if (c == 'C') { 
                int n = atoi(ansi_buf); if(n==0) n=1;
                cursor_x += n; if(cursor_x >= term_cols) cursor_x = term_cols-1;
            } else if (c == 'D') { 
                int n = atoi(ansi_buf); if(n==0) n=1;
                cursor_x -= n; if(cursor_x < 0) cursor_x = 0;
            }
            ansi_state = 0;
        } else if (ansi_pos < 63) {
            ansi_buf[ansi_pos++] = c;
        } else {
            ansi_state = 0;
        }
        return;
    }
    
    if (c == 27) { ansi_state = 1; return; }

    if (c == '\n' || c == '\r') term_newline();
    else if (c == '\b') {
        if (cursor_x > 0) { cursor_x--; GRID(abs_cursor_y, cursor_x).c = ' '; }
    } else {
        if (cursor_x >= term_cols) term_newline();
        GRID(abs_cursor_y, cursor_x).c = c;
        GRID(abs_cursor_y, cursor_x).fg = fg != 0 ? fg : term_fg_color;
        GRID(abs_cursor_y, cursor_x).bg = term_bg_color;
        cursor_x++;
        if (abs_cursor_y > max_drawn_y) max_drawn_y = abs_cursor_y;
    }
}

void term_print(const char *str, uint32_t fg) { while (*str) term_putchar(*str++, fg); }

void render_terminal(gui_window_t *win) {
    gui_draw_rect(win, 0, 0, win->w, win->h, C_BG);
    
    int start_y = max_drawn_y - term_rows + 1 - scroll_offset;
    if (start_y < 0) start_y = 0;

    for (int r = 0; r < term_rows; r++) {
        int grid_y = start_y + r;
        if (grid_y < 0 || grid_y > max_drawn_y) continue;
        for (int c = 0; c < term_cols; c++) {
            if (GRID(grid_y, c).c != ' ' || GRID(grid_y, c).bg != C_BG) {
                gui_draw_rect(win, c * CHAR_W, r * CHAR_H, CHAR_W, CHAR_H, GRID(grid_y, c).bg);
                if (GRID(grid_y, c).c != ' ') {
                    gui_draw_char(win, c * CHAR_W, r * CHAR_H, GRID(grid_y, c).c, GRID(grid_y, c).fg);
                }
            }
        }
    }

    if (scroll_offset == 0 && active_child_pid == 0) {
        int screen_cy = (abs_cursor_y - start_y);
        if (screen_cy >= 0 && screen_cy < term_rows) {
            gui_draw_rect(win, cursor_x * CHAR_W, screen_cy * CHAR_H + 6, CHAR_W, 2, C_TEXT);
        }
    }

    int max_scroll = max_drawn_y - term_rows + 1;
    if (max_scroll > 0) {
        gui_draw_rect(win, win->w - 10, 0, 10, win->h, 0x00111111);
        int track_h = win->h;
        int sb_h = (term_rows * track_h) / (max_drawn_y + 1);
        if (sb_h < 20) sb_h = 20;
        int sb_y = ((max_scroll - scroll_offset) * (track_h - sb_h)) / max_scroll;
        gui_draw_rounded_rect(win, win->w - 8, sb_y, 6, sb_h, 3, is_dragging_sb ? 0x00888888 : 0x00555555);
    }
    gui_render(win); 
}

void to_lower_str(char* str) {
    for(int i = 0; str[i]; i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') str[i] += 32;
    }
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) { if (*prefix++ != *str++) return 0; } return 1;
}

void rehash_path() {
    path_cmd_count = 0; vfs_dirent_t entry; int idx = 0;
    while (readdir(env_path, idx++, &entry) == 1) {
        if (entry.type == VFS_ATTR_FILE && path_cmd_count < MAX_BINARIES) {
            to_lower_str(entry.name); 
            int len = strlen(entry.name);
            if (len > 4 && strcmp(entry.name + len - 4, ".elf") == 0) {
                strncpy(path_cmds[path_cmd_count], entry.name, len - 4);
                path_cmds[path_cmd_count][len - 4] = '\0';    
            } else strcpy(path_cmds[path_cmd_count], entry.name);
            path_cmd_count++;
        }
    }
}

char* get_suggestion(char* input) {
    if (input[0] == '\0') return NULL;
    for (int i = 0; builtins[i] != NULL; i++) {
        if (starts_with(builtins[i], input)) return (char*)builtins[i];
    }
    for (int i = 0; i < path_cmd_count; i++) {
        if (starts_with(path_cmds[i], input)) return path_cmds[i];
    }
    return NULL;
}

void add_history(char* cmd) {
    if(cmd[0] == '\0') return;
    if(history_count > 0 && strcmp(history[history_count-1], cmd) == 0) return;
    if(history_count < MAX_HISTORY) strcpy(history[history_count++], cmd);
    else { for(int i=1; i<MAX_HISTORY; i++) strcpy(history[i-1], history[i]); strcpy(history[MAX_HISTORY-1], cmd); }
}

void draw_prompt() {
    char cwd[256]; getcwd(cwd, 256);
    term_print("\n", 0);
    term_print("\033[91m", 0); term_print("+--(", 0);
    term_print("\033[94m", 0); term_print("root@8086-os", 0);
    term_print("\033[91m", 0); term_print(")-[", 0);
    term_print("\033[97m", 0); term_print(cwd, 0);
    term_print("\033[91m", 0); term_print("]\n|-> ", 0);
    term_print("\033[0m", 0); 
    prompt_start_x = cursor_x;
    prompt_start_y = abs_cursor_y;
}

void update_input_line() {
    int cy = prompt_start_y; int cx = prompt_start_x;
    while (cy <= abs_cursor_y) {
        GRID(cy, cx).c = ' ';
        cx++; if(cx >= term_cols) { cx = 0; cy++; }
    }
    abs_cursor_y = prompt_start_y; cursor_x = prompt_start_x;
    for (int i = 0; i < input_len; i++) term_putchar(input_buffer[i], 0);

    if (input_pos == input_len && input_len > 0) {
        char* cmd_sug = get_suggestion(input_buffer);
        if (cmd_sug && strlen(cmd_sug) > input_len) term_print(cmd_sug + input_len, 0x00555555);
    }
    cursor_x = prompt_start_x + (input_pos % term_cols);
    abs_cursor_y = prompt_start_y + (input_pos / term_cols);
    if (abs_cursor_y > max_drawn_y) max_drawn_y = abs_cursor_y;
}

void split_args(char* input, char* argv[], int* argc) {
    *argc = 0; char* p = input;
    while (*p) {
        while (*p == ' ') p++; 
        if (!*p) break;
        if (*p == '>') { argv[(*argc)++] = ">"; *p = '\0'; p++; continue; }
        if (*p == '|') { argv[(*argc)++] = "|"; *p = '\0'; p++; continue; }
        if (*p == '"') { p++; argv[(*argc)++] = p; while (*p && *p != '"') p++; if (*p == '"') { *p = '\0'; p++; } } 
        else { 
            argv[(*argc)++] = p; 
            while (*p && *p != ' ' && *p != '>' && *p != '|') p++; 
            if (*p == '>' || *p == '|') { 
                char tmp = *p; *p = '\0'; p++; 
                if(tmp=='>') argv[(*argc)++] = ">"; else argv[(*argc)++] = "|"; 
            } else if (*p == ' ') { 
                *p = '\0'; p++; 
            } 
        }
    }
    argv[*argc] = NULL;
}

int resolve_path(char* cmd, char* resolved) {
    for(int i=0; cmd[i]; i++) if (cmd[i] == '/') { strcpy(resolved, cmd); return 1; }
    char full_path[256];
    sprintf(full_path, "%s/%s", env_path, cmd);
    if (get_file_size(full_path) > 0) { strcpy(resolved, full_path); return 1; }
    sprintf(full_path, "%s/%s.elf", env_path, cmd);
    if (get_file_size(full_path) > 0) { strcpy(resolved, full_path); return 1; }
    return 0;
}

void execute_command(gui_window_t *win) {
    term_print("\n", 0);
    if (input_len == 0) { draw_prompt(); return; }
    
    input_buffer[input_len] = '\0';
    add_history(input_buffer);

    char* args[32]; int argc;
    split_args(input_buffer, args, &argc);
    char* cmd = args[0];

    if (strcmp(cmd, "clear") == 0) { term_clear(); }
    else if (strcmp(cmd, "cd") == 0) { if (argc > 1) { if (chdir(args[1]) != 0) term_print("cd: No such directory\n", C_ERROR); } }
    else if (strcmp(cmd, "pwd") == 0) { char c[256]; getcwd(c, 256); term_print(c, 0); term_print("\n", 0); }
    else if (strcmp(cmd, "rehash") == 0) { rehash_path(); term_print("Path refreshed.\n", 0); }
    else if (strcmp(cmd, "exit") == 0) { 
        shm_get(1001, 0); 
        free(term_grid); gui_destroy_window(win); exit(0); 
    }
    else {
        char resolved[256];
        if (resolve_path(cmd, resolved)) {
            
            int pipe_idx = -1;
            char* redirect_file = NULL;
            for(int i=0; i<argc; i++) {
                if (strcmp(args[i], ">") == 0) { args[i] = NULL; if (i + 1 < argc) redirect_file = args[i+1]; break; }
                if (strcmp(args[i], "|") == 0) { args[i] = NULL; pipe_idx = i + 1; break; }
            }

            int in_pipe[2]; pipe(in_pipe);
            int out_pipe[2]; pipe(out_pipe);

            if (pipe_idx != -1) {
                char resolved2[256];
                if (resolve_path(args[pipe_idx], resolved2)) {
                    int pfd_inter[2]; pipe(pfd_inter);
                    int pid1 = spawn_ext(resolved, args, in_pipe[0], pfd_inter[1]);
                    int pid2 = spawn_ext(resolved2, &args[pipe_idx], pfd_inter[0], out_pipe[1]);
                    close(pfd_inter[0]); close(pfd_inter[1]);
                    close(out_pipe[1]); close(in_pipe[0]);
                    active_child_pid = pid2; 
                } else {
                    term_print("gterm: right command not found\n", C_ERROR);
                    close(in_pipe[0]); close(in_pipe[1]); close(out_pipe[0]); close(out_pipe[1]);
                    goto cleanup;
                }
            } else if (redirect_file) {
                int fd = open(redirect_file, O_WRONLY | O_CREAT | O_TRUNC);
                if(fd != -1) {
                    int pid = spawn_ext(resolved, args, in_pipe[0], fd);
                    close(fd); close(out_pipe[1]); close(in_pipe[0]);
                    active_child_pid = pid;
                } else {
                    term_print("gterm: failed to open redirect file\n", C_ERROR);
                    close(in_pipe[0]); close(in_pipe[1]); close(out_pipe[0]); close(out_pipe[1]);
                    goto cleanup;
                }
            } else {
                int pid = spawn_ext(resolved, args, in_pipe[0], out_pipe[1]);
                close(out_pipe[1]); close(in_pipe[0]);
                if (pid > 0) active_child_pid = pid;
            }

            if (active_child_pid > 0) {
                int main_pid = active_child_pid;
                char buf[4096]; int bytes;
                
                while (active_child_pid > 0) {
                    GUI_Event ev; int break_loop = 0;
                    while (gui_poll_event(win, &ev)) {
                        if (ev.type == GUI_EV_CLOSE) {
                            kill(main_pid, 9);
                            shm_get(1001, 0); free(term_grid); gui_destroy_window(win); exit(0);
                        }
                        if (ev.type == GUI_EV_RESIZE) {
                            gui_resize_buffer(win, ev.x, ev.y);
                            term_cols = (win->w - 12) / CHAR_W; term_rows = win->h / CHAR_H;
                            shm_get(1001, 8); int *sz = shm_map(1001);
                            if (sz) { sz[0] = term_cols; sz[1] = term_rows; }
                            render_terminal(win);
                            
                            
                            char dummy = 0; write(in_pipe[1], &dummy, 1);
                        }
                        if (ev.type == GUI_EV_KEY_PRESS) {
                            char key_char = (char)(ev.keycode & 0xFF);
                            uint8_t mods = (ev.keycode >> 24) & 0xFF;
                            if ((mods & 2) && (key_char == 'c' || key_char == 'C')) { 
                                kill(main_pid, 9); term_print("^C\n", C_ERROR);
                                active_child_pid = 0; break_loop = 1; break;
                            } else {
                                
                                char k = (char)(ev.keycode & 0xFF);
                                write(in_pipe[1], &k, 1);
                            }
                        }
                    }
                    if (break_loop) break; 
                    
                    int avail = read(out_pipe[0], buf, 0);
                    if (avail > 0) {
                        int to_read = sizeof(buf) - 1; if (avail < to_read) to_read = avail;
                        bytes = read(out_pipe[0], buf, to_read);
                        if (bytes > 0) { buf[bytes] = '\0'; term_print(buf, 0); render_terminal(win); }
                    } else if (avail < 0) active_child_pid = 0; 
                    else yield(); 
                }
                close(out_pipe[0]); close(in_pipe[1]);
                waitpid(main_pid);
            }
cleanup:
            ;
        } else {
            term_print("gterm: command not found\n", C_ERROR);
        }
    }

    input_len = 0; input_pos = 0; hist_idx = history_count; scroll_offset = 0;
    draw_prompt();
}

int main() {
    term_grid = malloc(MAX_LINES * MAX_COLS * sizeof(TermChar));
    if (!term_grid) return 1;

    gui_window_t *win = gui_create_window("Terminal", 80 * CHAR_W + 12, 24 * CHAR_H);
    if (!win) return 1;

    gui_set_resizable(win, 1);
    term_cols = (win->w - 12) / CHAR_W;
    term_rows = win->h / CHAR_H;

    shm_get(1001, 8);
    int *sz = shm_map(1001);
    if (sz) { sz[0] = term_cols; sz[1] = term_rows; }

    rehash_path();
    term_clear();
    draw_prompt();
    render_terminal(win);

    int running = 1;
    GUI_Event ev;

    while (running && !win->closed) {
        int redraw = 0;
        while (gui_poll_event(win, &ev)) {
            if (ev.type == GUI_EV_CLOSE) running = 0;
            else if (ev.type == GUI_EV_RESIZE) {
                gui_resize_buffer(win, ev.x, ev.y);
                term_cols = (win->w - 12) / CHAR_W; term_rows = win->h / CHAR_H; 
                
                shm_get(1001, 8);
                int *sz = shm_map(1001);
                if (sz) { sz[0] = term_cols; sz[1] = term_rows; }
                
                redraw = 1;
            }
            else if (ev.type == GUI_EV_MOUSE_SCROLL) {
                scroll_offset += ev.keycode * 3;
                int max_scroll = max_drawn_y - term_rows + 1;
                if (scroll_offset > max_scroll) scroll_offset = max_scroll;
                if (scroll_offset < 0) scroll_offset = 0; redraw = 1;
            }
            else if (ev.type == GUI_EV_MOUSE_MOVE || ev.type == GUI_EV_MOUSE_CLICK) {
                if (ev.button & 1) {
                    if (ev.x > win->w - 15) is_dragging_sb = 1;
                    if (is_dragging_sb) {
                        int max_scroll = max_drawn_y - term_rows + 1;
                        if (max_scroll > 0) {
                            scroll_offset = max_scroll - ((ev.y * max_scroll) / win->h);
                            if (scroll_offset > max_scroll) scroll_offset = max_scroll;
                            if (scroll_offset < 0) scroll_offset = 0; redraw = 1;
                        }
                    }
                } else is_dragging_sb = 0;
            }
            else if (ev.type == GUI_EV_KEY_PRESS) {
                char c = (char)(ev.keycode & 0xFF);
                uint8_t mods = (ev.keycode >> 24) & 0xFF;
                scroll_offset = 0; 
                
                if ((mods & 2) && (c == 'c' || c == 'C')) {
                    term_print("^C", C_ERROR);
                    input_buffer[0] = '\0'; input_len = 0; input_pos = 0;
                    draw_prompt(); redraw = 1;
                    continue;
                }

                if (c == '\n' || c == '\r') { execute_command(win); redraw = 1; } 
                else if (c == '\t') {
                    char* cmd_sug = get_suggestion(input_buffer);
                    if (cmd_sug && input_pos == input_len) { 
                        strcpy(input_buffer, cmd_sug); 
                        input_len = strlen(input_buffer); input_pos = input_len; 
                    }
                    update_input_line(); redraw = 1;
                }
                else if (ev.keycode == KEY_UP) {
                    if (hist_idx > 0) { 
                        hist_idx--; strcpy(input_buffer, history[hist_idx]); 
                        input_len = strlen(input_buffer); input_pos = input_len; 
                        update_input_line(); redraw = 1; 
                    } 
                } 
                else if (ev.keycode == KEY_DOWN) { 
                    if (hist_idx < history_count) { 
                        hist_idx++; 
                        if (hist_idx == history_count) { input_buffer[0] = '\0'; input_len = 0; input_pos = 0; } 
                        else { strcpy(input_buffer, history[hist_idx]); input_len = strlen(input_buffer); input_pos = input_len; } 
                        update_input_line(); redraw = 1; 
                    } 
                }
                else if (ev.keycode == KEY_LEFT) { if (input_pos > 0) { input_pos--; update_input_line(); redraw = 1; } }
                else if (ev.keycode == KEY_RIGHT) { if (input_pos < input_len) { input_pos++; update_input_line(); redraw = 1; } }
                else if (c == '\b') {
                    if (input_pos > 0) { 
                        for (int i = input_pos; i <= input_len; i++) input_buffer[i - 1] = input_buffer[i]; 
                        input_pos--; input_len--; update_input_line(); redraw = 1; 
                    } 
                } 
                else if (c >= 32 && c <= 126 && input_len < 254) {
                    for (int i = input_len; i >= input_pos; i--) input_buffer[i + 1] = input_buffer[i]; 
                    input_buffer[input_pos] = c; input_pos++; input_len++; 
                    update_input_line(); redraw = 1; 
                }
            }
        }

        if (redraw) render_terminal(win);
        yield();
    }

    shm_get(1001, 0); 
    free(term_grid);
    gui_destroy_window(win);
    return 0;
}
