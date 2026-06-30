/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/store.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"


#define C_SIDEBAR_BG   0x00202020
#define C_MAIN_BG      0x00181818
#define C_CARD_BG      0x00282828
#define C_DIVIDER      0x00333333
#define C_TEXT_PRI     0x00FFFFFF
#define C_TEXT_SEC     0x0098989D
#define C_ACCENT       0x000A84FF
#define C_ACCENT_HOV   0x000062C9
#define C_BTN_DISABLED 0x00444444
#define C_SUCCESS      0x0034C759
#define C_ERROR        0x00FF453A
#define C_BTN_UNINST   0x00FF453A
#define C_BTN_UNINST_H 0x00FF6B60
#define C_BTN_DELSRV   0x00660000
#define C_BTN_DELSRV_H 0x00990000
#define C_CTX_BG       0x00252525
#define C_CTX_BORDER   0x00444444

#define REPO_HOST "150.241.64.152"
#define REPO_PORT 80
#define BUFFER_SIZE 4096

#define TAB_DISCOVER 0
#define TAB_SEARCH   1
#define TAB_UPLOAD   2

typedef struct {
    char name[64];
    char filename[64];
    int is_installed;
} Package;

Package packages[128];
int num_packages = 0;

int current_tab = TAB_DISCOVER;
int scroll_y = 0;

char search_buf[64] = "";
int search_focus = 0;

char up_path_buf[128] = "";
int up_path_focus = 0;
char up_name_buf[64] = "";
int up_name_focus = 0;

static int is_dragging_sb = 0;



int check_installed(const char* filename) {
    char path[128];
    if (strlen(filename) > 63) return 0;
    sprintf(path, "/path/%s", filename);
    return get_file_size(path) > 0;
}

void update_installed_status() {
    for (int i = 0; i < num_packages; i++) {
        packages[i].is_installed = check_installed(packages[i].filename);
    }
}

void str_to_lower(char* d, const char* s) {
    while(*s) { *d = (*s >= 'A' && *s <= 'Z') ? *s + 32 : *s; d++; s++; }
    *d = '\0';
}

int get_visible_count() {
    if (current_tab != TAB_SEARCH || search_buf[0] == '\0') return num_packages;
    
    char search_low[64];
    str_to_lower(search_low, search_buf);
    
    int cnt = 0;
    for (int i = 0; i < num_packages; i++) {
        char name_low[64]; str_to_lower(name_low, packages[i].name);
        if (strstr(name_low, search_low) != NULL) cnt++;
    }
    return cnt;
}

void draw_overlay(gui_window_t *win, const char* text) {
    gui_draw_rounded_rect(win, win->w/2 - 120, win->h/2 - 25, 240, 50, 8, C_CTX_BORDER);
    gui_draw_rounded_rect(win, win->w/2 - 119, win->h/2 - 24, 238, 48, 7, C_MAIN_BG);
    gui_draw_string(win, win->w/2 - strlen(text)*4, win->h/2 - 4, text, C_TEXT_PRI);
    gui_render(win);
    yield();
}



