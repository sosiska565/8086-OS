#include "oslib.h"

void main() {
    Window *w = create_window(VGA32_COLOR_BLACK);
    set_current_active_window(w);

    int max_iter = 64;
    int sw = w->width;
    int sh = w->height;

    int step = 1; 

    for (int py = 0; py < sh; py += step) {
        for (int px = 0; px < sw; px += step) {
            float x0 = (float)px / sw * 3.5f - 2.5f;
            float y0 = (float)py / sh * 2.0f - 1.0f;
            float x = 0.0f;
            float y = 0.0f;
            int iter = 0;

            while (x*x + y*y <= 4.0f && iter < max_iter) {
                float xtemp = x*x - y*y + x0;
                y = 2.0f*x*y + y0;
                x = xtemp;
                iter++;
            }

            uint32_t color = VGA32_COLOR_BLACK;
            if (iter < max_iter) {
                int r = (iter * 8) % 255;
                int g = (iter * 16) % 255;
                int b = (iter * 32) % 255;
                color = (r << 16) | (g << 8) | b;
            }

            Rect r;
            r.x = px;
            r.y = py;
            r.width = step;
            r.height = step;
            r.color = color;
            draw_rect_filled(&r);
        }
        if (py % 10 == 0) {
            window_refresh(w);
        }
    }
    
    window_refresh(w);
    
    while(1) {
        sleep(1000);
    }
}