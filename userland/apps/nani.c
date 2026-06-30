/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/nani.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

#define MAX_LINES 2000
#define MAX_LINE_LEN 256
#define MAX_COLS 400
#define MAX_ROWS 150

char *lines[MAX_LINES];
int num_lines = 0;

int cx = 0, cy = 0;
int scroll_x = 0, scroll_y = 0;
int term_cols = 80, term_rows = 24;

static int prev_term_cols = -1;
static int prev_term_rows = -1;

int is_modified = 0;
char filename[128] = "untitled.txt";
char status_msg[128] = "";

char front_char[MAX_ROWS][MAX_COLS];
uint8_t front_col[MAX_ROWS][MAX_COLS];

char back_char[MAX_ROWS][MAX_COLS];
uint8_t back_col[MAX_ROWS][MAX_COLS];

void init_editor() {
    for (int i = 0; i < MAX_LINES; i++) lines[i] = NULL;
    lines[0] = malloc(MAX_LINE_LEN);
    lines[0][0] = '\0';
    num_lines = 1;

    for (int y = 0; y < MAX_ROWS; y++) {
        for (int x = 0; x < MAX_COLS; x++) {
            front_char[y][x] = 0;
            front_col[y][x] = 255;
        }
    }
}

void load_file(const char *path) {
    strcpy(filename, path);
    int sz = get_file_size(filename);
    if (sz <= 0) { strcpy(status_msg, "New file"); return; }

    uint8_t *buf = malloc(sz + 1);
    if (!buf) { strcpy(status_msg, "Out of memory!"); return; }
    read_file(filename, buf); buf[sz] = '\0';

    num_lines = 0; int line_idx = 0;
    lines[0] = malloc(MAX_LINE_LEN);

    for (int i = 0; i < sz; i++) {
        if (buf[i] == '\n') {
            lines[num_lines][line_idx] = '\0';
            num_lines++;
            if (num_lines >= MAX_LINES) break;
            lines[num_lines] = malloc(MAX_LINE_LEN);
            line_idx = 0;
        } else if (buf[i] != '\r' && line_idx < MAX_LINE_LEN - 1) {
            lines[num_lines][line_idx++] = buf[i];
        }
    }
    if (num_lines < MAX_LINES) { lines[num_lines][line_idx] = '\0'; num_lines++; }
    free(buf); sprintf(status_msg, "Loaded %d lines", num_lines);
}

void save_file() {
    int total_size = 0;
    for (int i = 0; i < num_lines; i++) total_size += strlen(lines[i]) + 1;

    uint8_t *buf = malloc(total_size + 1);
    if (!buf) { strcpy(status_msg, "Out of memory!"); return; }

    int pos = 0;
    for (int i = 0; i < num_lines; i++) {
        int len = strlen(lines[i]);
        memcpy(buf + pos, lines[i], len);
        pos += len; buf[pos++] = '\n';
    }
    buf[pos] = '\0';

    if (write_file(filename, buf, pos) > 0) {
        is_modified = 0; strcpy(status_msg, "File saved!");
    } else {
        strcpy(status_msg, "Failed to save!");
    }
    free(buf);
}

void put_char(int x, int y, char c, uint8_t col) {
    if (x >= 0 && x < term_cols && y >= 0 && y < term_rows) {
        back_char[y][x] = c; back_col[y][x] = col;
    }
}

void put_str(int x, int y, const char* str, uint8_t col) {
    while (*str) put_char(x++, y, *str++, col);
}

void draw_screen() {
    shm_get(1001, 8); 
    int *gterm_sz = shm_map(1001); 

    if (gterm_sz && gterm_sz[0] > 0 && gterm_sz[1] > 0) {
        term_cols = gterm_sz[0]; term_rows = gterm_sz[1];
    } else {
        get_term_size(&term_cols, &term_rows);
    }
    
    if (term_cols > MAX_COLS) term_cols = MAX_COLS;
    if (term_rows > MAX_ROWS) term_rows = MAX_ROWS;

    if (term_cols != prev_term_cols || term_rows != prev_term_rows) {
        for (int y = 0; y < MAX_ROWS; y++) {
            for (int x = 0; x < MAX_COLS; x++) {
                front_char[y][x] = 0;
                front_col[y][x] = 255;
            }
        }
        printf("\033[2J"); 
        prev_term_cols = term_cols;
        prev_term_rows = term_rows;
    }

    if (cy < scroll_y) scroll_y = cy;
    if (cy >= scroll_y + term_rows - 2) scroll_y = cy - (term_rows - 2) + 1;
    if (cx < scroll_x) scroll_x = cx;
    if (cx >= scroll_x + term_cols) scroll_x = cx - term_cols + 1;

    for (int y = 0; y < term_rows; y++) {
        for (int x = 0; x < term_cols; x++) {
            back_char[y][x] = ' '; back_col[y][x] = 0;
        }
    }

    char header[256];
    sprintf(header, "  Nani Editor - %s %s", filename, is_modified ? "[Modified]" : "");
    put_str(0, 0, header, 1); 
    for (int i = strlen(header); i < term_cols; i++) put_char(i, 0, ' ', 1);

    for (int r = 0; r < term_rows - 2; r++) {
        int line_idx = scroll_y + r;
        if (line_idx < num_lines) {
            char *line = lines[line_idx]; int len = strlen(line);
            for (int c = 0; c < term_cols; c++) {
                int char_idx = scroll_x + c;
                if (char_idx < len) put_char(c, r + 1, line[char_idx], 0);
            }
        } else {
            put_char(0, r + 1, '~', 0);
        }
    }

    char footer[256];
    sprintf(footer, "  F1: Save    F2: Exit    | %s", status_msg);
    put_str(0, term_rows - 1, footer, 1);
    for (int i = strlen(footer); i < term_cols; i++) put_char(i, term_rows - 1, ' ', 1);
    status_msg[0] = '\0';

    int screen_x = cx - scroll_x;
    int screen_y = cy - scroll_y + 1;
    if (screen_x >= 0 && screen_x < term_cols && screen_y >= 1 && screen_y < term_rows - 1) {
        back_col[screen_y][screen_x] = 1; 
    }
}

