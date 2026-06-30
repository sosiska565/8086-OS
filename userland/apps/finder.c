/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/finder.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

#define MAX_FILES 128
#define MAX_DISKS 8


#define C_SIDEBAR_BG   0x00282828
#define C_MAIN_BG      0x001E1E1E
#define C_TOOLBAR_BG   0x002D2D2D
#define C_DIVIDER      0x00111111
#define C_TEXT_PRI     0x00EBEBEB
#define C_TEXT_SEC     0x00888888
#define C_ACCENT       0x000A84FF
#define C_SEL_BG       0x003A3A3C
#define C_FOLDER       0x000A84FF
#define C_FILE         0x0098989D
#define C_CTX_BG       0x00252525
#define C_CTX_BORDER   0x00444444

typedef struct { char dev_name[16]; char mount_path[32]; int is_mounted; } DiskInfo;
DiskInfo disks[MAX_DISKS]; int num_disks = 0;

vfs_dirent_t files[MAX_FILES]; int num_files = 0;
char current_path[256] = "/";

int scroll_y = 0; 
int max_visible = 1; 
int is_dragging_scroll = 0; 

int ctx_active = 0, ctx_x = 0, ctx_y = 0, ctx_selected = -1;
char clip_path[256] = ""; int clip_action = 0;
char search_buf[32] = ""; int search_focus = 0; 


void init_finder() {
    int trash_exists = 0;
    vfs_dirent_t ent; int idx = 0;
    while(readdir("/", idx++, &ent) == 1) {
        if (strcmp(ent.name, "TRASH") == 0) {
            trash_exists = 1;
            break;
        }
    }
    if (!trash_exists) mkdir("/TRASH"); 
}

void parse_disks() {
    num_disks = 0; int sz = get_file_size("/proc/disks"); if (sz <= 0) return;
    char *buf = malloc(sz + 1); read_file("/proc/disks", (uint8_t*)buf); buf[sz] = '\0';
    char *line = buf;
    while (*line) {
        char *next = strchr(line, '\n'); if (next) *next = '\0';
        if (strncmp(line, "sd", 2) == 0 || strncmp(line, "hd", 2) == 0) {
            char dev_name[16]; int i = 0;
            while(line[i] && line[i] != '\t' && i < 15) { dev_name[i] = line[i]; i++; } dev_name[i] = '\0';
            sprintf(disks[num_disks].dev_name, "/dev/%s", dev_name);
            sprintf(disks[num_disks].mount_path, "/mnt/%s", dev_name);
            disks[num_disks].is_mounted = (strcmp(dev_name, "sda") == 0);
            num_disks++;
        }
        if (!next) break; line = next + 1;
    }
    free(buf);
}

void load_directory() {
    num_files = 0; scroll_y = 0; ctx_active = 0;
    vfs_dirent_t entry; int idx = 0;
    if (strcmp(current_path, "/") != 0) { strcpy(files[num_files].name, ".."); files[num_files].type = VFS_ATTR_DIR; num_files++; }
    while (readdir(current_path, idx++, &entry) == 1 && num_files < MAX_FILES) files[num_files++] = entry;
}

int str_contains_nocase(const char *haystack, const char *needle) {
    if (!*needle) return 1;
    char h[64], n[64];
    strncpy(h, haystack, 63); to_lower(h); strncpy(n, needle, 63); to_lower(n);
    return strstr(h, n) != NULL;
}

void do_copy(char* src, char* dst) { int sz = get_file_size(src); if (sz >= 0) { uint8_t* b = malloc(sz); read_file(src, b); write_file(dst, b, sz); free(b); } }

