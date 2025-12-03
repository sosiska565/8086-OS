#include "programs/system/console/console.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"

static void print_rainbow(const char* text) {
    vga_color_t colors[] = {
        VGA_COLOR_RED, VGA_COLOR_YELLOW, VGA_COLOR_GREEN, 
        VGA_COLOR_CYAN, VGA_COLOR_BLUE, VGA_COLOR_MAGENTA
    };
    
    int i = 0;
    int color_idx = 0;
    while(text[i] != '\0') {
        if(text[i] != ' ' && text[i] != '\n') {
            print_char_colored(text[i], VGA_COLOR(colors[color_idx % 6], VGA_COLOR_BLACK));
            color_idx++;
        } else {
            print_char(text[i]);
        }
        i++;
    }
}

static void cmd_rainbow(void) {
    print("Enter text: ");
    char text[100];
    gets(text, 100);
    print_rainbow(text);
    print("\n");
}

static void cmd_colortest(void) {
    print("\nVGA Color Table:\n");
    print("================\n");
    
    const char* color_names[] = {
        "Black", "Blue", "Green", "Cyan", 
        "Red", "Magenta", "Brown", "Light Grey",
        "Dark Grey", "Light Blue", "Light Green", "Light Cyan",
        "Light Red", "Light Magenta", "Yellow", "White"
    };
    
    for(int i = 0; i < 16; i++) {
        printnumber(i);
        print(": ");
        print_colored("████ ", VGA_COLOR(VGA_COLOR_BLACK, i));
        print(color_names[i]);
        print("\n");
    }
}

static void cmd_banner(void) {
    set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    print("\n");
    print("  ____   ____   _____\n");
    print(" | __ ) / __ \\ / ____|\n");
    print(" |  _ \\| |  | | (___  \n");
    print(" | |_) | |  | |\\___ \\ \n");
    print(" |____/| |__| |____) |\n");
    print("       \\____/|_____/ \n");
    print("\n");
    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    print("         Basic Operating System\n");
    print("         Version 1.0.0\n\n");
}

static void cmd_sysinfo(void) {
    print("\nSystem Information:\n");
    print("==================\n");
    print("OS Name: BOS (Basic Operating System)\n");
    print("Version: 1.0.0\n");
    print("Architecture: x86 (32-bit)\n");
    print("Video Mode: VGA Text Mode 80x25\n");
    print("Memory: Real Mode addressing\n");
    print("\n");
}

static void cmd_echo(char **tokens) {
    if (tokens[1] == 0) {
        print("\n");
        return;
    }
    
    int i = 1;
    while(tokens[i] != 0) {
        print(tokens[i]);
        if(tokens[i+1] != 0) {
            print(" ");
        }
        i++;
    }
    print("\n");
}

static void cmd_calc(char **tokens) {
    if (!tokens[1] || !tokens[2] || !tokens[3]) {
        print("Usage: calc <num1> <op> <num2>\n");
        print("Operations: + - * /\n");
        return;
    }
    
    int num1 = 0, num2 = 0;
    char *s1 = tokens[1];
    char *s2 = tokens[3];
    
    while(*s1 >= '0' && *s1 <= '9') {
        num1 = num1 * 10 + (*s1 - '0');
        s1++;
    }
    
    while(*s2 >= '0' && *s2 <= '9') {
        num2 = num2 * 10 + (*s2 - '0');
        s2++;
    }
    
    char op = tokens[2][0];
    int result = 0;
    
    if(op == '+') {
        result = num1 + num2;
    } else if(op == '-') {
        result = num1 - num2;
    } else if(op == '*') {
        result = num1 * num2;
    } else if(op == '/') {
        if(num2 == 0) {
            print("Error: Division by zero!\n");
            return;
        }
        result = num1 / num2;
    } else {
        print("Unknown operation!\n");
        return;
    }
    
    print("Result: ");
    printnumber(result);
    print("\n");
}

