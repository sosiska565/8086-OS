#include "drivers/vga/vga.h"

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (LINES * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT)

#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

#define CURSOR_LOCATION_HIGH 0x0E
#define CURSOR_LOCATION_LOW  0x0F
#define CURSOR_START_REG     0x0A
#define CURSOR_END_REG       0x0B

unsigned int current_loc = 0;
char *vidptr = (char*) 0xb8000;
unsigned int lines = 0;
static uint8_t current_color = 0x07;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void set_text_color(vga_color_t fg) {
    current_color = (current_color & 0xF0) | (fg & 0x0F);
}

void set_background_color(vga_color_t bg) {
    current_color = (current_color & 0x0F) | ((bg & 0x0F) << 4);
}

void set_color(vga_color_t fg, vga_color_t bg) {
    current_color = ((bg & 0x0F) << 4) | (fg & 0x0F);
}

uint8_t get_current_color(void) {
    return current_color;
}

void update_cursor(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_HIGH);
    outb(VGA_DATA_REGISTER, (current_loc >> 8) & 0xFF);
    
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_LOW);
    outb(VGA_DATA_REGISTER, current_loc & 0xFF);
}

void disable_cursor(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_START_REG);
    outb(VGA_DATA_REGISTER, 0x20);
}

void enable_cursor(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_START_REG);
    outb(VGA_DATA_REGISTER, 0x0E);
    
    outb(VGA_CTRL_REGISTER, CURSOR_END_REG);
    outb(VGA_DATA_REGISTER, 0x0F);
}

void enable_cursor_block(void) {
    outb(VGA_CTRL_REGISTER, CURSOR_START_REG);
    outb(VGA_DATA_REGISTER, 0x00);
    
    outb(VGA_CTRL_REGISTER, CURSOR_END_REG);
    outb(VGA_DATA_REGISTER, 0x0F);
}

unsigned int get_cursor_position(void) {
    unsigned int pos = 0;
    
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_HIGH);
    pos = inb(VGA_DATA_REGISTER) << 8;
    
    outb(VGA_CTRL_REGISTER, CURSOR_LOCATION_LOW);
    pos |= inb(VGA_DATA_REGISTER);
    
    return pos;
}

void set_cursor_position(unsigned int x, unsigned int y) {
    if (x >= COLUMNS_IN_LINE) x = COLUMNS_IN_LINE - 1;
    if (y >= LINES) y = LINES - 1;
    
    current_loc = y * COLUMNS_IN_LINE + x;
    update_cursor();
}

void move_cursor_next_line(void) {
    current_loc += COLUMNS_IN_LINE - (current_loc % COLUMNS_IN_LINE);
    lines++;
    
    if (current_loc >= COLUMNS_IN_LINE * LINES) {
        current_loc = 0;
    }
    
    update_cursor();
}

void get_cursor_xy(unsigned int *x, unsigned int *y) {
    *x = current_loc % COLUMNS_IN_LINE;
    *y = current_loc / COLUMNS_IN_LINE;
}

void scroll_screen(void) {
    for (unsigned int i = 0; i < (LINES - 1) * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i++) {
        vidptr[i] = vidptr[i + COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT];
    }
    
    unsigned int last_line_start = (LINES - 1) * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT;
    for (unsigned int i = 0; i < COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT; i += 2) {
        vidptr[last_line_start + i] = ' ';
        vidptr[last_line_start + i + 1] = current_color;
    }
    
    current_loc -= COLUMNS_IN_LINE;
    lines--;
    update_cursor();
}

void clear_screen(void) {
    unsigned int i = 0;
    while(i < SCREENSIZE) {
        vidptr[i++] = ' ';
        vidptr[i++] = current_color;
    }
    current_loc = 0;
    lines = 0;
    update_cursor();
}

void clear_screen_colored(uint8_t color) {
    unsigned int i = 0;
    while(i < SCREENSIZE) {
        vidptr[i++] = ' ';
        vidptr[i++] = color;
    }
    current_loc = 0;
    lines = 0;
    update_cursor();
}

void print_char(char c) {
    print_char_colored(c, current_color);
}

