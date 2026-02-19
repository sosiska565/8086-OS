#include "oslib.h"

void a(int x, char** ar){
    Window *win = create_window(VGA32_COLOR_BLUE);
    set_current_active_window(win);

    while(1);
}

void main() {
    process_struct as;
    as.foo = a;
    as.argc = 0;
    as.argv = 0;
    as.name = "penis";
    for(int i = 0; i < 4000; i++){
        int pid = fork(&as);
        kill(pid);
    }
}