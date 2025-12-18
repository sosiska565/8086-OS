#include "programs/system/console/console.h"

static int main(void) {
    char *test;
    cmd_box();

    while(1) {
        enable_cursor();
        print("> ");
        
        char command[100];
        gets(command, 100);
        char **tokens = parse_str(command);
        
        if (command[0] == '\0') {
            continue;
        }
        
        if (tokens[0] && strcmp(tokens[0], "help") == 0) {
            cmd_help(1);
        } else if (tokens[0] && strcmp(tokens[0], "clear") == 0) {
            clear_screen();
        } else if (tokens[0] && strcmp(tokens[0], "exit") == 0) {
            break;
        } else if (tokens[0] && strcmp(tokens[0], "setbgcolor") == 0) {
            cmd_setbgcolor(tokens);
        } else if (tokens[0] && strcmp(tokens[0], "settextcolor") == 0) {
            cmd_settextcolor(tokens);
        } else if (tokens[0] && strcmp(tokens[0], "colortest") == 0) {
            cmd_colortest();
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
        } else if(tokens[0] && strcmp(tokens[0], "memory_viewer") == 0){
            memory_viewer.main();
        } else if (tokens[0] && strcmp(tokens[0], "kmalloc") == 0) {
            int n = 0;
            char* str = tokens[1];

            while(*str >= '0' && *str <= '9'){
                n = n * 10 + (*str - '0');
                str++;
            }

            test = (char*)kmalloc(n);
        } else if (tokens[0] && strcmp(tokens[0], "kfree") == 0) {
            kfree(test);
        } else if (tokens[0] && strcmp(tokens[0], "heapdump") == 0){
            heap_dump();
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