void fetch_packages(gui_window_t *win) {
    draw_overlay(win, "Connecting to repository...");

    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    if (target_ip == 0 || target_ip == 0xFFFFFFFF) {
        draw_overlay(win, "DNS/Network Error!");
        for(int i=0; i<50; i++) yield();
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(REPO_PORT);
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        draw_overlay(win, "Connection Failed!");
        for(int i=0; i<50; i++) yield();
        close(sock); return;
    }

    char request[256];
    sprintf(request, "GET /packages.txt HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", REPO_HOST);
    send(sock, request, strlen(request), 0);

    char buf[BUFFER_SIZE];
    int bytes;
    int header_passed = 0;
    
    char *index_data = malloc(16384);
    if (!index_data) {
        draw_overlay(win, "Out of memory!");
        for(int i=0; i<50; i++) yield();
        close(sock); return;
    }
    memset(index_data, 0, 16384);
    int index_pos = 0;

    draw_overlay(win, "Fetching list...");

    while ((bytes = recv(sock, buf, BUFFER_SIZE, 0)) > 0) {
        char *data_ptr = buf;
        int data_len = bytes;
        if (!header_passed) {
            for (int i = 0; i < bytes - 3; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
                    header_passed = 1; data_ptr = buf + i + 4; data_len = bytes - (i + 4); break;
                }
            }
            if (!header_passed) continue;
        }
        if (data_len > 0 && index_pos + data_len < 16383) {
            memcpy(index_data + index_pos, data_ptr, data_len);
            index_pos += data_len;
        }
    }
    index_data[index_pos] = '\0';
    close(sock);

    num_packages = 0;
    char *line = index_data;
    
    while (*line && num_packages < 128) {
        char *next = strchr(line, '\n');
        if (next) *next = '\0';

        char name[64] = "", file[64] = "";
        int i = 0, j = 0;
        
        while(line[i] && line[i] != ' ' && j < 63) name[j++] = line[i++];
        name[j] = '\0';
        
        while(line[i] && line[i] != ' ') i++; 
        while(line[i] == ' ') i++;
        
        j = 0;
        while(line[i] && line[i] != '\r' && line[i] != '\n' && j < 63) file[j++] = line[i++];
        file[j] = '\0';

        if (name[0] && file[0] && strstr(name, "<") == NULL) { 
            strcpy(packages[num_packages].name, name);
            strcpy(packages[num_packages].filename, file);
            num_packages++;
        }
        if (!next) break;
        line = next + 1;
    }

    free(index_data); 
    update_installed_status();
}

void install_package(gui_window_t *win, int pkg_idx) {
    char out_path[128];
    sprintf(out_path, "/path/%s", packages[pkg_idx].filename);
    
    char msg[128];
    sprintf(msg, "Downloading %s...", packages[pkg_idx].name);
    draw_overlay(win, msg);

    int fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) { 
        draw_overlay(win, "FS Error: Read-Only?");
        for(int i=0; i<50; i++) yield();
        return; 
    }

    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest)); dest.sin_family = AF_INET; dest.sin_port = htons(REPO_PORT); dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        draw_overlay(win, "Connection Failed!");
        for(int i=0; i<50; i++) yield();
        close(fd); close(sock); return;
    }

    char request[256];
    sprintf(request, "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", packages[pkg_idx].filename, REPO_HOST);
    send(sock, request, strlen(request), 0);

    char buf[BUFFER_SIZE]; int bytes; int header_passed = 0; int total = 0;
    while ((bytes = recv(sock, buf, BUFFER_SIZE, 0)) > 0) {
        char *data_ptr = buf; int data_len = bytes;
        if (!header_passed) {
            for (int i = 0; i < bytes - 3; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
                    header_passed = 1; data_ptr = buf + i + 4; data_len = bytes - (i + 4); break;
                }
            }
            if (!header_passed) continue;
        }
        if (data_len > 0) {
            write(fd, data_ptr, data_len);
            total += data_len;
            sprintf(msg, "Downloading: %d KB", total / 1024);
            draw_overlay(win, msg);
        }
    }
    close(fd); close(sock);

    packages[pkg_idx].is_installed = 1;
    draw_overlay(win, "Installation Complete!");
    for(int i=0; i<50; i++) yield();
}

void uninstall_package_local(gui_window_t *win, int pkg_idx) {
    char out_path[128];
    sprintf(out_path, "/path/%s", packages[pkg_idx].filename);
    
    if (delete_file(out_path) == 1) {
        packages[pkg_idx].is_installed = 0;
        draw_overlay(win, "Uninstalled locally!");
    } else {
        draw_overlay(win, "Failed to delete file!");
    }
    for(int i=0; i<50; i++) yield();
}

void remove_package_server(gui_window_t *win, int pkg_idx) {
    draw_overlay(win, "Removing from server...");

    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest)); dest.sin_family = AF_INET; dest.sin_port = htons(REPO_PORT); dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        draw_overlay(win, "Connection Failed!");
        for(int i=0; i<50; i++) yield();
        close(sock); return;
    }

    char header[256];
    sprintf(header, "POST /remove HTTP/1.0\r\nHost: %s\r\nContent-Length: %d\r\n\r\n", 
            REPO_HOST, (int)strlen(packages[pkg_idx].name));

    send(sock, header, strlen(header), 0);
    send(sock, packages[pkg_idx].name, strlen(packages[pkg_idx].name), 0);

    char resp_buf[128];
    while (recv(sock, resp_buf, sizeof(resp_buf), 0) > 0) yield();
    close(sock);

    draw_overlay(win, "Removed from Server!");
    for(int i=0; i<50; i++) yield();
    
    fetch_packages(win);
}

