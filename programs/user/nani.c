#include "oslib.h"
#include "string_lib.h"

#define MAX_TEXT_SIZE 32768

Window *win;
char text_buffer[MAX_TEXT_SIZE];
uint32_t char_colors[MAX_TEXT_SIZE];

int text_length = 0;
int cursor_index = 0;
int scroll_line = 0;
int scroll_col = 0; 
char filename[64];


int is_alpha(unsigned int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
int is_digit(unsigned int c) { return (c >= '0' && c <= '9'); }

int is_keyword(char *str, int len) {
    const char *kw[] = {"int", "void", "char", "if", "else", "while", "for", "return", "struct", "#include", "#define", "static"};
    for(int i = 0; i < 12; i++) {
        int kl = strlen((char*)kw[i]);
        if (kl == len) {
            int match = 1;
            for(int j = 0; j < len; j++) {
                if (str[j] != kw[i][j]) { match = 0; break; }
            }
            if (match) return 1;
        }
    }
    return 0;
}


void draw_text_absolute(int x, int y, char *str, uint32_t color) {
    while(*str) {
        text_struct ts;
        ts.x = x;
        ts.y = y;
        ts.color = color;
        window_draw_char(win, &ts, *str);
        x += 8;
        str++;
    }
}


void update_syntax() {
    int in_comment = 0;
    int in_string = 0;
    int in_char = 0;

    for (int i = 0; i < text_length; i++) {
        unsigned int c = text_buffer[i];
        char_colors[i] = VGA32_COLOR_WHITE; 

        
        if (in_comment) {
            char_colors[i] = VGA32_COLOR_DARK_GREY;
            if (c == '\n') in_comment = 0;
            continue;
        }
        if (in_string) {
            char_colors[i] = VGA32_COLOR_LIGHT_GREEN;
            if (c == '"' && (i == 0 || text_buffer[i-1] != '\\')) in_string = 0;
            continue;
        }
        if (in_char) {
            char_colors[i] = VGA32_COLOR_LIGHT_GREEN;
            if (c == '\'' && (i == 0 || text_buffer[i-1] != '\\')) in_char = 0;
            continue;
        }

        
        if (c == '/' && i + 1 < text_length && text_buffer[i+1] == '/') {
            in_comment = 1;
            char_colors[i] = VGA32_COLOR_DARK_GREY;
            continue;
        }
        if (c == '"') { in_string = 1; char_colors[i] = VGA32_COLOR_LIGHT_GREEN; continue; }
        if (c == '\'') { in_char = 1; char_colors[i] = VGA32_COLOR_LIGHT_GREEN; continue; }

        if (is_digit(c)) {
            char_colors[i] = VGA32_COLOR_LIGHT_RED;
            continue;
        }

        
        if (is_alpha(c) || c == '#') {
            int len = 0;
            while (i + len < text_length && (is_alpha(text_buffer[i+len]) || is_digit(text_buffer[i+len]) || text_buffer[i+len] == '#')) {
                len++;
            }
            
            int kw = is_keyword(&text_buffer[i], len);
            uint32_t color = kw ? VGA32_COLOR_YELLOW : VGA32_COLOR_WHITE;
            
            if (!kw && len > 0 && i + len < text_length && text_buffer[i+len] == '(') {
                color = VGA32_COLOR_LIGHT_BLUE;
            }

            for (int j = 0; j < len; j++) char_colors[i+j] = color;
            i += len - 1; 
            continue;
        }
        
        
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']' || c == '+' || c == '-' || c == '*' || c == '=' || c == ';' || c == ',') {
            char_colors[i] = VGA32_COLOR_LIGHT_CYAN;
        }
    }
}


