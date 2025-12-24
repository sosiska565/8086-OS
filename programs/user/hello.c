#include "oslib.h"

void main(void){
    print("Press any key: ");
    
    char c = getc();
    
    print("\nYou pressed: ");
    print_char(c);
    print("\n");
}