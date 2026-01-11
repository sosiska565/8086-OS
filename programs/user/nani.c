#include "oslib.h"
#include "strings.h"
#include <stdint.h>

#define FONT_W 8
#define FONT_H 8
#define INITIAL_CAPACITY 1024 

#define COL_BG_UI    0x000000AA
#define COL_FG_UI    0x00FFFF55
#define COL_BG_TEXT  0x00000000
#define COL_FG_TEXT  0x00AAAAAA
#define COL_CURSOR   0x00FFFFFF

int screen_width_chars = 80;  
int screen_height_chars = 25; 
int view_height = 23;

uint8_t *file_data;
int file_size;
int buffer_capacity;
int *line_offsets;
int total_lines = 0;
int line_index_capacity = 0;

int scroll_offset = 0;
int col_offset = 0;
int cursor_x = 0;
int cursor_y = 0;

void ensure_capacity(int required_size) {
    if (required_size >= buffer_capacity) {
        int new_capacity = buffer_capacity * 2;
        if (new_capacity < required_size) new_capacity = required_size + 1024;
        uint8_t *new_data = (uint8_t*)malloc(new_capacity);
        if (new_data == 0) return; 
        for(int i = 0; i < file_size; i++) new_data[i] = file_data[i];
        if (file_data) free(file_data);
        file_data = new_data;
        buffer_capacity = new_capacity;
    }
}

void build_line_index(void) {
    int needed_cap = (file_size + 2) * 4;
    if (line_offsets == 0 || needed_cap > line_index_capacity) {
        if (line_offsets) free(line_offsets);
        line_index_capacity = needed_cap + 1024;
        line_offsets = (int*)malloc(line_index_capacity);
    }
    total_lines = 0;
    line_offsets[0] = 0;
    total_lines++;
    for (int i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            line_offsets[total_lines] = i + 1;
            total_lines++;
        }
    }
    line_offsets[total_lines] = file_size + 1;
}

int get_line_length(int line_idx) {
    if (line_idx >= total_lines) return 0;
    int start = line_offsets[line_idx];
    int end;
    if (line_idx + 1 < total_lines) end = line_offsets[line_idx + 1] - 1;
    else end = file_size;
    if (end > start && file_data[end-1] == '\n') end--;
    return end - start;
}

int get_buffer_index(int x, int y) {
    if (y >= total_lines) y = total_lines - 1;
    int len = get_line_length(y);
    if (x > len) x = len;
    return line_offsets[y] + x;
}

void insert_char(char c) {
    ensure_capacity(file_size + 1);
    int idx = get_buffer_index(cursor_x, cursor_y);
    for (int i = file_size; i > idx; i--) file_data[i] = file_data[i-1];
    file_data[idx] = c;
    file_size++;
    if (c == '\n') { cursor_y++; cursor_x = 0; } else { cursor_x++; }
    build_line_index();
}

void delete_char_backspace() {
    if (cursor_x == 0 && cursor_y == 0) return;
    int idx = get_buffer_index(cursor_x, cursor_y) - 1;
    if (file_data[idx] == '\n') { cursor_y--; cursor_x = get_line_length(cursor_y); } else { cursor_x--; }
    for (int i = idx; i < file_size - 1; i++) file_data[i] = file_data[i+1];
    file_size--;
    build_line_index();
}


void draw_interface(char *filename) {
    set_cursor_position(0, 0);
    set_background_color(COL_BG_UI);
    
    printf("%C NANI: %s", COL_FG_UI, filename);
    int current_len = 7 + strlen(filename);
    for(int i = current_len; i < screen_width_chars; i++) printf(" ");

    set_cursor_position(0, screen_height_chars - 2);
    
    char mem_str[32];
    itoa(buffer_capacity / 1024, mem_str, 10);
    
    printf("%C[F2] Save  [Esc] Quit  MEM: %sKB", COL_FG_UI, mem_str);
    int footer_len = 30 + strlen(mem_str); 
    for(int i = footer_len; i < screen_width_chars; i++) printf(" ");

    set_background_color(COL_BG_TEXT);
}

void draw_content(void) {
    for (int i = 0; i < view_height; i++) {
        int screen_y = i + 1;
        set_cursor_position(0, screen_y);
        
        set_background_color(COL_BG_TEXT);
        for(int k=0; k < screen_width_chars; k++) printf(" ");
        set_cursor_position(0, screen_y);

        int current_line_idx = scroll_offset + i;

        if (current_line_idx >= total_lines) {
            printf("%C~", COL_BG_UI);
            continue;
        }

        int start_pos = line_offsets[current_line_idx];
        int len = get_line_length(current_line_idx);

        int printed_chars = 0;
        for (int j = col_offset; j < len && printed_chars < screen_width_chars; j++) {
            char c = file_data[start_pos + j];
            if (c >= 32) printf("%C%c", COL_FG_TEXT, c);
            else printf(" ");
            printed_chars++;
        }
    }
}