static void cmd_time(void) {
    static int uptime = 0;
    uptime++;
    
    print("System Uptime: ");
    printnumber(uptime);
    print(" commands executed\n");
}

static void cmd_ascii(void) {
    print("\nASCII Table (printable):\n");
    print("========================\n");
    
    for(int i = 32; i < 127; i++) {
        printnumber(i);
        print(": ");
        print_char((char)i);
        print("  ");
        
        if((i - 31) % 8 == 0) {
            print("\n");
        }
    }
    print("\n");
}

static void cmd_box(void) {
    print("\n");
    print("+-------------------------------+\n");
    print("|   Welcome to 8086-OS Console! |\n");
    print("|   Type 'help' for commands!   |\n");
    print("+-------------------------------+\n");
    print("\n");
}

static void cmd_count(char **tokens) {
    if(!tokens[1]) {
        print("Usage: count <number>\n");
        return;
    }
    
    int num = 0;
    char *s = tokens[1];
    while(*s >= '0' && *s <= '9') {
        num = num * 10 + (*s - '0');
        s++;
    }
    
    if(num > 100) {
        print("Number too large! Max: 100\n");
        return;
    }
    
    print("Counting to ");
    printnumber(num);
    print(":\n");
    
    for(int i = 1; i <= num; i++) {
        printnumber(i);
        print(" ");
        if(i % 10 == 0) {
            print("\n");
        }
    }
    print("\n");
}

static void cmd_matrix(void) {
    print("\nMatrix Effect (press any key to stop):\n");
    set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    
    for(int i = 0; i < 200; i++) {
        char c = (i % 26) + 'A';
        print_char(c);
        
        if(i % 80 == 0) {
            print("\n");
        }
    }
    
    set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    print("\n");
}

