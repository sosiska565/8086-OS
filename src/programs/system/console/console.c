#include "console.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"

void execute_command(char **tokens) {
    if(!tokens[0]) return;
    
    for(int i = 0; commands[i].name != NULL; i++) {
        if(strcmp(tokens[0], commands[i].name) == 0) {
            commands[i].handler(tokens);
            return;
        }
    }
    
    print_colored("Unknown command: ", VGA_COLOR(VGA_COLOR_RED, VGA_COLOR_BLACK));
    print(tokens[0]);
    print("\n");
}

int console_main(void) {
    cmd_box();
    console.should_exit = 0;

    while(!console.should_exit) {
        enable_cursor();
        print("> ");
        
        char command[100];
        gets(command, 100);
        
        if(command[0] == '\0') continue;
        
        char **tokens = parse_str(command);
        execute_command(tokens);
    }
    
    disable_cursor();
    return 0;
}

Console console = {
    .main = console_main,
    .should_exit = 0
};