void draw_editor() {
    
    int cursor_row = 0;
    int line_start = 0;
    for(int i = 0; i < cursor_index; i++) {
        if(text_buffer[i] == '\n') {
            cursor_row++;
            line_start = i + 1;
        }
    }
    int cursor_col = cursor_index - line_start;

    
    int max_visible_lines = (win->height - 16) / 8; 
    if (cursor_row < scroll_line) scroll_line = cursor_row;
    if (cursor_row >= scroll_line + max_visible_lines) scroll_line = cursor_row - max_visible_lines + 1;

    
    int max_visible_cols = (win->width - 36 - 8) / 8;
    if (max_visible_cols < 1) max_visible_cols = 1;
    if (cursor_col < scroll_col) scroll_col = cursor_col;
    if (cursor_col >= scroll_col + max_visible_cols) scroll_col = cursor_col - max_visible_cols + 1;

    
    Rect bg;
    bg.x = win->x; bg.y = win->y + 8; bg.width = win->width; bg.height = win->height - 16;
    bg.color = VGA32_COLOR_BLACK;
    draw_rect_filled(&bg);
    
    
    Rect margin;
    margin.x = win->x; margin.y = win->y + 8; margin.width = 32; margin.height = win->height - 16;
    margin.color = 0x00222222; 
    draw_rect_filled(&margin);

    update_syntax();

    int current_line = 0;
    int current_col = 0;

    
    if (scroll_line == 0) {
        char lnbuf[8];
        itoa(1, lnbuf, 10);
        draw_text_absolute(2, 8, lnbuf, VGA32_COLOR_LIGHT_GREY);
    }

    
    for (int i = 0; i < text_length; i++) {
        unsigned int c = text_buffer[i];

        if (c == '\n') {
            current_line++;
            current_col = 0;
            if (current_line >= scroll_line && current_line < scroll_line + max_visible_lines) {
                char lnbuf[8];
                itoa(current_line + 1, lnbuf, 10);
                draw_text_absolute(2, 8 + (current_line - scroll_line) * 8, lnbuf, VGA32_COLOR_LIGHT_GREY);
            }
            continue;
        }

        
        if (current_line >= scroll_line && current_line < scroll_line + max_visible_lines) {
            if (current_col >= scroll_col && current_col < scroll_col + max_visible_cols) {
                text_struct ts;
                ts.x = 36 + (current_col - scroll_col) * 8;
                ts.y = 8 + (current_line - scroll_line) * 8;
                ts.color = char_colors[i];
                window_draw_char(win, &ts, c);
            }
        }
        current_col++;
    }

    
    Rect top;
    top.x = win->x; top.y = win->y; top.width = win->width; top.height = 8; top.color = VGA32_COLOR_BLUE;
    draw_rect_filled(&top);
    draw_text_absolute(4, 0, " NANI 1.0 - ", VGA32_COLOR_WHITE);
    draw_text_absolute(100, 0, filename, VGA32_COLOR_YELLOW);

    
    Rect bot;
    bot.x = win->x; bot.y = win->y + win->height - 8; bot.width = win->width; bot.height = 8; bot.color = VGA32_COLOR_LIGHT_GREY;
    draw_rect_filled(&bot);
    draw_text_absolute(4, win->height - 8, "F5 Save  ESC Exit    Chars: ", VGA32_COLOR_BLACK);
    
    char cbuf[16];
    itoa(text_length, cbuf, 10);
    draw_text_absolute(230, win->height - 8, cbuf, VGA32_COLOR_BLACK);
    
    
    if (cursor_row >= scroll_line && cursor_row < scroll_line + max_visible_lines &&
        cursor_col >= scroll_col && cursor_col < scroll_col + max_visible_cols) {
        
        int cursor_screen_x = 36 + (cursor_col - scroll_col) * 8;
        int cursor_screen_y = 8 + (cursor_row - scroll_line) * 8;

        Rect curs;
        curs.x = win->x + cursor_screen_x; 
        curs.y = win->y + cursor_screen_y;
        curs.width = 8; curs.height = 8; curs.color = VGA32_COLOR_WHITE;
        
        unsigned int cur_c = (cursor_index < text_length && text_buffer[cursor_index] != '\n') ? text_buffer[cursor_index] : ' ';
        draw_rect_filled(&curs);
        
        text_struct ts;
        ts.x = cursor_screen_x;
        ts.y = cursor_screen_y;
        ts.color = VGA32_COLOR_BLACK;
        window_draw_char(win, &ts, cur_c);
    }

    window_refresh(win);
}