void upload_package(gui_window_t *win) {
    if (up_path_buf[0] == '\0' || up_name_buf[0] == '\0') {
        draw_overlay(win, "Fill all fields!");
        for(int i=0; i<50; i++) yield();
        return;
    }

    int local_fd = open(up_path_buf, O_RDONLY);
    if (local_fd < 0) { 
        draw_overlay(win, "File not found!");
        for(int i=0; i<50; i++) yield();
        return; 
    }
    
    int file_size = get_file_size(up_path_buf);
    uint8_t* file_buf = malloc(file_size);
    read_file(up_path_buf, file_buf);
    close(local_fd);

    draw_overlay(win, "Connecting to upload...");

    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest; memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET; dest.sin_port = htons(REPO_PORT); dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        draw_overlay(win, "Connection Failed!");
        for(int i=0; i<50; i++) yield();
        free(file_buf); close(sock); return;
    }

    const char* filename = strrchr(up_path_buf, '/');
    if (filename) filename++; else filename = up_path_buf;

    char header[512];
    sprintf(header, "POST /upload HTTP/1.0\r\nHost: %s\r\nX-File-Name: %s\r\nX-Package-Name: %s\r\nContent-Length: %d\r\n\r\n", 
            REPO_HOST, filename, up_name_buf, file_size);

    send(sock, header, strlen(header), 0);
    
    int sent = 0;
    char msg[128];
    while(sent < file_size) {
        int chunk = file_size - sent;
        if (chunk > 2048) chunk = 2048;
        int res = send(sock, file_buf + sent, chunk, 0);
        if (res > 0) {
            sent += res;
            sprintf(msg, "Uploading: %d%%", (sent * 100) / file_size);
            draw_overlay(win, msg);
        } else yield();
    }

    char resp_buf[128];
    while (recv(sock, resp_buf, sizeof(resp_buf), 0) > 0) yield();

    free(file_buf); close(sock);
    
    draw_overlay(win, "Upload Successful!");
    for(int i=0; i<50; i++) yield();

    up_path_buf[0] = '\0'; up_name_buf[0] = '\0';
    fetch_packages(win);
}

void draw_sidebar_item(gui_window_t *win, int y, const char *title, int tab_idx, int icon) {
    int hovered = (win->mx >= 10 && win->mx <= 140 && win->my >= y && win->my <= y + 30);
    if (hovered && win->clicked) { current_tab = tab_idx; scroll_y = 0; }
    
    if (current_tab == tab_idx) {
        gui_draw_rounded_rect(win, 10, y, 130, 30, 6, 0x003A3A3C);
        gui_draw_char(win, 20, y + 11, icon, C_ACCENT);
        gui_draw_string(win, 35, y + 11, title, C_TEXT_PRI);
    } else {
        if (hovered) gui_draw_rounded_rect(win, 10, y, 130, 30, 6, 0x002A2A2C);
        gui_draw_char(win, 20, y + 11, icon, C_TEXT_SEC);
        gui_draw_string(win, 35, y + 11, title, C_TEXT_SEC);
    }
}