void render_screen() {
    char out_buf[8192]; int out_pos = 0;
    
    #define FLUSH_OUT() do { if (out_pos > 0) { write(1, out_buf, out_pos); out_pos = 0; } } while(0)
    #define OUT_C(c) do { out_buf[out_pos++] = (c); if (out_pos >= 8000) FLUSH_OUT(); } while(0)
    #define OUT_STR(s) do { const char* _s = (s); while(*_s) OUT_C(*_s++); } while(0)

    int cur_y = -1, cur_x = -1;
    uint8_t current_color = 255;

    for (int y = 0; y < term_rows; y++) {
        for (int x = 0; x < term_cols; x++) {
            if (y == term_rows - 1 && x == term_cols - 1) continue; 

            if (front_char[y][x] != back_char[y][x] || front_col[y][x] != back_col[y][x]) {
                if (cur_y != y || cur_x != x) {
                    char ansi[32]; sprintf(ansi, "\033[%d;%dH", y + 1, x + 1);
                    OUT_STR(ansi);
                }
                
                if (current_color != back_col[y][x]) {
                    current_color = back_col[y][x];
                    if (current_color == 1) OUT_STR("\033[30m\033[47m"); 
                    else OUT_STR("\033[0m"); 
                }
                
                OUT_C(back_char[y][x]);
                cur_y = y; cur_x = x + 1;
                
                front_char[y][x] = back_char[y][x];
                front_col[y][x] = back_col[y][x];
            }
        }
    }

    OUT_STR("\033[0m"); 
    FLUSH_OUT();
}

void insert_char(char c) {
    char *line = lines[cy]; int len = strlen(line);
    if (len < MAX_LINE_LEN - 1) {
        memmove(&line[cx + 1], &line[cx], len - cx + 1);
        line[cx] = c; cx++; is_modified = 1;
    }
}

void handle_enter() {
    if (num_lines >= MAX_LINES) return;
    for (int i = num_lines; i > cy + 1; i--) lines[i] = lines[i - 1];
    lines[cy + 1] = malloc(MAX_LINE_LEN);
    strcpy(lines[cy + 1], &lines[cy][cx]);
    lines[cy][cx] = '\0';
    cy++; cx = 0; num_lines++; is_modified = 1;
}

void handle_backspace() {
    if (cx > 0) {
        char *line = lines[cy]; int len = strlen(line);
        memmove(&line[cx - 1], &line[cx], len - cx + 1);
        cx--; is_modified = 1;
    } else if (cy > 0) {
        int prev_len = strlen(lines[cy - 1]);
        if (prev_len + strlen(lines[cy]) < MAX_LINE_LEN - 1) {
            strcat(lines[cy - 1], lines[cy]);
            free(lines[cy]);
            for (int i = cy; i < num_lines - 1; i++) lines[i] = lines[i + 1];
            num_lines--; cy--; cx = prev_len; is_modified = 1;
        }
    }
}

int main(int argc, char **argv) {
    init_editor();
    if (argc > 1) load_file(argv[1]);

    printf("\033[2J\033[1;1H\033[?25l"); 

    int running = 1;
    while (running) {
        draw_screen();
        render_screen();
        
        int c = getc();
        if (c < 0) break;
        if (c == 0) continue; 

        if (c == KEY_UP) { if (cy > 0) cy--; int l = strlen(lines[cy]); if (cx > l) cx = l; } 
        else if (c == KEY_DOWN) { if (cy < num_lines - 1) cy++; int l = strlen(lines[cy]); if (cx > l) cx = l; } 
        else if (c == KEY_LEFT) { if (cx > 0) cx--; else if (cy > 0) { cy--; cx = strlen(lines[cy]); } } 
        else if (c == KEY_RIGHT) { int l = strlen(lines[cy]); if (cx < l) cx++; else if (cy < num_lines - 1) { cy++; cx = 0; } } 
        else if (c == KEY_F1) save_file();
        else if (c == KEY_F2) running = 0;
        else if (c == KEY_BACKSPACE || c == '\b') handle_backspace();
        else if (c == KEY_ENTER || c == '\n' || c == '\r') handle_enter();
        else if (c == KEY_TAB || c == '\t') { for (int i = 0; i < 4; i++) insert_char(' '); }
        else if (c >= 32 && c <= 126) insert_char((char)c);
    }

    printf("\033[2J\033[%d;1H\033[0m\033[?25h", term_rows);
    for (int i = 0; i < num_lines; i++) free(lines[i]);
    return 0;
}