void draw_thread(int argc, char** argv) {
    while(1) {
        draw_editor();
        sleep(50);
    }
}


void insert_char(unsigned int c) {
    if (text_length >= MAX_TEXT_SIZE - 1) return;
    for (int i = text_length; i > cursor_index; i--) {
        text_buffer[i] = text_buffer[i-1];
    }
    text_buffer[cursor_index] = c;
    cursor_index++;
    text_length++;
}

void delete_char() {
    if (cursor_index <= 0) return;
    for (int i = cursor_index; i < text_length; i++) {
        text_buffer[i-1] = text_buffer[i];
    }
    cursor_index--;
    text_length--;
}

void move_cursor_up() {
    int line_start = cursor_index;
    while(line_start > 0 && text_buffer[line_start-1] != '\n') line_start--;
    int col = cursor_index - line_start;
    
    if (line_start == 0) { cursor_index = 0; return; }
    
    int prev_start = line_start - 1;
    while(prev_start > 0 && text_buffer[prev_start-1] != '\n') prev_start--;
    
    int prev_len = (line_start - 1) - prev_start;
    cursor_index = prev_start + (col < prev_len ? col : prev_len);
}

void move_cursor_down() {
    int line_start = cursor_index;
    while(line_start > 0 && text_buffer[line_start-1] != '\n') line_start--;
    int col = cursor_index - line_start;
    
    int next_start = cursor_index;
    while(next_start < text_length && text_buffer[next_start] != '\n') next_start++;
    if (next_start >= text_length) { cursor_index = text_length; return; }
    next_start++; 
    
    int next_end = next_start;
    while(next_end < text_length && text_buffer[next_end] != '\n') next_end++;
    
    int next_len = next_end - next_start;
    cursor_index = next_start + (col < next_len ? col : next_len);
}

void main(int argc, char **argv){
    if(argc < 2 || strcmp(argv[1], "&") == 0){
        printf("\nUsage: exec nani.bin <file name>\n");
        return;
    }

    strcpy(filename, argv[1]);
    
    int fs = get_file_size(filename);
    if(fs > 0 && fs < MAX_TEXT_SIZE) {
        read_file(filename, (uint8_t*)text_buffer);
        text_length = fs;
        cursor_index = text_length;
    } else {
        text_length = 0;
        cursor_index = 0;
    }

    win = create_window(VGA32_COLOR_BLACK);

    process_struct p;
    p.foo = draw_thread;
    p.argc = 0;
    p.argv = 0;
    p.name = "nani_draw";
    int drw_pid = fork(&p);

    while(1) {
        uint8_t sc = get_scanecode();
        if (sc & 0x80) continue; 

        if (sc == 0x01) { 
            break; 
        }
        else if (sc == 0x3F) { 
            write_file(filename, (uint8_t*)text_buffer, text_length);
        }
        else if (sc == 0x48) { move_cursor_up(); }
        else if (sc == 0x50) { move_cursor_down(); }
        else if (sc == 0x4B) { if (cursor_index > 0) cursor_index--; }
        else if (sc == 0x4D) { if (cursor_index < text_length) cursor_index++; }
        else if (sc == 0x0E) { delete_char(); }
        else if (sc == 0x1C) { insert_char('\n'); }
        else if (sc == 0x0F) {
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
            insert_char(' ');
        }
        else {
            unsigned int c = scancode_to_ascii(sc);
            if (c != 0) insert_char(c);
        }
    }

    kill(drw_pid);
    return;
}