void draw_cursor() {
    int screen_x = cursor_x - col_offset;
    int screen_y = cursor_y - scroll_offset + 1;

    if (screen_x < 0 || screen_x >= screen_width_chars) return;
    if (screen_y < 1 || screen_y >= screen_height_chars - 1) return;

    set_cursor_position(screen_x, screen_y);

    int idx = get_buffer_index(cursor_x, cursor_y);
    char c = ' ';
    if (idx < file_size) {
        c = file_data[idx];
        if (c == '\n' || c < 32) c = ' ';
    }

    set_background_color(0x00FFFFFF);
    printf("%C%c", COL_BG_TEXT, c);
    
    set_background_color(COL_BG_TEXT);
}

void update_scroll_logic() {
    if (cursor_y - scroll_offset >= view_height - 1) {
        scroll_offset = cursor_y - view_height + 2;
    }
    if (cursor_y < scroll_offset) {
        scroll_offset = cursor_y;
    }
    
    if (cursor_x - col_offset >= screen_width_chars) {
        col_offset = cursor_x - screen_width_chars + 1;
    }
    if (cursor_x < col_offset) {
        col_offset = cursor_x;
    }

    int screen_x = cursor_x - col_offset;
    int screen_y = cursor_y - scroll_offset + 1;

    set_cursor_position(screen_x, screen_y);
}

void main(int argc, char **argv){
    if(argc == 1){ printf("Usage: nani <filename>\n"); return; }

    int w_pixels = getScreenWidth();
    int h_pixels = getScreenHeight();
    if (w_pixels > 200) { 
        screen_width_chars = w_pixels / FONT_W;
        screen_height_chars = h_pixels / FONT_H;
    } else {
        screen_width_chars = 80;
        screen_height_chars = 25;
    }
    view_height = screen_height_chars - 2;

    int size = get_file_size(argv[1]);
    if(size > 0) {
        buffer_capacity = size + 1024;
        file_data = (uint8_t*)malloc(buffer_capacity);
        if(!file_data) { printf("Out of memory\n"); return; }
        read_file(argv[1], file_data); 
        file_size = size;
    } else {
        buffer_capacity = INITIAL_CAPACITY;
        file_data = (uint8_t*)malloc(buffer_capacity);
        if(!file_data) { printf("Out of memory\n"); return; }
        file_size = 0;
    }

    line_offsets = 0; line_index_capacity = 0;
    build_line_index();
    col_offset = 0;

    cls();

    while(1){
        update_scroll_logic();

        draw_content();

        draw_interface(argv[1]);
        
        draw_cursor();

        uint8_t scancode = get_scanecode();

        if(scancode == 0x01) break;

        else if(scancode == 0x3C) {
            write_file(argv[1], file_data, file_size);
            set_cursor_position(screen_width_chars - 10, 0);
            set_background_color(COL_BG_UI);
            printf("%CSAVED!", COL_FG_UI);
            set_background_color(COL_BG_TEXT);
             for(volatile int k=0; k<500000000; k++);
        }
        else if(scancode == 0x48) {
            if (cursor_y > 0) {
                cursor_y--;
                int len = get_line_length(cursor_y);
                if (cursor_x > len) cursor_x = len;
            }
        }
        else if(scancode == 0x50) {
            if (cursor_y < total_lines - 1) {
                cursor_y++;
                int len = get_line_length(cursor_y);
                if (cursor_x > len) cursor_x = len;
            }
        }
        else if(scancode == 0x4B) {
            if (cursor_x > 0) cursor_x--;
            else if (cursor_y > 0) { 
                cursor_y--;
                cursor_x = get_line_length(cursor_y);
            }
        }
        else if(scancode == 0x4D) {
            int len = get_line_length(cursor_y);
            if (cursor_x < len) cursor_x++;
            else if (cursor_y < total_lines - 1) {
                cursor_y++;
                cursor_x = 0;
            }
        }
        else if(scancode == 0x0E) {
            delete_char_backspace();
        }
        else if(scancode == 0x1C) {
            insert_char('\n');
        }
        else {
            char c = scancode_to_ascii(scancode);
            if (c != 0) insert_char(c);
        }
    }

    cls();
    set_cursor_position(0, 0);
    if(file_data) free(file_data);
    if(line_offsets) free(line_offsets);
}