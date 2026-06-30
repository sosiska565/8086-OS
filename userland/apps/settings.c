#include <oslib.h>
#include "libgui.h"

#define C_SIDEBAR_BG   0x00282828
#define C_MAIN_BG      0x001E1E1E
#define C_ACCENT       0x000A84FF
#define C_TEXT_PRI     0x00FFFFFF
#define C_TEXT_SEC     0x00888888
#define C_DIVIDER      0x00111111
#define C_SUCCESS      0x0034C759
#define C_ERROR        0x00FF453A

int current_tab = 0; 
int msg_timer = 0;
char status_msg[128] = "";
uint32_t status_color = C_SUCCESS;

char wall_path_buf[64] = "/wallpapers/wallpaper.bmp";
int wall_path_focus = 0;

char region_buf[64] = "Europe/Moscow"; 
int region_focus = 0;
char fetched_time[64] = "Not synced yet";

void draw_sidebar_item(gui_window_t *win, int y, const char *title, int tab_idx) {
    int hovered = (win->mx >= 10 && win->mx <= 140 && win->my >= y && win->my <= y + 30);
    if (hovered && win->clicked) {
        current_tab = tab_idx;
        msg_timer = 0; 
    }
    
    if (current_tab == tab_idx) {
        gui_draw_rounded_rect(win, 10, y, 130, 30, 6, 0x003A3A3C);
        gui_draw_string(win, 25, y + 11, title, C_TEXT_PRI);
    } else {
        if (hovered) gui_draw_rounded_rect(win, 10, y, 130, 30, 6, 0x002A2A2C);
        gui_draw_string(win, 25, y + 11, title, C_TEXT_SEC);
    }
}

void apply_wallpaper(const char* path) {
    WM_Queue *wm_queue = (WM_Queue*)shm_map(WM_SHM_KEY);
    if (wm_queue) {
        int tail = wm_queue->tail;
        wm_queue->commands[tail].type = WM_CMD_SET_WALLPAPER;
        strncpy(wm_queue->commands[tail].title, path, 31);
        wm_queue->commands[tail].title[31] = '\0';
        wm_queue->tail = (tail + 1) % 32;
    }
    strcpy(status_msg, "Wallpaper updated!");
    status_color = C_SUCCESS;
    msg_timer = 150; 
}


void save_tz_config(const char* new_tz) {
    int sz = get_file_size("/kernel.cfg");
    if (sz < 0) sz = 0;
    char* buf = malloc(sz + 128); char* out = malloc(sz + 128); out[0] = '\0';
    if (sz > 0) {
        read_file("/kernel.cfg", (uint8_t*)buf); buf[sz] = '\0';
        char* line = buf;
        while (*line) {
            char* next = strchr(line, '\n'); if (next) *next = '\0';
            if (strncmp(line, "TZ=", 3) != 0 && strlen(line) > 0) { strcat(out, line); strcat(out, "\n"); }
            if (!next) break; line = next + 1;
        }
    }
    strcat(out, "TZ="); strcat(out, new_tz); strcat(out, "\n");
    write_file("/kernel.cfg", (uint8_t*)out, strlen(out));
    free(buf); free(out);
}


void sync_network_time(gui_window_t *win) {
    strcpy(status_msg, "Connecting to Time API...");
    status_color = C_TEXT_SEC;
    msg_timer = 50;
    gui_render(win); yield(); 

    uint32_t target_ip = inet_addr("worldtimeapi.org");
    if (target_ip == 0) target_ip = gethostbyname("worldtimeapi.org");
    
    if (target_ip == 0 || target_ip == 0xFFFFFFFF) {
        strcpy(status_msg, "DNS or Network Error!"); status_color = C_ERROR; msg_timer = 150; return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest)); dest.sin_family = AF_INET; dest.sin_port = htons(80); dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        strcpy(status_msg, "Connection Failed!"); status_color = C_ERROR; msg_timer = 150; close(sock); return;
    }

    char request[256];
    sprintf(request, "GET /api/timezone/%s HTTP/1.0\r\nHost: worldtimeapi.org\r\nConnection: close\r\n\r\n", region_buf);
    send(sock, request, strlen(request), 0);

    char buf[2048]; int bytes = 0; int total = 0;
    while ((bytes = recv(sock, buf + total, sizeof(buf) - total - 1, 0)) > 0) {
        total += bytes; if (total >= sizeof(buf) - 1) break;
    }
    buf[total] = '\0'; close(sock);

    char* time_ptr = strstr(buf, "\"datetime\":\"");
    if (time_ptr) {
        time_ptr += 12;
        int i = 0;
        while (time_ptr[i] && time_ptr[i] != '.' && time_ptr[i] != '+' && i < 63) {
            fetched_time[i] = time_ptr[i] == 'T' ? ' ' : time_ptr[i];
            i++;
        }
        fetched_time[i] = '\0';
        save_tz_config(region_buf);
        strcpy(status_msg, "Region saved & Time synced!"); status_color = C_SUCCESS; msg_timer = 150;
    } else {
        strcpy(status_msg, "Invalid Region Name!"); status_color = C_ERROR; msg_timer = 150;
    }
}

int main(int argc, char** argv) {
    gui_window_t *win = gui_create_window("System Settings", 600, 400);
    if (!win) return 1;
    gui_set_resizable(win, 1);

    
    char* env_tz = getenv("TZ");
    if (env_tz && strlen(env_tz) > 0) strcpy(region_buf, env_tz);

    while (!win->closed) {
        gui_update(win);

        gui_draw_rect(win, 0, 0, 150, win->h, C_SIDEBAR_BG);
        gui_draw_rect(win, 150, 0, win->w - 150, win->h, C_MAIN_BG);
        gui_draw_rect(win, 150, 0, 1, win->h, C_DIVIDER);

        gui_draw_string(win, 15, 20, "Settings", C_TEXT_SEC);
        draw_sidebar_item(win, 45, "Wallpaper", 0);
        draw_sidebar_item(win, 80, "Region & Time", 1); 

        if (current_tab == 0) {
            gui_draw_string(win, 180, 30, "Desktop Wallpaper", C_TEXT_PRI);
            gui_draw_string(win, 180, 55, "Enter path to a .bmp file:", C_TEXT_SEC);
            gui_textfield_dark(win, 180, 75, 280, 30, wall_path_buf, 31, &wall_path_focus);

            if (gui_button(win, 180, 115, 140, 30, "Apply Wallpaper", 1)) {
                apply_wallpaper(wall_path_buf);
            }
        }
        else if (current_tab == 1) {
            gui_draw_string(win, 180, 30, "Region and Timezone", C_TEXT_PRI);
            
            gui_draw_string(win, 180, 55, "Select Region (e.g., Europe/London):", C_TEXT_SEC);
            gui_textfield_dark(win, 180, 75, 280, 30, region_buf, 60, &region_focus);

            if (gui_button(win, 180, 115, 160, 30, "Apply & Sync Time", 1)) {
                sync_network_time(win);
            }

            
            gui_draw_string(win, 180, 165, "Current Network Time:", C_TEXT_SEC);
            gui_draw_string(win, 180, 185, fetched_time, C_ACCENT);
        }

        if (msg_timer > 0) {
            gui_draw_string(win, 180, win->h - 30, status_msg, status_color);
            msg_timer--;
        }

        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}