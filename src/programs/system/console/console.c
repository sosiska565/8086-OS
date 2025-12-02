#include "programs/system/console/console.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"

static int main(void) {
    print("Console v1.0\n");
    print("Type 'help' for commands\n\n");

    while(1) {
        print("$ ");
        
        char command[100];
        gets(command, 100);
        
        if (command[0] == '\0') {
            continue;
        }
        
        if (strcmp(command, "help") == 0) {
            print("help - show this help\n");
            print("clear - clear screen\n");
            print("exit - exit console\n");
        }
        else if (strcmp(command, "clear") == 0) {
            clear_screen();
        }
        else if (strcmp(command, "exit") == 0) {
            print("Goodbye!\n");
            return 0;
        }
        else {
            print("Unknown command: ");
            print(command);
            print("\n");
        }
    }
    
    return 0;
}

Console console = {
    .main = main
};