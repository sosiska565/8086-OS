#include "oslib.h"
#include "string_lib.h"

Window *win;

void draw_interface(int argc, char** argv){
    text_struct ts;
    Rect header_rect;
    char *header = "NANI V1.0";
    int header_len = strlen(header);

    while(1){
        ts.x = ((win->width - (header_len * 8)) / 2);
        ts.y = 0;
        ts.color = VGA32_COLOR_YELLOW;
        ts.str = header;

        header_rect.y = win->y;
        header_rect.x = win->x;
        header_rect.height = 8;
        header_rect.width = win->width;
        header_rect.color = VGA32_COLOR_BLUE;

        draw_rect_filled(&header_rect);
        print_window(win, &ts);

        sleep(1000);
    }
}

void main(){
    win = create_window(VGA32_COLOR_BLACK);

    process_struct draw_process;
    draw_process.foo = draw_interface;
    draw_process.argc = 0;
    draw_process.argv = 0;
    draw_process.name = "draw_proc";
    int drw_pid = fork(&draw_process);

    char buff[100];

    while(1){
        gets(buff, 100);
    }

    kill(drw_pid);
    return;
}