#include <string.h>
#include <oslib.h>
#include <stddef.h>

void print_header(int header_bg_color, int header_text_color, char *title){
    int len = 0;
    while(title[len] != '\0') len++;

    int padding = (80 - len) / 2;

    set_background_color(header_bg_color);
    set_text_color(header_text_color);

    for(int i = 0; i < padding; i++){
        print(" ");
    }
    print(title);
    for(int i = 0; i <= padding; i++){
        print(" ");
    }

    set_background_color(VGA_COLOR_BLACK);
    set_text_color(VGA_COLOR_LIGHT_GREY);
}

void print_footer(int footer_bg_color, int footer_text_color, char *text) {
    int len = 0;
    while(text[len] != '\0') len++;

    set_cursor_position(0, 25);
    
    set_background_color(footer_bg_color);
    set_text_color(footer_text_color);
    
    print(text);

    for(int i = 0; i < 79 - len; i++) print(" ");
    
    set_background_color(VGA_COLOR_BLACK);
    set_text_color(VGA_COLOR_LIGHT_GREY);
}

void strcpy(char *dst, char *src){
    int i = 0;
    while (src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

char *strcat(char *dest, const char *src) {
    char *original_dest = dest;
    
    while (*dest != '\0') {
        dest++;
    }
    
    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    
    *dest = '\0';
    
    return original_dest;
}

void itoa(int n, char* buffer, int base) {
    int i = 0;
    if (n == 0) { buffer[0] = '0'; buffer[1] = 0; return; }
    while (n != 0) {
        int rem = n % base;
        buffer[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n = n / base;
    }
    buffer[i] = 0;
    for(int j = 0; j < i / 2; j++) {
        char tmp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }
}

//псведо графика
void draw_text_box_ex(char* lines[], char* title, 
                      uint8_t padding_top, uint8_t padding_bottom,
                      uint8_t padding_left, uint8_t padding_right,
                      uint8_t border_color, uint8_t text_color, uint8_t title_color,
                      uint8_t centered) {
    
    uint16_t max_width = 0;
    uint16_t line_count = 0;
    
    for (uint16_t i = 0; lines[i] != NULL; i++) {
        uint16_t len = 0;
        char* ptr = lines[i];
        while (*ptr != '\0') { len++; ptr++; }
        if (len > max_width) max_width = len;
        line_count++;
    }
    
    uint16_t title_len = 0;
    if (title != NULL) {
        char* ptr = title;
        while (*ptr != '\0') { title_len++; ptr++; }
        uint16_t needed_width = title_len + 4;
        if (needed_width > max_width) max_width = needed_width;
    }
    
    uint16_t inner_width = max_width + padding_left + padding_right;
    uint16_t box_width = inner_width + 2;
    
    uint8_t old_color = get_current_color();

    unsigned int start_x = 0;
    unsigned int start_y = 0;
    unsigned int temp_x;

    get_cursor_xy(&temp_x, &start_y);
    
    if (centered) {
        if (box_width < 80) {
            start_x = (80 - box_width) / 2;
        } else {
            start_x = 0;
        }
    } else {
        start_x = temp_x;
    }

    uint16_t current_row_offset = 0;
    set_cursor_position(start_x, start_y + current_row_offset);
    set_current_color(border_color);
    print_char(201);
    
    if (title != NULL) {
        uint8_t left_parts = 1;
        for (uint8_t i = 0; i < left_parts; i++) print_char(205);
        
        set_current_color(title_color);
        print_char(' ');
        print(title);
        print_char(' ');
        set_current_color(border_color);
        
        uint16_t used = 2 + left_parts + title_len + 2;
        uint16_t right_parts = (box_width > used) ? (box_width - used) : 0;
        for (uint16_t i = 0; i < right_parts; i++) print_char(205);
    } else {
        for (uint16_t i = 0; i < box_width - 2; i++) print_char(205);
    }
    print_char(187);
    current_row_offset++;

    for (uint8_t row = 0; row < padding_top; row++) {
        set_cursor_position(start_x, start_y + current_row_offset);
        print_char(186);
        for (uint16_t i = 0; i < box_width - 2; i++) print_char(' ');
        print_char(186);
        current_row_offset++;
    }
    
    set_current_color(text_color);
    for (uint16_t line_idx = 0; line_idx < line_count; line_idx++) {
        set_cursor_position(start_x, start_y + current_row_offset);
        
        set_current_color(border_color);
        print_char(186);
        set_current_color(text_color);
        
        char* line = lines[line_idx];
        uint16_t line_len = 0;
        char* ptr = line;
        while (*ptr != '\0') { line_len++; ptr++; }
        
        if (centered) {
            uint16_t total_spaces = max_width - line_len;
            uint16_t spaces_left = (total_spaces / 2) + padding_left;
            uint16_t spaces_right = (total_spaces - (total_spaces / 2)) + padding_right;
            
            for (uint16_t i = 0; i < spaces_left; i++) print_char(' ');
            print(line);
            for (uint16_t i = 0; i < spaces_right; i++) print_char(' ');
        } else {
            for (uint8_t i = 0; i < padding_left; i++) print_char(' ');
            print(line);
            uint16_t spaces_needed = max_width - line_len + padding_right;
            for (uint16_t i = 0; i < spaces_needed; i++) print_char(' ');
        }
        
        set_current_color(border_color);
        print_char(186);
        current_row_offset++;
    }
    
    set_current_color(border_color);
    for (uint8_t row = 0; row < padding_bottom; row++) {
        set_cursor_position(start_x, start_y + current_row_offset);
        print_char(186);
        for (uint16_t i = 0; i < box_width - 2; i++) print_char(' ');
        print_char(186);
        current_row_offset++;
    }
    
    set_cursor_position(start_x, start_y + current_row_offset);
    print_char(200);
    for (uint16_t i = 0; i < box_width - 2; i++) print_char(205);
    print_char(188);
    print("\n");
    
    set_current_color(old_color);
}

void draw_text_box(char* lines[], char* title, uint8_t padding, 
                   uint8_t border_color, uint8_t text_color, uint8_t title_color,
                   uint8_t centered) {
    draw_text_box_ex(lines, title, padding, padding, padding, padding, 
                    border_color, text_color, title_color, centered);
}

void draw_simple_box(char* lines[], char* title, uint8_t centered) {
    draw_text_box(lines, title, 1, VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY, 
                  VGA_COLOR_YELLOW, centered);
}