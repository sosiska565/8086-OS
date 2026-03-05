#include "oslib.h"
#include "string_lib.h"
#include "libgui.h" 

Window *tm_win;
int tm_running = 1;

#define HISTORY_SIZE 20
uint32_t cpu_hist[HISTORY_SIZE];
uint32_t ram_hist[HISTORY_SIZE];

void init_history() {
    for(int i=0; i<HISTORY_SIZE; i++) { cpu_hist[i] = 0; ram_hist[i] = 0; }
}

void push_history(uint32_t cpu, uint32_t ram_kb) {
    for(int i=0; i<HISTORY_SIZE-1; i++) {
        cpu_hist[i] = cpu_hist[i+1];
        ram_hist[i] = ram_hist[i+1];
    }
    cpu_hist[HISTORY_SIZE-1] = cpu;
    ram_hist[HISTORY_SIZE-1] = ram_kb;
}


void draw_graph(Window *win, int x, int y, int w, int h, uint32_t *data, uint32_t max_val, uint32_t color) {
    
    gui_panel(win, x, y, w, h, 0x0010151C, 0x00445566);

    int bar_w = (w - 2) / HISTORY_SIZE;
    if (bar_w < 1) bar_w = 1;
    int start_x = (w - 2) - (HISTORY_SIZE * bar_w); 
    if (start_x < 0) start_x = 0;
    if (max_val == 0) max_val = 1; 

    for(int i=0; i<HISTORY_SIZE; i++) {
        uint32_t val = data[i] > max_val ? max_val : data[i];
        int bar_h = (int)((val * (uint32_t)(h - 2)) / max_val);
        if (val > 0 && bar_h == 0) bar_h = 1;
        if (bar_h == 0) continue; 
        
        int bar_x = x + 1 + start_x + (i * bar_w);
        int bar_y = y + h - 1 - bar_h;
        int bar_width = bar_w - 1 > 0 ? bar_w - 1 : 1;
        
        
        if (bar_x >= x + 1 && bar_x + bar_width <= x + w - 1) { 
            gui_draw_rect(win, bar_x, bar_y, bar_width, bar_h, color);
        }
    }
}

void taskmgr_thread(int argc, char **argv) {
    init_history();
    task_info_t tasks[64];

    while(tm_running) {
        
        gui_draw_rect(tm_win, 0, 0, tm_win->width, tm_win->height, 0x00111116);
        gui_draw_rect(tm_win, 0, 0, tm_win->width, 20, 0x002B2B36);
        gui_label(tm_win, 8, 6, " PERFORMANCE MONITOR", VGA32_COLOR_WHITE);
        
        uint32_t used_ram = 0, total_ram = 0, cpu_usage = 0;
        get_system_info(&used_ram, &total_ram, &cpu_usage);
        
        uint32_t used_kb = used_ram / 1024; uint32_t total_kb = total_ram / 1024;
        uint32_t used_mb = used_kb / 1024; uint32_t total_mb = total_kb / 1024;
        push_history(cpu_usage, used_kb);

        int graph_w = (tm_win->width - 24) / 2;
        int graph_h = 60;
        if (graph_w < 50) graph_w = 50;

        
        draw_graph(tm_win, 8, 28, graph_w, graph_h, cpu_hist, 100, 0x0000AAFF);
        char buf[64]; char num[16];
        strcpy(buf, "CPU: "); itoa(cpu_usage, num, 10); strcat(buf, num); strcat(buf, "%");
        gui_label(tm_win, 8, 32 + graph_h, buf, VGA32_COLOR_WHITE);

        
        uint32_t ram_max_scale = total_kb / 8; 
        for(int i=0; i<HISTORY_SIZE; i++) if(ram_hist[i] > ram_max_scale) ram_max_scale = ram_hist[i];
        
        draw_graph(tm_win, 16 + graph_w, 28, graph_w, graph_h, ram_hist, ram_max_scale, 0x00AA55FF);
        strcpy(buf, "RAM: "); itoa(used_mb, num, 10); strcat(buf, num); strcat(buf, " / "); 
        itoa(total_mb, num, 10); strcat(buf, num); strcat(buf, " MB");
        gui_label(tm_win, 16 + graph_w, 32 + graph_h, buf, VGA32_COLOR_WHITE);

        
        int list_y = 32 + graph_h + 20;
        gui_label(tm_win, 8, list_y, "PID   STATE       PARENT   NAME", 0x008888AA);
        list_y += 16;

        
        int task_count = get_task_list(tasks, 64);
        for(int i = 0; i < task_count; i++) {
            if (list_y > tm_win->height - 24) break;
            
            itoa(tasks[i].id, num, 10);
            gui_label(tm_win, 8, list_y, num, VGA32_COLOR_WHITE);

            char *st = "READY   "; uint32_t st_col = VGA32_COLOR_WHITE;
            if (tasks[i].state == 0) { st = "RUNNING "; st_col = 0x0000FF00; }
            else if (tasks[i].state == 2) { st = "SLEEPING"; st_col = 0x0055AAFF; }
            
            gui_label(tm_win, 56, list_y, st, st_col);

            if(tasks[i].parent_id >= 0) itoa(tasks[i].parent_id, num, 10);
            else strcpy(num, "-");
            gui_label(tm_win, 152, list_y, num, 0x00888888);

            gui_label(tm_win, 224, list_y, tasks[i].name, VGA32_COLOR_YELLOW);

            list_y += 12;
        }

        gui_label(tm_win, 8, tm_win->height - 16, "ESC: Close window", 0x00666666);
        window_refresh(tm_win);
        sleep(200); 
    }
    exit();
}

void main(int argc, char **argv) {
    tm_win = create_window(0x00111116);
    set_current_active_window(tm_win);
    tm_running = 1;

    process_struct p;
    p.foo = taskmgr_thread;
    p.argc = 0;
    p.argv = 0;
    p.name = "taskmgr_ui";
    int drw_pid = fork(&p);

    while(1) {
        uint8_t sc = get_scanecode();
        if (sc == 0x01) { 
            tm_running = 0;
            break;
        }
    }

    kill(drw_pid);
    close_window(tm_win);
}