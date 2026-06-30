#include <oslib.h>
#include "libgui.h"


#define MAC_SIDEBAR_BG   0x00F0F0F0
#define MAC_MAIN_BG      0x00FFFFFF
#define MAC_BLUE         0x00007AFF
#define MAC_TEXT         0x00000000
#define MAC_GRAY_TEXT    0x00888888
#define MAC_HOVER        0x00E0E0E0

int current_tab = 0; 

void draw_sidebar_item(gui_window_t *win, int y, const char *title, int tab_idx) {
    int hovered = (win->mx >= 10 && win->mx <= 130 && win->my >= y && win->my <= y + 25);
    
    
    if (hovered && win->clicked) current_tab = tab_idx;
    
    if (current_tab == tab_idx) {
        gui_draw_rounded_rect(win, 10, y, 120, 25, 4, MAC_BLUE);
        gui_draw_string(win, 20, y + 8, title, 0x00FFFFFF);
    } else {
        if (hovered) gui_draw_rounded_rect(win, 10, y, 120, 25, 4, MAC_HOVER);
        gui_draw_string(win, 20, y + 8, title, MAC_TEXT);
    }
}

int main() {
    gui_window_t *win = gui_create_window("System Settings", 500, 350);
    if (!win) return 1;
    
    gui_set_resizable(win, 1);

    int dark_mode = 0;
    int volume = 75;
    int brightness = 80;

    while (!win->closed) {
        gui_update(win);

        
        gui_draw_rect(win, 0, 0, 140, win->h, MAC_SIDEBAR_BG);
        gui_draw_rect(win, 140, 0, win->w - 140, win->h, dark_mode ? 0x00222222 : MAC_MAIN_BG);
        gui_draw_rect(win, 140, 0, 1, win->h, 0x00D0D0D0); 

        
        gui_draw_string(win, 15, 15, "Settings", MAC_GRAY_TEXT);
        draw_sidebar_item(win, 35, "General", 0);
        draw_sidebar_item(win, 65, "Display", 1);
        draw_sidebar_item(win, 95, "About", 2);

        
        uint32_t text_color = dark_mode ? 0x00FFFFFF : MAC_TEXT;
        
        if (current_tab == 0) {
            gui_draw_string(win, 160, 20, "General", text_color);
            gui_checkbox(win, 160, 50, &dark_mode, "Use Dark Mode");
            
            gui_draw_string(win, 160, 90, "Sound Volume", text_color);
            gui_slider(win, 260, 94, 150, &volume);
            
            
            if (gui_button(win, 160, 140, 150, 28, "Play Test Sound", 1)) {
                
            }
        } 
        else if (current_tab == 1) {
            gui_draw_string(win, 160, 20, "Displays", text_color);
            
            gui_draw_string(win, 160, 50, "Brightness", text_color);
            gui_slider(win, 260, 54, 150, &brightness);
            
            
            gui_draw_rounded_rect(win, 160, 100, 200, 120, 8, 0x00E8E8E8);
            gui_draw_rect(win, 170, 110, 180, 100, 0x00000000);
            gui_draw_string(win, 220, 150, "Built-in", 0x00FFFFFF);
        } 
        else if (current_tab == 2) {
            gui_draw_string(win, 160, 20, "About This Mac", text_color);
            gui_draw_string(win, 160, 50, "8086-OS Version 1.5", text_color);
            gui_draw_string(win, 160, 70, "Processor: x86", MAC_GRAY_TEXT);
            
            uint32_t used, total;
            get_mem_info(&used, &total);
            char mem_str[64];
            sprintf(mem_str, "Memory: %d MB", total / (1024 * 1024));
            gui_draw_string(win, 160, 90, mem_str, MAC_GRAY_TEXT);
        }

        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}