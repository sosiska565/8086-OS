/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/ww.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

#define MAX_LINES 1000
#define MAX_LINE_LEN 256

#define KEY_ESC 27
#define KEY_UP 17
#define KEY_DOWN 18
#define KEY_LEFT 19
#define KEY_RIGHT 20
#define KEY_BACKSPACE 8
#define KEY_ENTER '\n'

enum Mode { NORMAL, INSERT, VISUAL, COMMAND };

char* lines[MAX_LINES];
int num_lines = 1;

int cx = 0, cy = 0;       
int scroll_y = 0;         
int screen_cols = 80, screen_rows = 25;
enum Mode current_mode = NORMAL;
char filename[128] = "";

char command_buf[64] = "";
int command_pos = 0;

int vis_start_x = 0;
int vis_start_y = 0;

const char* keywords[] = {
    "int", "char", "void", "return", "if", "else", 
    "while", "for", "struct", "include", "define", "static", NULL
};

int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

void init_editor() {
    for (int i = 0; i < MAX_LINES; i++) {
        lines[i] = malloc(MAX_LINE_LEN);
        memset(lines[i], 0, MAX_LINE_LEN);
    }
}

void load_file(const char* fname) {
    strcpy(filename, fname);
    int sz = get_file_size(filename);
    if (sz > 0) {
        char* buf = malloc(sz + 1);
        read_file(filename, (uint8_t*)buf);
        buf[sz] = '\0';
        
        int line_idx = 0;
        int col_idx = 0;
        for (int i = 0; i < sz; i++) {
            if (buf[i] == '\n') {
                lines[line_idx][col_idx] = '\0';
                line_idx++;
                col_idx = 0;
                if (line_idx >= MAX_LINES) break;
            } else if (buf[i] != '\r') {
                if (col_idx < MAX_LINE_LEN - 1) {
                    lines[line_idx][col_idx++] = buf[i];
                }
            }
        }
        lines[line_idx][col_idx] = '\0';
        num_lines = line_idx + 1;
        free(buf);
    }
}

void save_file() {
    if (filename[0] == '\0') return;
    
    int total_len = 0;
    for (int i = 0; i < num_lines; i++) {
        total_len += strlen(lines[i]) + 1; 
    }
    
    char* buf = malloc(total_len + 1);
    int pos = 0;
    for (int i = 0; i < num_lines; i++) {
        int len = strlen(lines[i]);
        for (int c = 0; c < len; c++) buf[pos++] = lines[i][c];
        buf[pos++] = '\n';
    }
    buf[pos] = '\0';
    
    write_file(filename, (uint8_t*)buf, pos);
    free(buf);
}

void draw_syntax_char(char c, int* color_state) {
    if (*color_state == 0 && c == '"') *color_state = 1;
    
    if (*color_state == 1) set_color(10, 0); 
    else if (*color_state == 2) set_color(8, 0); 
    else set_color(7, 0); 
    
    print_char(c);
    
    if (*color_state == 1 && c == '"') *color_state = 0;
}

void print_line_with_syntax(int y) {
    char* str = lines[y];
    int len = strlen(str);
    int color_state = 0; 
    
    for (int i = 0; i < len; i++) {
        if (color_state == 0 && str[i] == '/' && str[i+1] == '/') color_state = 2;
        
        int is_selected = 0;
        if (current_mode == VISUAL) {
            int min_y = (y < vis_start_y) ? y : vis_start_y;
            int max_y = (y > vis_start_y) ? y : vis_start_y;
            if (y > min_y && y < max_y) is_selected = 1;
            else if (y == vis_start_y && y == cy) {
                int min_x = (cx < vis_start_x) ? cx : vis_start_x;
                int max_x = (cx > vis_start_x) ? cx : vis_start_x;
                if (i >= min_x && i <= max_x) is_selected = 1;
            }
            else if (y == min_y && y != max_y && i >= ((y == vis_start_y) ? vis_start_x : cx)) is_selected = 1;
            else if (y == max_y && y != min_y && i <= ((y == cy) ? cx : vis_start_x)) is_selected = 1;
        }

        if (is_selected) set_color(15, 1); 
        else if (color_state != 0) {
            draw_syntax_char(str[i], &color_state);
            continue;
        } 
        else {
            int is_kw = 0;
            if (is_alpha(str[i]) && (i == 0 || !is_alpha(str[i-1]))) {
                for (int k = 0; keywords[k] != NULL; k++) {
                    int kw_len = strlen(keywords[k]);
                    if (strncmp(&str[i], keywords[k], kw_len) == 0 && !is_alpha(str[i + kw_len])) {
                        is_kw = 1;
                        set_color(11, 0); 
                        for (int j = 0; j < kw_len; j++) print_char(str[i++]);
                        i--; 
                        break;
                    }
                }
            }
            if (!is_kw) {
                set_color(7, 0);
                print_char(str[i]);
            }
            continue;
        }
        
        print_char(str[i]);
    }
    set_color(7, 0);
}

void draw_screen() {
    clear_screen();
    for (int i = 0; i < screen_rows - 1; i++) {
        set_cursor(0, i);
        int file_y = scroll_y + i;
        
        if (file_y < num_lines) {
            set_color(14, 0);
            printf("%d ", file_y + 1);
            if (file_y + 1 < 10) printf("   ");
            else if (file_y + 1 < 100) printf("  ");
            else printf(" ");
            
            print_line_with_syntax(file_y);
        } else {
            set_color(8, 0);
            printf("~");
        }
    }

    set_cursor(0, screen_rows - 1);
    set_color(0, 7); 
    for(int i=0; i<screen_cols; i++) print_char(' '); 
    set_cursor(0, screen_rows - 1);
    
    if (current_mode == NORMAL) printf(" [NORMAL]  %s   Ln %d, Col %d ", filename, cy + 1, cx + 1);
    else if (current_mode == INSERT) printf(" [INSERT]  %s ", filename);
    else if (current_mode == VISUAL) printf(" [VISUAL]  %s ", filename);
    else if (current_mode == COMMAND) printf(" :%s", command_buf);
    
    set_color(7, 0); 
    
    if (current_mode == COMMAND) {
        set_cursor(2 + command_pos, screen_rows - 1);
    } else {
        set_cursor(5 + cx, cy - scroll_y);
    }
}