void draw_package_list(gui_window_t *win) {
    int start_x = 170;
    int base_y = (current_tab == TAB_SEARCH) ? 60 : 20;
    int start_y = base_y - scroll_y;
    int card_w = win->w - start_x - 20; 
    
    if (current_tab == TAB_SEARCH && search_buf[0] == '\0') {
        gui_draw_string(win, start_x, 70, "Type to search...", C_TEXT_SEC);
        return;
    }

    char search_low[64];
    if (current_tab == TAB_SEARCH) str_to_lower(search_low, search_buf);

    int drawn = 0;
    for (int i = 0; i < num_packages; i++) {
        if (current_tab == TAB_SEARCH) {
            char name_low[64]; str_to_lower(name_low, packages[i].name);
            if (strstr(name_low, search_low) == NULL) continue;
        }

        int cy = start_y + (drawn * 70);
        drawn++;

        if (cy + 60 < 0 || cy > win->h) continue;

        gui_draw_rounded_rect(win, start_x, cy, card_w, 60, 8, C_CARD_BG);
        
        gui_draw_rounded_rect(win, start_x + 10, cy + 10, 40, 40, 8, C_ACCENT);
        gui_draw_char(win, start_x + 26, cy + 26, packages[i].name[0] ? packages[i].name[0] : '?', C_TEXT_PRI);

        gui_draw_string(win, start_x + 65, cy + 18, packages[i].name, C_TEXT_PRI);
        gui_draw_string(win, start_x + 65, cy + 34, packages[i].filename, C_TEXT_SEC);

        
        int btn1_w = 60;
        int btn1_x = start_x + card_w - btn1_w - 15;
        int btn1_y = cy + 15;
        int btn1_hov = (win->mx >= btn1_x && win->mx <= btn1_x + btn1_w && win->my >= btn1_y && win->my <= btn1_y + 30);

        int btn2_w = 70;
        int btn2_x = btn1_x - btn2_w - 10;
        int btn2_y = cy + 15;
        int btn2_hov = (win->mx >= btn2_x && win->mx <= btn2_x + btn2_w && win->my >= btn2_y && win->my <= btn2_y + 30);

        int btn3_w = 70;
        int btn3_x = packages[i].is_installed ? (btn2_x - btn3_w - 10) : (btn1_x - btn3_w - 10);
        int btn3_y = cy + 15;
        int btn3_hov = (win->mx >= btn3_x && win->mx <= btn3_x + btn3_w && win->my >= btn3_y && win->my <= btn3_y + 30);

        if (packages[i].is_installed) {
            gui_draw_rounded_rect(win, btn1_x, btn1_y, btn1_w, 30, 6, btn1_hov ? 0x00555555 : C_BTN_DISABLED);
            gui_draw_string(win, btn1_x + 14, btn1_y + 11, "OPEN", C_TEXT_PRI);
            if (btn1_hov && win->clicked) {
                char exec_path[128]; sprintf(exec_path, "/path/%s", packages[i].filename);
                spawn(exec_path, NULL, NULL);
            }
            
            gui_draw_rounded_rect(win, btn2_x, btn2_y, btn2_w, 30, 6, btn2_hov ? C_BTN_UNINST_H : C_BTN_UNINST);
            gui_draw_string(win, btn2_x + 11, btn2_y + 11, "UNINST", C_TEXT_PRI);
            if (btn2_hov && win->clicked) uninstall_package_local(win, i);
        } else {
            gui_draw_rounded_rect(win, btn1_x, btn1_y, btn1_w, 30, 6, btn1_hov ? C_ACCENT_HOV : C_ACCENT);
            gui_draw_string(win, btn1_x + 18, btn1_y + 11, "GET", C_TEXT_PRI);
            if (btn1_hov && win->clicked) install_package(win, i);
        }

        gui_draw_rounded_rect(win, btn3_x, btn3_y, btn3_w, 30, 6, btn3_hov ? C_BTN_DELSRV_H : C_BTN_DELSRV);
        gui_draw_string(win, btn3_x + 7, btn3_y + 11, "DEL SRV", C_TEXT_PRI);
        if (btn3_hov && win->clicked) remove_package_server(win, i);
    }
}