static void cmd_joke(void) {
    static int joke_num = 0;
    
    const char* jokes[] = {
        "Why do programmers prefer dark mode?\nBecause light attracts bugs!",
        "How many programmers does it take to change a light bulb?\nNone. It's a hardware problem!",
        "Why did the developer go broke?\nBecause he used up all his cache!",
        "What's a programmer's favorite hangout place?\nFoo Bar!",
        "Why do Java developers wear glasses?\nBecause they can't C#!"
    };
    
    print("\n");
    print_colored("Joke of the day:\n", VGA_COLOR(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
    print(jokes[joke_num % 5]);
    print("\n\n");
    
    joke_num++;
}

static void cmd_reverse(char **tokens) {
    if(!tokens[1]) {
        print("Usage: reverse <text>\n");
        return;
    }
    
    char buffer[100];
    int pos = 0;
    
    int i = 1;
    while(tokens[i] != 0) {
        char *s = tokens[i];
        while(*s != '\0' && pos < 99) {
            buffer[pos++] = *s++;
        }
        if(tokens[i+1] != 0 && pos < 99) {
            buffer[pos++] = ' ';
        }
        i++;
    }
    buffer[pos] = '\0';
    
    print("Reversed: ");
    for(int j = pos - 1; j >= 0; j--) {
        print_char(buffer[j]);
    }
    print("\n");
}

static void cmd_settextcolor(char **tokens) {
    if (tokens[1] != 0) {
        char* color_str = tokens[1];
        int color = 0;
        
        while (*color_str >= '0' && *color_str <= '9') {
            color = color * 10 + (*color_str - '0');
            color_str++;
        }
        
        if (color >= 0 && color <= 15) {
            set_text_color(color);
            print("Text color set to ");
            printnumber(color);
            print("\n");
        } else {
            print("Invalid color! Use 0-15\n");
        }
    } else {
        print("Usage: settextcolor <0-15>\n");
    }
}

static int main(void) {
    cmd_box();
    enable_cursor();

    while(1) {
        print("> ");
        
        char command[100];
        gets(command, 100);
        char **tokens = parse_str(command);
        
        if (command[0] == '\0') {
            continue;
        }
        
        if (tokens[0] && strcmp(tokens[0], "help") == 0) {
            print("\nAvailable Commands:\n");
            print("===================\n");
            print("System:\n");
            print("  help            - Show this help\n");
            print("  clear           - Clear screen\n");
            print("  exit            - Exit console\n");
            print("  sysinfo         - System information\n");
            print("  time            - Show uptime\n");
            print("\nGraphics:\n");
            print("  setbgcolor <N>  - Set background color (0-15)\n");
            print("  settextcolor <N>- Set text color (0-15)\n");
            print("  colortest       - Show all colors\n");
            print("  rainbow         - Rainbow text\n");
            print("  banner          - Show OS banner\n");
            print("  box             - Draw a box\n");
            print("  matrix          - Matrix effect\n");
            print("\nUtilities:\n");
            print("  echo <text>     - Print text\n");
            print("  calc <a> <op> <b> - Calculator (+,-,*,/)\n");
            print("  count <N>       - Count to N\n");
            print("  reverse <text>  - Reverse text\n");
            print("  ascii           - Show ASCII table\n");
            print("  joke            - Random joke\n");
            print("\n");
            
        } else if (tokens[0] && strcmp(tokens[0], "clear") == 0) {
            clear_screen();
            
        } else if (tokens[0] && strcmp(tokens[0], "exit") == 0) {
            break;
            
        } else if (tokens[0] && strcmp(tokens[0], "setbgcolor") == 0) {
            if (tokens[1] != 0) {
                char* color_str = tokens[1];
                int color = 0;
                
                while (*color_str >= '0' && *color_str <= '9') {
                    color = color * 10 + (*color_str - '0');
                    color_str++;
                }
                
                if (color >= 0 && color <= 15) {
                    set_background_color(color);
                    print("Background color set to ");
                    printnumber(color);
                    print("\n");
                } else {
                    print("Invalid color! Use 0-15\n");
                }
            } else {
                print("Usage: setbgcolor <0-15>\n");
            }
            
        } else if (tokens[0] && strcmp(tokens[0], "settextcolor") == 0) {
            cmd_settextcolor(tokens);
            
        } else if (tokens[0] && strcmp(tokens[0], "colortest") == 0) {
            cmd_colortest();
            
        } else if (tokens[0] && strcmp(tokens[0], "rainbow") == 0) {
            cmd_rainbow();
            
        } else if (tokens[0] && strcmp(tokens[0], "banner") == 0) {
            cmd_banner();
            
        } else if (tokens[0] && strcmp(tokens[0], "sysinfo") == 0) {
            cmd_sysinfo();
            
        } else if (tokens[0] && strcmp(tokens[0], "echo") == 0) {
            cmd_echo(tokens);
            
        } else if (tokens[0] && strcmp(tokens[0], "calc") == 0) {
            cmd_calc(tokens);
            
        } else if (tokens[0] && strcmp(tokens[0], "time") == 0) {
            cmd_time();
            
        } else if (tokens[0] && strcmp(tokens[0], "ascii") == 0) {
            cmd_ascii();
            
        } else if (tokens[0] && strcmp(tokens[0], "box") == 0) {
            cmd_box();
            
        } else if (tokens[0] && strcmp(tokens[0], "count") == 0) {
            cmd_count(tokens);
            
        } else if (tokens[0] && strcmp(tokens[0], "matrix") == 0) {
            cmd_matrix();
            
        } else if (tokens[0] && strcmp(tokens[0], "joke") == 0) {
            cmd_joke();
            
        } else if (tokens[0] && strcmp(tokens[0], "reverse") == 0) {
            cmd_reverse(tokens);
            
        } else {
            print_colored("Unknown command: ", VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK));
            if (tokens[0]) {
                print(tokens[0]);
            } else {
                print(command);
            }
            print("\n");
            print("Type 'help' for available commands\n");
        }
    }
    
    disable_cursor();
    return 0;
}

Console console = {
    .main = main
};