void check_scroll() {
    if (cy < scroll_y) scroll_y = cy;
    if (cy >= scroll_y + screen_rows - 1) scroll_y = cy - screen_rows + 2;
    
    int len = strlen(lines[cy]);
    if (current_mode == NORMAL && cx > len - 1) cx = (len > 0) ? len - 1 : 0;
    if (current_mode == INSERT && cx > len) cx = len;
}

void do_insert_char(char c) {
    int len = strlen(lines[cy]);
    if (len >= MAX_LINE_LEN - 1) return;
    for (int i = len; i >= cx; i--) lines[cy][i + 1] = lines[cy][i];
    lines[cy][cx] = c;
    cx++;
}

void do_backspace() {
    if (cx > 0) {
        int len = strlen(lines[cy]);
        for (int i = cx - 1; i < len; i++) lines[cy][i] = lines[cy][i + 1];
        cx--;
    } else if (cy > 0) {
        int prev_len = strlen(lines[cy - 1]);
        int cur_len = strlen(lines[cy]);
        if (prev_len + cur_len < MAX_LINE_LEN) {
            strcat(lines[cy - 1], lines[cy]);
            for (int i = cy; i < num_lines - 1; i++) strcpy(lines[i], lines[i + 1]);
            num_lines--;
            cy--;
            cx = prev_len;
        }
    }
}

void do_enter() {
    if (num_lines >= MAX_LINES) return;
    for (int i = num_lines; i > cy; i--) strcpy(lines[i], lines[i - 1]);
    num_lines++;
    strcpy(lines[cy + 1], &lines[cy][cx]);
    lines[cy][cx] = '\0';
    cy++;
    cx = 0;
}

void process_normal(unsigned int c) {
    if (c == 'i') current_mode = INSERT;
    else if (c == 'v') { current_mode = VISUAL; vis_start_x = cx; vis_start_y = cy; }
    else if (c == ':') { current_mode = COMMAND; command_buf[0] = '\0'; command_pos = 0; }
    else if (c == 'h' || c == KEY_LEFT) { if (cx > 0) cx--; }
    else if (c == 'l' || c == KEY_RIGHT) { if (cx < strlen(lines[cy]) - 1) cx++; }
    else if (c == 'j' || c == KEY_DOWN) { if (cy < num_lines - 1) cy++; }
    else if (c == 'k' || c == KEY_UP) { if (cy > 0) cy--; }
    else if (c == 'o') { 
        cy++; cx = 0; do_enter(); cy--; 
        current_mode = INSERT; 
    }
    else if (c == 'x') {
        int len = strlen(lines[cy]);
        if (len > 0 && cx < len) {
            for (int i = cx; i < len; i++) lines[cy][i] = lines[cy][i+1];
            if (cx >= len - 1 && cx > 0) cx--;
        }
    }
}

int main(int argc, char** argv) {
    if (argc > 1) strcpy(filename, argv[1]);
    else strcpy(filename, "untitled.c");
    
    get_term_size(&screen_cols, &screen_rows);
    init_editor();
    load_file(filename);

    while (1) {
        check_scroll();
        draw_screen();

        unsigned int c = getc();

        if (current_mode == NORMAL) {
            process_normal(c);
        } 
        else if (current_mode == INSERT) {
            if (c == KEY_ESC) { current_mode = NORMAL; if(cx>0) cx--; }
            else if (c == KEY_ENTER) do_enter();
            else if (c == KEY_BACKSPACE) do_backspace();
            else if (c == KEY_LEFT) { if (cx > 0) cx--; }
            else if (c == KEY_RIGHT) { if (cx < strlen(lines[cy])) cx++; }
            else if (c == KEY_UP) { if (cy > 0) cy--; }
            else if (c == KEY_DOWN) { if (cy < num_lines - 1) cy++; }
            else if (c >= 32 && c <= 126) do_insert_char((char)c);
        }
        else if (current_mode == VISUAL) {
            if (c == KEY_ESC) current_mode = NORMAL;
            else if (c == 'h' || c == KEY_LEFT) { if (cx > 0) cx--; }
            else if (c == 'l' || c == KEY_RIGHT) { if (cx < strlen(lines[cy])) cx++; }
            else if (c == 'j' || c == KEY_DOWN) { if (cy < num_lines - 1) cy++; }
            else if (c == 'k' || c == KEY_UP) { if (cy > 0) cy--; }
            else if (c == 'x' || c == 'd') {
                current_mode = NORMAL; 
            }
        }
        else if (current_mode == COMMAND) {
            if (c == KEY_ESC) current_mode = NORMAL;
            else if (c == KEY_BACKSPACE && command_pos > 0) {
                command_pos--;
                command_buf[command_pos] = '\0';
            }
            else if (c == KEY_ENTER) {
                if (strcmp(command_buf, "w") == 0) save_file();
                else if (strcmp(command_buf, "q") == 0) break;
                else if (strcmp(command_buf, "wq") == 0) { save_file(); break; }
                current_mode = NORMAL;
            }
            else if (c >= 32 && c <= 126 && command_pos < 60) {
                command_buf[command_pos++] = (char)c;
                command_buf[command_pos] = '\0';
            }
        }
    }

    clear_screen();
    return 0;
}