void print_char_colored(char c, uint8_t color) {
    if(c == '\n') {
        move_cursor_next_line();
    } else if(c == '\b') {
        if(current_loc > 0) {
            current_loc--;
            vidptr[current_loc * 2] = ' ';
            vidptr[current_loc * 2 + 1] = color;
            update_cursor();
        }
    } else {
        vidptr[current_loc * 2] = c;
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;
        
        if(current_loc % COLUMNS_IN_LINE == 0) {
            lines++;
            
            if(current_loc >= COLUMNS_IN_LINE * LINES) {
                scroll_screen();
                current_loc = (LINES - 1) * COLUMNS_IN_LINE;
            }
        }
        
        update_cursor();
    }
}

void print(const char* str) {
    print_colored(str, current_color);
}

void print_colored(const char* str, uint8_t color) {
    unsigned int i = 0;
    while(str[i] != '\0') {
        if(str[i] == '\b') {
            if(current_loc > 0) {
                current_loc--;
                vidptr[current_loc * 2] = ' ';
                vidptr[current_loc * 2 + 1] = color;
            }
        } else if(str[i] == '\n') {
            current_loc += COLUMNS_IN_LINE - (current_loc % COLUMNS_IN_LINE);
            lines++;
            
            if(current_loc >= COLUMNS_IN_LINE * LINES) {
                scroll_screen();
                current_loc = (LINES - 1) * COLUMNS_IN_LINE;
            }
        } else {
            vidptr[current_loc * 2] = str[i];
            vidptr[current_loc * 2 + 1] = color;
            current_loc++;
            
            if(current_loc % COLUMNS_IN_LINE == 0) {
                lines++;
                
                if(current_loc >= COLUMNS_IN_LINE * LINES) {
                    scroll_screen();
                    current_loc = (LINES - 1) * COLUMNS_IN_LINE;
                }
            }
        }
        
        i++;
    }
    
    update_cursor();
}

void printn(char *c) {
    print("\n");
    print(c);
}

void printn_colored(char *c, uint8_t color) {
    print_char_colored('\n', color);
    print_colored(c, color);
}

void printnumber(int num) {
    printnumber_colored(num, current_color);
}

void printnumber_colored(int num, uint8_t color) {
    char buffer[32];
    int i = 0, isNegative = 0;

    if(num < 0) {
        isNegative = 1;
        num = -num;
    }

    do {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    } while(num > 0);

    if(isNegative) {
        buffer[i++] = '-';
    }

    for(int j = i - 1; j >= 0; j--) {
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;
        
        if(current_loc % COLUMNS_IN_LINE == 0) {
            lines++;
            
            if(current_loc >= COLUMNS_IN_LINE * LINES) {
                scroll_screen();
                current_loc = (LINES - 1) * COLUMNS_IN_LINE;
            }
        }
    }
    
    update_cursor();
}

void printhex(unsigned int num) {
    printhex_colored(num, current_color);
}

void printhex_colored(unsigned int num, uint8_t color) {
    char buffer[32];
    int i = 0;

    print_colored("0x", color);

    if(num == 0) {
        buffer[i++] = '0';
    } else {
        while(num > 0) {
            uint32_t remainder = num % 16;

            if(remainder < 10) {
                buffer[i++] = remainder + '0';
            } else {
                buffer[i++] = remainder - 10 + 'A';
            }

            num /= 16;
        }
    }

    for(int j = i - 1; j >= 0; j--) {
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = color;
        current_loc++;
        
        if(current_loc % COLUMNS_IN_LINE == 0) {
            lines++;
            
            if(current_loc >= COLUMNS_IN_LINE * LINES) {
                scroll_screen();
                current_loc = (LINES - 1) * COLUMNS_IN_LINE;
            }
        }
    }
    
    update_cursor();
}

//utils

char** parse_str(char* str) {
    static char* tokens[256]; 
    static char buffer[256]; 
    
    if (!str) {
        tokens[0] = 0;
        return tokens;
    }
    
    int token_count = 0;
    char* ptr = buffer;
    
    while (*str && token_count < 255) {
        while (*str == ' ') {
            str++;
        }
        
        if (!*str) break;
        
        tokens[token_count] = ptr;
        
        while (*str && *str != ' ') {
            *ptr++ = *str++;
        }
        
        *ptr++ = '\0';
        token_count++;
    }
    
    tokens[token_count] = 0;
    
    return tokens;
}

int strcmp(char *c1, char *c2) {
    for(int i = 0; ; i++) {
        if(c1[i] != c2[i]) {
            return c1[i] - c2[i];
        }
        if(c1[i] == '\0') {
            return 0;
        }
    }
}