#include "drivers/vga/vga.h"

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE (LINES * COLUMNS_IN_LINE * BYTES_FOR_EACH_ELEMENT)

unsigned int current_loc = 0;
char *vidptr = (char*) 0xb8000;
unsigned int lines = 0;

void clear_screen(void){
    unsigned int i = 0;
    while(i < SCREENSIZE){
        vidptr[i++] = ' ';
        vidptr[i++] = 0x07;
    }
    current_loc = 0;
    lines = 0;
}

//string
void print(const char* str){
    unsigned int i = 0;
    while(str[i] != '\0'){
        if(str[i] == '\b'){
            vidptr[current_loc * 2] = ' ';
            vidptr[current_loc * 2 + 1] = 0x07;
            current_loc--;
        } else if(str[i] == '\n'){
            current_loc += 80 - (current_loc % 80);
            lines++;
        } else {
            vidptr[current_loc * 2] = str[i];
            vidptr[current_loc * 2 + 1] = 0x07;
            current_loc++;
        }

        if(current_loc >= 80 * 25){
            current_loc = 0;
        }
        i++;
    }

    if(lines >= LINES){
        clear_screen();
        lines = 0;
    }
}

//numbers
void printnumber(int num){
    char buffer[32];
    int i = 0, isNegative = 0;

    if(num < 0){
        isNegative = 1;
        num = -num;
    }

    do{
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    } while(num > 0);

    if(isNegative){
        buffer[i++] = '-';
    }

    for(int j = i - 1; j >= 0; j--){
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = 0x07;
        current_loc++;

        if(current_loc >= 80 * 25){
            current_loc = 0;
        }
    }

    if(lines >= LINES){
        clear_screen();
        lines = 0;
    }
}

//hex
void printhex(unsigned int num){
    char buffer[32];
    int i = 0;

    print("0x");

    if(num == 0){
        buffer[i++] = '0';
    } else {
        while(num > 0){
            uint32_t remainder = num % 16;

            if(remainder < 10){
                buffer[i++] = remainder + '0';
            } else {
                buffer[i++] = remainder - 10 + 'A';
            }

            num /= 16;
        }
    }

    for(int j = i - 1; j >= 0; j--){
        vidptr[current_loc * 2] = buffer[j];
        vidptr[current_loc * 2 + 1] = 0x07;
        current_loc++;

        if(current_loc >= 80 * 25){
            current_loc = 0;
        }
    }

    if(lines >= LINES){
        clear_screen();
        lines = 0;
    }
}

void print_char(char c){
    vidptr[current_loc * 2] = c;
    vidptr[current_loc * 2 + 1] = 0x07;
    current_loc++;

    if(current_loc >= 80 * 25){
        current_loc = 0;
    }
    
    if(lines >= LINES){
        clear_screen();
        lines = 0;
    }
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