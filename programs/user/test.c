#include "oslib.h"

void main(){
    Window *win = create_window(0xFF0000AA);
    set_current_active_window(win);

    printf("Пенис!");

    while(1){
        getc();
    }
}