int main() {
    init_finder(); parse_disks();
    gui_window_t *win = gui_create_window("Finder", 500, 360);
    if (!win) return 1;

    gui_set_resizable(win, 1);
    load_directory();

    while (!win->closed) {
        gui_update(win);

        max_visible = (win->h - 40) / 24; 
        if (max_visible < 1) max_visible = 1;
        
        if (scroll_y > num_files - max_visible) scroll_y = num_files - max_visible;
        if (scroll_y < 0) scroll_y = 0;

        if (!ctx_active) {
            if (win->key_code == KEY_DOWN && scroll_y < num_files - max_visible) scroll_y++;
            if (win->key_code == KEY_UP && scroll_y > 0) scroll_y--;
            if (win->scroll_z != 0) {
                scroll_y -= win->scroll_z; 
                if (scroll_y > num_files - max_visible) scroll_y = num_files - max_visible;
                if (scroll_y < 0) scroll_y = 0;
                win->scroll_z = 0; 
            }
        }

        if (win->mbtn & 1) { 
            if (win->clicked && win->mx >= win->w - 15) is_dragging_scroll = 1; 
            if (is_dragging_scroll && num_files > max_visible) {
                int track_h = win->h - 40;
                int rel_y = win->my - 40;
                if (rel_y < 0) rel_y = 0;
                if (rel_y > track_h) rel_y = track_h;
                scroll_y = (rel_y * (num_files - max_visible)) / track_h;
            }
        } else is_dragging_scroll = 0; 

        if ((win->clicked || (win->mbtn & 2)) && !is_dragging_scroll) {
            if (ctx_active) {
                if (win->clicked && win->mx >= ctx_x && win->mx <= ctx_x + 120 && win->my >= ctx_y && win->my <= ctx_y + 80) {
                    int menu_item = (win->my - ctx_y) / 20;
                    char target_file[256];
                    if (ctx_selected >= 0) sprintf(target_file, "%s%s%s", current_path, strcmp(current_path,"/")==0?"":"/", files[ctx_selected].name);
                    
                    if (menu_item == 0 && ctx_selected >= 0) { strcpy(clip_path, target_file); clip_action = 1; } 
                    else if (menu_item == 1 && ctx_selected >= 0) { strcpy(clip_path, target_file); clip_action = 2; } 
                    else if (menu_item == 2 && clip_action != 0) { 
                        char new_dst[256]; char *fname = strrchr(clip_path, '/'); fname = fname ? fname + 1 : clip_path;
                        sprintf(new_dst, "%s%s%s", current_path, strcmp(current_path,"/")==0?"":"/", fname);
                        do_copy(clip_path, new_dst); if (clip_action == 2) delete_file(clip_path); clip_action = 0; load_directory();
                    } else if (menu_item == 3 && ctx_selected >= 0) { 
                        char trash_dst[256]; char *fname = strrchr(target_file, '/'); fname = fname ? fname + 1 : target_file;
                        sprintf(trash_dst, "/TRASH/%s", fname); do_copy(target_file, trash_dst); delete_file(target_file); load_directory();
                    }
                }
                ctx_active = 0;
            } else {
                if (win->mbtn & 2) { 
                    if (win->mx > 120 && win->my > 40) {
                        int draw_count = 0;
                        ctx_selected = -1;
                        for (int i = 0; i < num_files; i++) {
                            if (search_buf[0] != '\0' && !str_contains_nocase(files[i].name, search_buf)) continue;
                            if (draw_count >= scroll_y && draw_count < scroll_y + max_visible) {
                                int item_y = 40 + ((draw_count - scroll_y) * 24);
                                if (win->my >= item_y && win->my < item_y + 24) {
                                    if (strcmp(files[i].name, "..") != 0) ctx_selected = i;
                                    break;
                                }
                            }
                            draw_count++;
                        }
                        ctx_active = 1; ctx_x = win->mx; ctx_y = win->my;
                        if (ctx_y > win->h - 80) ctx_y = win->h - 80;
                    }
                } else if (win->clicked) { 
                    if (win->mx < 120) {
                        if (win->my >= 50 && win->my <= 70) { strcpy(current_path, "/"); load_directory(); search_buf[0]='\0'; }
                        if (win->my >= 75 && win->my <= 95) { strcpy(current_path, "/TRASH"); load_directory(); search_buf[0]='\0'; }
                        for (int i = 0; i < num_disks; i++) {
                            int dy = 140 + i * 25;
                            if (win->my >= dy && win->my <= dy + 20) {
                                if (!disks[i].is_mounted) { mkdir(disks[i].mount_path); if (mount(disks[i].dev_name, disks[i].mount_path, "fat32") == 0) disks[i].is_mounted = 1; }
                                if (disks[i].is_mounted) { strcpy(current_path, disks[i].mount_path); load_directory(); search_buf[0]='\0'; }
                            }
                        }
                    } else if (win->mx > 120 && win->my > 40 && win->mx < win->w - 15) { 
                        int draw_count = 0;
                        for (int i = 0; i < num_files; i++) {
                            if (search_buf[0] != '\0' && !str_contains_nocase(files[i].name, search_buf)) continue;
                            if (draw_count >= scroll_y && draw_count < scroll_y + max_visible) {
                                int item_y = 40 + ((draw_count - scroll_y) * 24);
                                if (win->my >= item_y && win->my < item_y + 24) {
                                    if (files[i].type == VFS_ATTR_DIR) {
                                        if (strcmp(files[i].name, "..") == 0) { char* ls = strrchr(current_path, '/'); if (ls && ls != current_path) *ls = '\0'; else strcpy(current_path, "/"); } 
                                        else { if (strcmp(current_path, "/") != 0) strcat(current_path, "/"); strcat(current_path, files[i].name); }
                                        load_directory(); search_buf[0] = '\0'; 
                                    } else {
                                        int len = strlen(files[i].name);
                                        if (len > 4 && strcmp(files[i].name + len - 4, ".elf") == 0) {
                                            char exe[256]; sprintf(exe, "%s%s%s", current_path, strcmp(current_path,"/")==0?"":"/", files[i].name); spawn(exe, NULL, NULL);
                                        }
                                    }
                                    break;
                                }
                            }
                            draw_count++;
                        }
                    }
                }
            }
        }

        
        gui_draw_rect(win, 0, 0, 120, win->h, C_SIDEBAR_BG);
        gui_draw_rect(win, 120, 0, win->w - 120, win->h, C_MAIN_BG);
        gui_draw_rect(win, 120, 0, win->w - 120, 35, C_TOOLBAR_BG);
        gui_draw_rect(win, 120, 35, win->w - 120, 1, C_DIVIDER);
        gui_draw_rect(win, 120, 0, 1, win->h, C_DIVIDER);
        
        
        gui_draw_string(win, 10, 20, "Favorites", C_TEXT_SEC);
        gui_draw_string(win, 20, 55, "Root Area", C_TEXT_PRI);
        gui_draw_string(win, 20, 80, "Trash Bin", C_TEXT_PRI);

        gui_draw_string(win, 10, 120, "Drives", C_TEXT_SEC);
        for(int i=0; i<num_disks; i++) {
            int dy = 140 + i * 25;
            gui_draw_string(win, 20, dy + 5, disks[i].dev_name + 5, C_TEXT_PRI);
            gui_draw_circle_filled(win, 80, dy + 8, 4, disks[i].is_mounted ? 0x0034C759 : 0x00FF9F0A);
        }

        
        gui_draw_string(win, 140, 14, current_path, C_TEXT_PRI);
        gui_textfield_dark(win, win->w - 130, 6, 120, 22, search_buf, 15, &search_focus);
        if (search_buf[0] == '\0' && !search_focus) gui_draw_string(win, win->w - 120, 12, "Search", 0x00888888);

        
        int draw_count = 0;
        for (int i = 0; i < num_files; i++) {
            if (search_buf[0] != '\0' && !str_contains_nocase(files[i].name, search_buf)) continue;
            
            if (draw_count >= scroll_y && draw_count < scroll_y + max_visible) {
                int item_y = 40 + ((draw_count - scroll_y) * 24);
                
                int is_hovered = (win->mx > 120 && win->mx < win->w - 15 && win->my >= item_y && win->my < item_y + 24);
                if (is_hovered && !ctx_active) {
                    gui_draw_rounded_rect(win, 125, item_y, win->w - 145, 24, 4, C_SEL_BG);
                }

                gui_draw_rounded_rect(win, 130, item_y + 4, 16, 12, 2, (files[i].type == VFS_ATTR_DIR) ? C_FOLDER : C_FILE);
                gui_draw_string(win, 155, item_y + 8, files[i].name, C_TEXT_PRI);
            }
            draw_count++;
        }

        
        if (draw_count > max_visible) {
            gui_draw_rect(win, win->w - 10, 40, 10, win->h - 40, C_MAIN_BG);
            int track_h = win->h - 40;
            int scroll_h = ((max_visible * 100) / draw_count) * track_h / 100;
            if (scroll_h < 20) scroll_h = 20; 
            
            int scroll_pos = 40 + (scroll_y * (track_h - scroll_h)) / (draw_count - max_visible);
            uint32_t sb_color = is_dragging_scroll ? 0x00888888 : 0x00555555;
            gui_draw_rounded_rect(win, win->w - 8, scroll_pos, 6, scroll_h, 3, sb_color);
        }

        
        if (ctx_active) {
            gui_draw_rounded_rect(win, ctx_x, ctx_y, 120, 80, 4, C_CTX_BORDER);
            gui_draw_rounded_rect(win, ctx_x + 1, ctx_y + 1, 118, 78, 3, C_CTX_BG);
            
            int menu_item = -1;
            if (win->mx >= ctx_x && win->mx <= ctx_x + 120 && win->my >= ctx_y && win->my <= ctx_y + 80) {
                menu_item = (win->my - ctx_y) / 20;
            }
            
            for(int mi=0; mi<4; mi++) {
                if (menu_item == mi) gui_draw_rounded_rect(win, ctx_x + 2, ctx_y + 2 + mi*20, 116, 16, 2, C_SEL_BG);
            }

            gui_draw_string(win, ctx_x + 10, ctx_y + 6,  "Copy", ctx_selected >= 0 ? C_TEXT_PRI : C_TEXT_SEC);
            gui_draw_string(win, ctx_x + 10, ctx_y + 26, "Cut", ctx_selected >= 0 ? C_TEXT_PRI : C_TEXT_SEC);
            gui_draw_string(win, ctx_x + 10, ctx_y + 46, "Paste", clip_action != 0 ? C_TEXT_PRI : C_TEXT_SEC);
            gui_draw_string(win, ctx_x + 10, ctx_y + 66, "To Trash", ctx_selected >= 0 ? 0xFF453A : C_TEXT_SEC);
        }

        gui_render(win);
        yield();
    }

    for(int i=0; i<num_disks; i++) if(disks[i].is_mounted && strcmp(disks[i].dev_name, "/dev/sda") != 0) unmount(disks[i].mount_path);
    gui_destroy_window(win);
    return 0;
}
