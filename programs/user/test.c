#include "oslib.h"

uint32_t get_rainbow_color(int pos) {
    while (pos < 0) pos += 1530;
    pos = pos % 1530;

    int phase = pos / 255;
    int offset = pos % 255;
    int r = 0, g = 0, b = 0;

    if (phase == 0)      { r = 255; g = offset; b = 0; }
    else if (phase == 1) { r = 255 - offset; g = 255; b = 0; }
    else if (phase == 2) { r = 0; g = 255; b = offset; }
    else if (phase == 3) { r = 0; g = 255 - offset; b = 255; }
    else if (phase == 4) { r = offset; g = 0; b = 255; }
    else if (phase == 5) { r = 255; g = 0; b = 255 - offset; }

    return (r << 16) | (g << 8) | b;
}

void main() {
    Window *w = create_window(VGA32_COLOR_BLACK);
    set_current_active_window(w);

    int time_offset = 0;
    
    int speed = 10;
    int wave_density = 4;

    while (1) {
        for (int y = 0; y < w->height; y++) {
            
            int color_pos = (y * wave_density) + time_offset;
            uint32_t line_color = get_rainbow_color(color_pos);

            Rect line;
            line.x = w->x;
            line.y = w->y + y;
            line.width = w->width;
            line.height = 1;
            line.color = line_color;
            
            draw_rect_filled(&line);
        }
        
        window_refresh(w);
        
        time_offset -= speed; 
        
        if (time_offset >= 1530) time_offset -= 1530;
        if (time_offset < 0) time_offset += 1530;

        sleep(26);
    }
}