int main() {
    gui_window_t *win = gui_create_window("App Store", 600, 450);
    if (!win) return 1;
    gui_set_resizable(win, 1);

    fetch_packages(win);

    while (!win->closed) {
        gui_update(win);

        
        int vis_items = get_visible_count();
        int base_y = (current_tab == TAB_SEARCH) ? 60 : 20;
        int view_h = win->h - base_y;
        int total_h = vis_items * 70;
        int max_s = total_h - view_h + 20; 
        if (max_s < 0) max_s = 0;

        
        if (win->scroll_z != 0 && current_tab != TAB_UPLOAD) {
            scroll_y += win->scroll_z * 20;
            if (scroll_y < 0) scroll_y = 0;
            if (scroll_y > max_s) scroll_y = max_s;
        }

        
        if (win->mbtn & 1) {
            if (win->clicked && current_tab != TAB_UPLOAD && max_s > 0) {
                if (win->mx >= win->w - 15 && win->mx <= win->w) {
                    is_dragging_sb = 1;
                }
            }
            if (is_dragging_sb && max_s > 0) {
                int track_y = base_y;
                int track_h = win->h - track_y;
                int sh = (track_h * track_h) / total_h;
                if (sh < 20) sh = 20;
                
                int rel_y = win->my - track_y - (sh / 2);
                if (rel_y < 0) rel_y = 0;
                if (rel_y > track_h - sh) rel_y = track_h - sh;
                
                scroll_y = (rel_y * max_s) / (track_h - sh);
            }
        } else {
            is_dragging_sb = 0;
        }

        
        gui_draw_rect(win, 0, 0, 150, win->h, C_SIDEBAR_BG);
        gui_draw_rect(win, 150, 0, win->w - 150, win->h, C_MAIN_BG);
        gui_draw_rect(win, 150, 0, 1, win->h, C_DIVIDER);

        
        gui_draw_string(win, 15, 20, "Store", C_TEXT_SEC);
        draw_sidebar_item(win, 45, "Discover", TAB_DISCOVER, '*');
        draw_sidebar_item(win, 80, "Search", TAB_SEARCH, '?');
        draw_sidebar_item(win, 115, "Publish", TAB_UPLOAD, '^');

        
        int ref_y = win->h - 40;
        int ref_hov = (win->mx >= 10 && win->mx <= 140 && win->my >= ref_y && win->my <= ref_y + 30);
        if (ref_hov && win->clicked) {
            fetch_packages(win);
            scroll_y = 0;
        }
        if (ref_hov) gui_draw_rounded_rect(win, 10, ref_y, 130, 30, 6, 0x002A2A2C);
        gui_draw_char(win, 20, ref_y + 11, 'R', C_TEXT_PRI);
        gui_draw_string(win, 35, ref_y + 11, "Refresh", C_TEXT_PRI);

        
        if (current_tab == TAB_DISCOVER || current_tab == TAB_SEARCH) {
            if (current_tab == TAB_SEARCH) {
                gui_draw_rect(win, 151, 0, win->w - 151, 50, C_MAIN_BG);
                int search_w = win->w - 200;
                if (search_w < 150) search_w = 150;
                gui_textfield_dark(win, 170, 15, search_w, 30, search_buf, 30, &search_focus);
                if (search_buf[0] == '\0' && !search_focus) gui_draw_string(win, 180, 23, "Search apps...", C_TEXT_SEC);
            }
            
            draw_package_list(win);

            
            if (max_s > 0) {
                int track_y = base_y;
                int track_h = win->h - track_y;
                int sh = (track_h * track_h) / total_h;
                if (sh < 20) sh = 20;
                
                int sb_y = track_y + (scroll_y * (track_h - sh)) / max_s;
                uint32_t sb_col = is_dragging_sb ? 0x00888888 : 0x00555555;
                gui_draw_rounded_rect(win, win->w - 10, sb_y, 8, sh, 4, sb_col);
            }
        }
        else if (current_tab == TAB_UPLOAD) {
            int cx = 170;
            int field_w = win->w - cx - 20;
            if (field_w > 350) field_w = 350;

            gui_draw_string(win, cx, 40, "Publish Your App", C_TEXT_PRI);
            gui_draw_string(win, cx, 70, "1. Local File Path (e.g. /sys/myapp.elf)", C_TEXT_SEC);
            gui_textfield_dark(win, cx, 90, field_w, 30, up_path_buf, 60, &up_path_focus);
            
            gui_draw_string(win, cx, 140, "2. Package Display Name (e.g. MyApp)", C_TEXT_SEC);
            gui_textfield_dark(win, cx, 160, field_w, 30, up_name_buf, 30, &up_name_focus);

            int btn_y = 220;
            int btn_hov = (win->mx >= cx && win->mx <= cx + 150 && win->my >= btn_y && win->my <= btn_y + 35);
            gui_draw_rounded_rect(win, cx, btn_y, 150, 35, 6, btn_hov ? C_ACCENT_HOV : C_ACCENT);
            gui_draw_string(win, cx + 50, btn_y + 14, "PUBLISH", C_TEXT_PRI);

            if (btn_hov && win->clicked) upload_package(win);
        }

        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}
