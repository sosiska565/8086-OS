#include <oslib.h>
#include <string.h>
#include <stdint.h>

#define SCREEN_WIDTH 79 
#define SCREEN_HEIGHT 25
#define VIEW_HEIGHT (SCREEN_HEIGHT - 2) 

#define MAX_FILE_SIZE 40000 

uint8_t *file_data;
int file_size;
int *line_offsets;
int total_lines = 0;

int scroll_offset = 0;
int cursor_x = 0;
int cursor_y = 0;

void build_line_index(void) {
    if (line_offsets) free(line_offsets);
    line_offsets = (int*)malloc((MAX_FILE_SIZE) * 4);
    
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
    if (file_size >= MAX_FILE_SIZE - 1) return;

    int idx = get_buffer_index(cursor_x, cursor_y);

    for (int i = file_size; i > idx; i--) {
        file_data[i] = file_data[i-1];
    }

    file_data[idx] = c;
    file_size++;
    
    if (c == '\n') {
        cursor_y++;
        cursor_x = 0;
    } else {
        cursor_x++;
    }
    build_line_index();
}

void delete_char_backspace() {
    if (cursor_x == 0 && cursor_y == 0) return;

    int idx = get_buffer_index(cursor_x, cursor_y) - 1;
    
    if (file_data[idx] == '\n') {
        cursor_y--;
        cursor_x = get_line_length(cursor_y);
    } else {
        cursor_x--;
    }

    for (int i = idx; i < file_size - 1; i++) {
        file_data[i] = file_data[i+1];
    }
    
    file_size--;
    build_line_index();
}

void draw_interface(char *filename) {
    set_cursor_position(0, 0);
    print_header(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_YELLOW, filename);
    print_footer(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_YELLOW, "[F2] Save  [Esc] Quit  Typing: Enabled");
}

void draw_content(void) {
    for (int i = 0; i < VIEW_HEIGHT; i++) {
        int screen_y = i + 1;
        set_cursor_position(0, screen_y);
        
        int current_line_idx = scroll_offset + i;
        
        for(int k=0; k<SCREEN_WIDTH; k++) print_char(' ');
        set_cursor_position(0, screen_y);

        if (current_line_idx >= total_lines) {
            print_char_colored('~', VGA_COLOR_BLUE);
            continue;
        }

        int start_pos = line_offsets[current_line_idx];
        int len = get_line_length(current_line_idx);

        for (int j = 0; j < len && j < SCREEN_WIDTH; j++) {
            char c = file_data[start_pos + j];
            if (c >= 32) print_char(c);
            else print_char(' ');
        }
    }
}

void update_cursor_logic() {
    if (cursor_y >= scroll_offset + VIEW_HEIGHT) {
        scroll_offset = cursor_y - VIEW_HEIGHT + 1;
    }
    if (cursor_y < scroll_offset) {
        scroll_offset = cursor_y;
    }

    int screen_x = cursor_x;
    int screen_y = cursor_y - scroll_offset + 1;

    if (screen_x > SCREEN_WIDTH - 1) screen_x = SCREEN_WIDTH - 1;

    set_cursor_position(screen_x, screen_y);
}

void main(int argc, char **argv){
    if(argc == 1){
        print("Usage: nani <filename>\n");
        return;
    }

    file_data = (uint8_t*)malloc(MAX_FILE_SIZE);
    for(int i=0; i<MAX_FILE_SIZE; i++) file_data[i] = 0;

    int size = get_file_size(argv[1]);
    if(size > 0) {
        read_file(argv[1], file_data);
        file_size = size;
    } else {
        file_size = 0;
    }

    build_line_index();
    cls();

    while(1){
        draw_interface(argv[1]);
        draw_content();
        update_cursor_logic();

        uint8_t scancode = get_scanecode();

        if(scancode == 0x01) break;

        if(scancode == 0x3C) {
            write_file(argv[1], file_data, file_size);
            
            set_cursor_position(60, 0);
            print_colored("SAVED!", VGA_COLOR_GREEN);
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
            if (c != 0) {
                insert_char(c);
            }
        }
    }

    cls();
    free(file_data);
    free(line_offsets);
}