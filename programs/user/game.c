#include <oslib.h>
#include <string.h>
#include <stddef.h>

#define MAP_WIDTH 79
#define MAP_HEIGHT 23

int playerX = 1;
int playerY = 1;

int gold = 0;
int diamond = 0;
int ametyst = 0;
int emerald = 0;
int ruby = 0;
int wood = 0;

char ruds[6] = {
    '*',
    '%',
    '=',
    '+',
    '$',
    '|',
};

char map[MAP_HEIGHT][MAP_WIDTH];

void init_map(void){
    for(int i = 0; i < MAP_HEIGHT; i++){
        for(int j = 0; j < MAP_WIDTH; j++){
            map[i][j] = ' '; 
        }
    }

    for(int i = 0; i < MAP_WIDTH; i++) map[0][i] = '#';
    for(int i = 0; i < MAP_HEIGHT; i++) map[i][0] = '#';
    for(int i = 0; i < MAP_WIDTH; i++) map[MAP_HEIGHT - 1][i] = '#';
    for(int i = 0; i < MAP_HEIGHT; i++) map[i][MAP_WIDTH - 1] = '#';
}

void gen_ruds(void){
    for(int i = 0; i < 20; i++){
        int centerX = randmm(1, MAP_WIDTH - 2);
        int centerY = randmm(1, MAP_HEIGHT - 2);
        int rudType = randmm(0, 5);
        int radius = randmm(1, 2);
        
        int dx = centerX - playerX;
        int dy = centerY - playerY;
        if(dx*dx + dy*dy > (radius + 2)*(radius + 2)){
            for(int y = -radius; y <= radius; y++){
                for(int x = -radius; x <= radius; x++){
                    if(x*x + y*y <= radius*radius){
                        int nx = centerX + x;
                        int ny = centerY + y;
                        
                        if(nx >= 1 && nx < MAP_WIDTH - 1 && 
                           ny >= 1 && ny < MAP_HEIGHT - 1 &&
                           map[ny][nx] != '#'){
                            if(randmm(0, 100) > 25){
                                map[ny][nx] = ruds[rudType];
                            }
                        }
                    }
                }
            }
        }
    }
}

void mine(void){
    if(map[playerY][playerX] == '*') {
        gold++;
        map[playerY][playerX] = ' ';
    }
    else if(map[playerY][playerX] == '%') {
        diamond++;
        map[playerY][playerX] = ' ';
    }
    else if(map[playerY][playerX] == '=') {
        ametyst++;
        map[playerY][playerX] = ' ';
    }
    else if(map[playerY][playerX] == '+') {
        emerald++;
        map[playerY][playerX] = ' ';
    }
    else if(map[playerY][playerX] == '$') {
        ruby++;
        map[playerY][playerX] = ' ';
    }
    else if(map[playerY][playerX] == '|') {
        wood++;
        map[playerY][playerX] = ' ';
    }
}

void shop(void){
    set_cursor_position(0, 5);
    draw_simple_box((char *[]){
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        "1. ",
        NULL
    }, "Shop", 1);
    while (1) {
        char c = getc();

        if(c == 'e' || c == 'E'){
            cls();
            break;
        }
    }
}

void print_map(void){
    cls();
    for(int i = 0; i < MAP_HEIGHT; i++){
        for(int j =  0; j < MAP_WIDTH; j++){
            if(i == playerY && j == playerX){
                print_char_colored('@', VGA_COLOR_LIGHT_GREEN);
            }
            else if(map[i][j] == '*'){
                print_char_colored(map[i][j], VGA_COLOR_YELLOW);
            }
            else if(map[i][j] == '%'){
                print_char_colored(map[i][j], VGA_COLOR_CYAN);
            }
            else if(map[i][j] == '='){
                print_char_colored(map[i][j], VGA_COLOR_LIGHT_MAGENTA);
            }
            else if(map[i][j] == '+'){
                print_char_colored(map[i][j], VGA_COLOR_GREEN);
            }
            else if(map[i][j] == '$'){
                print_char_colored(map[i][j], VGA_COLOR_RED);
            }
            else if(map[i][j] == '|'){
                print_char_colored(map[i][j], VGA_COLOR_BROWN);
            }
            else if(map[i][j] == '#'){
                print_char_colored('#', VGA_COLOR_LIGHT_GREY);
            }
            else {
                print_char_colored(' ', VGA_COLOR_LIGHT_GREY);
            }
        }
        print_char('\n');
    }
}

void main(void){
    init_map();
    gen_ruds();
    while(1){
        print_map();
        print_colored("Gold: ", VGA_COLOR_YELLOW);
        print_number(gold);
        print(" |");
        print_colored(" diamond: ", VGA_COLOR_CYAN);
        print_number(diamond);
        print(" |");
        print_colored(" ametyst: ", VGA_COLOR_LIGHT_MAGENTA);
        print_number(ametyst);
        print(" |");
        print_colored(" emerald: ", VGA_COLOR_GREEN);
        print_number(emerald);
        print(" |");
        print_colored(" ruby: ", VGA_COLOR_RED);
        print_number(ruby);
        print(" |");
        print_colored(" wood: ", VGA_COLOR_BROWN);
        print_number(wood);
        print(" |");

        char c = getc();

        int newX = playerX;
        int newY = playerY;

        if(c == 'a' || c == 'A') newX--;
        else if(c == 'd' || c == 'D') newX++;
        else if(c == 's' || c == 'S') newY++;
        else if(c == 'w' || c == 'W') newY--;
        else if(c == 'f' || c == 'F') mine();
        else if(c == 'e' || c == 'E') shop();
        else if(c == 'q' || c == 'Q') {
            cls();
            break;
        }

        if(map[newY][newX] != '#'){
            playerX = newX;
            playerY = newY;
        }
    }
}