#include "programs/system/console/system.h"

void cmd_colortest(void) {
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

void cmd_banner(void) {
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

void cmd_sysinfo(void) {
    print("\nSystem Information:\n");
    print("==================\n");
    print("OS Name: BOS (Basic Operating System)\n");
    print("Version: 1.0.0\n");
    print("Architecture: x86 (32-bit)\n");
    print("Video Mode: VGA Text Mode 80x25\n");
    print("Memory: Real Mode addressing\n");
    print("\n");
}

void cmd_echo(char **tokens) {
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

void cmd_calc(char **tokens) {
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

void cmd_time(void) {
    print("System Uptime: ");
    printnumber(get_ticks());
    print(" commands executed\n");
}

void cmd_ascii(void) {
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

void cmd_box(void) {
    print("\n");
    print("+-------------------------------+\n");
    print("|   Welcome to 8086-OS Console! |\n");
    print("|   Type 'help' for commands!   |\n");
    print("+-------------------------------+\n");
    print("\n");
}

void cmd_settextcolor(char **tokens) {
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

void cmd_help(int page){
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
    print("  banner          - Show OS banner\n");
    print("  box             - Draw a box\n");
    print("\nUtilities:\n");
    print("  echo <text>     - Print text\n");
    print("  calc <a> <op> <b> - Calculator (+,-,*,/)\n");
    print("  ascii           - Show ASCII table\n");
    print("\n");
}

void cmd_setbgcolor(char** tokens){
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
}