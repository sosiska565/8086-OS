#include "oslib.h"
#include "string_lib.h"

Window *win;

void draw_interface(int argc, char** argv){
    text_struct ts;
    Rect header_rect;
    char *header = "NANI V1.0";
    int header_len = strlen(header);

    ts.x = ((win->width - (header_len * 8)) / 2);
    ts.y = 0;
    ts.color = VGA32_COLOR_YELLOW;
    ts.str = header;

    header_rect.y = win->y;
    header_rect.x = win->x;
    header_rect.height = 8;
    header_rect.width = win->width;
    header_rect.color = VGA32_COLOR_BLUE;

    while(1){
        draw_rect_filled(&header_rect);
        print_window(win, &ts);

        sleep(10);
    }
}

void main(){
    win = create_window(VGA32_COLOR_BLACK);

    int drw_pid = fork(draw_interface, 0, 0);

    char buff[100];

    while(1){
        gets(buff, 100);
    }

    kill(drw_pid);
    return;
}