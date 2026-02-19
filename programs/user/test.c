#include "oslib.h"

void main(){
    Window *w = create_window(VGA32_COLOR_BLACK);
    set_current_active_window(w);

    int x = w->x;
    int y = w->y;
    uint32_t color = 0;

    while(1){
        if(x < w->width) x++;
        else {
            x = 0;
            y++;
            color++;
            window_refresh(w);
        }
        if(y > w->height){
            y = 0;
            x = 0;
        }

        if(color >= 4294967296){
            color = 0;
        };

        Rect r;
        r.color = color;
        r.height = 1;
        r.width = 1;
        r.x = x + w->x;
        r.y = y + w->y;
        draw_rect_filled(&r);
    }
}