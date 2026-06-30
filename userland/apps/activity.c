#include <oslib.h>
#include "libgui.h"


#define MAC_SIDEBAR_BG   0x00F2F2F7 
#define MAC_MAIN_BG      0x00FFFFFF 
#define MAC_TEXT_DARK    0x00000000 
#define MAC_TEXT_GRAY    0x00888888 
#define MAC_BLUE         0x00007AFF 
#define MAC_BLUE_HOVER   0x000062C9 
#define MAC_ITEM_HOVER   0x00E3E3E8 
#define MAC_GREEN        0x0034C759 
#define MAC_DIVIDER      0x00D1D1D6 

#define TAB_PROCESSES 0
#define TAB_MEMORY    1


int draw_sidebar_item(gui_window_t *win, int y, const char* text, int icon_char, int is_active) {
    int hovered = (win->mx >= 10 && win->mx <= 130 && win->my >= y && win->my <= y + 28);
    
    if (is_active) {
        gui_draw_rounded_rect(win, 10, y, 120, 28, 5, MAC_BLUE);
        gui_draw_char(win, 20, y + 10, icon_char, MAC_MAIN_BG);
        gui_draw_string(win, 35, y + 10, text, MAC_MAIN_BG);
    } else {
        if (hovered) {
            gui_draw_rounded_rect(win, 10, y, 120, 28, 5, MAC_ITEM_HOVER);
        }
        gui_draw_char(win, 20, y + 10, icon_char, MAC_BLUE);
        gui_draw_string(win, 35, y + 10, text, MAC_TEXT_DARK);
    }
    return (hovered && win->clicked);
}

int main() {
    
    gui_window_t *win = gui_create_window("Activity Monitor", 500, 350);
    if (!win) return 1;

    gui_set_resizable(win, 1);

    int current_tab = TAB_PROCESSES;
    int selected_pid = -1;
    task_info_user_t tasks[64];

    while (!win->closed) {
        gui_update(win);

        
        gui_draw_rect(win, 0, 0, 140, win->h, MAC_SIDEBAR_BG);
        gui_draw_rect(win, 140, 0, win->w - 140, win->h, MAC_MAIN_BG);
        gui_draw_rect(win, 140, 0, 1, win->h, MAC_DIVIDER); 

        
        gui_draw_string(win, 15, 15, "System", MAC_TEXT_GRAY);
        
        if (draw_sidebar_item(win, 35, "Processes", '#', current_tab == TAB_PROCESSES)) {
            current_tab = TAB_PROCESSES;
        }
        if (draw_sidebar_item(win, 65, "Memory", '*', current_tab == TAB_MEMORY)) {
            current_tab = TAB_MEMORY;
            selected_pid = -1; 
        }

        
        if (current_tab == TAB_PROCESSES) {
            gui_draw_string(win, 160, 15, "Process Name", MAC_TEXT_GRAY);
            gui_draw_string(win, 300, 15, "PID", MAC_TEXT_GRAY);
            gui_draw_string(win, 360, 15, "Status", MAC_TEXT_GRAY);
            gui_draw_rect(win, 141, 35, win->w - 141, 1, MAC_ITEM_HOVER);

            int num_tasks = get_tasks(tasks, 64);
            int list_start_y = 40;

            for (int i = 0; i < num_tasks; i++) {
                int item_y = list_start_y + (i * 24);
                
                
                if (win->clicked && win->mx > 140 && win->my >= item_y && win->my < item_y + 24) {
                    selected_pid = tasks[i].id;
                }

                
                if (selected_pid == tasks[i].id) {
                    gui_draw_rect(win, 141, item_y, win->w - 141, 24, MAC_BLUE);
                } else if (win->mx > 140 && win->my >= item_y && win->my < item_y + 24) {
                    gui_draw_rect(win, 141, item_y, win->w - 141, 24, MAC_ITEM_HOVER); 
                }

                uint32_t t_color = (selected_pid == tasks[i].id) ? MAC_MAIN_BG : MAC_TEXT_DARK;

                gui_draw_string(win, 160, item_y + 8, tasks[i].name, t_color);
                
                char pid_str[16];
                sprintf(pid_str, "%d", tasks[i].id);
                gui_draw_string(win, 300, item_y + 8, pid_str, t_color);

                char* state_str = "RUNNING";
                if (tasks[i].state == 1) state_str = "READY";
                if (tasks[i].state == 2) state_str = "SLEEPING";
                gui_draw_string(win, 360, item_y + 8, state_str, t_color);
            }

            
            if (selected_pid != -1) {
                
                if (selected_pid > 1) {
                    if (gui_button(win, win->w - 110, win->h - 40, 90, 26, "Force Quit", 1)) {
                        kill(selected_pid, 9);
                        selected_pid = -1; 
                    }
                } else {
                    gui_draw_string(win, win->w - 150, win->h - 30, "System Process (Locked)", MAC_TEXT_GRAY);
                }
            }

        } else if (current_tab == TAB_MEMORY) {
            uint32_t mem_used = 0, mem_total = 0;
            get_mem_info(&mem_used, &mem_total);
            
            gui_draw_string(win, 160, 20, "Memory Pressure", MAC_TEXT_DARK);

            
            int bar_x = 160;
            int bar_y = 50;
            int bar_w = win->w - 180;
            int bar_h = 16;
            
            gui_draw_rounded_rect(win, bar_x, bar_y, bar_w, bar_h, 8, MAC_ITEM_HOVER); 
            
            if (mem_total > 0) {
                int fill_w = (mem_used * bar_w) / mem_total;
                if (fill_w > 16) {
                    gui_draw_rounded_rect(win, bar_x, bar_y, fill_w, bar_h, 8, MAC_GREEN); 
                }
            }

            
            char info[64];
            sprintf(info, "Physical Memory: %d MB", mem_total / (1024 * 1024));
            gui_draw_string(win, 160, 90, info, MAC_TEXT_DARK);
            
            sprintf(info, "Memory Used:     %d MB", mem_used / (1024 * 1024));
            gui_draw_string(win, 160, 110, info, MAC_TEXT_DARK);

            sprintf(info, "Free Memory:     %d MB", (mem_total - mem_used) / (1024 * 1024));
            gui_draw_string(win, 160, 130, info, MAC_TEXT_DARK);
        }

        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}