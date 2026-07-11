/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/filepicker.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"

#define MAX_FILES 128

vfs_dirent_t files[MAX_FILES];
int num_files = 0;
char current_path[256] = "/";
int scroll_y = 0;
int selected_idx = -1;

char filename_buf[64] = "";
int filename_focus = 0;

void load_directory() {
    num_files = 0;
    scroll_y = 0;
    selected_idx = -1;
    vfs_dirent_t entry;
    int idx = 0;
    
    if (strcmp(current_path, "/") != 0) {
        strcpy(files[num_files].name, "..");
        files[num_files].type = VFS_ATTR_DIR;
        num_files++;
    }
    
    while(readdir(current_path, idx++, &entry) == 1 && num_files < MAX_FILES) {
        files[num_files++] = entry;
    }
}

int main(int argc, char** argv) {
    gui_window_t* win = gui_create_window("Select File", 450, 350);
    if (!win) return 1;
    gui_set_resizable(win, 0);

    load_directory();
    
    int is_dragging_scroll = 0;

    while(!win->closed) {
        gui_update(win);
        
        int max_visible = 10;
        int max_s = num_files - max_visible;
        if (max_s < 0) max_s = 0;
        
        if (win->scroll_z != 0) {
            scroll_y += win->scroll_z;
            if (scroll_y < 0) scroll_y = 0;
            if (scroll_y > max_s) scroll_y = max_s;
        }

        gui_draw_rect(win, 0, 0, win->w, win->h, 0x00F0F0F0);
        
        gui_draw_rect(win, 0, 0, win->w, 35, 0x00E0E0E0);
        gui_draw_rect(win, 0, 35, win->w, 1, 0x00CCCCCC);
        gui_draw_string(win, 10, 10, current_path, 0x00000000);

        gui_draw_rect(win, 10, 45, win->w - 30, 240, 0x00FFFFFF);
        gui_draw_rect(win, 10, 45, win->w - 30, 240, 0x00A0A0A0); 
        gui_draw_rect(win, 11, 46, win->w - 32, 238, 0x00FFFFFF);

        int draw_count = 0;
        for(int i = 0; i < num_files; i++) {
            if (draw_count >= scroll_y && draw_count < scroll_y + max_visible) {
                int item_y = 48 + ((draw_count - scroll_y) * 23);
                
                int hovered = (win->mx >= 12 && win->mx <= win->w - 36 && win->my >= item_y && win->my < item_y + 23);
                
                if (selected_idx == i) {
                    gui_draw_rect(win, 12, item_y, win->w - 36, 23, 0x00007AFF);
                } else if (hovered) {
                    gui_draw_rect(win, 12, item_y, win->w - 36, 23, 0x00E8E8E8);
                }

                uint32_t text_col = (selected_idx == i) ? 0x00FFFFFF : 0x00000000;
                uint32_t icon_col = (files[i].type == VFS_ATTR_DIR) ? ((selected_idx == i) ? 0x00FFFFFF : 0x00007AFF) : text_col;
                
                gui_draw_string(win, 18, item_y + 4, files[i].type == VFS_ATTR_DIR ? "[DIR]" : "[FILE]", icon_col);
                gui_draw_string(win, 70, item_y + 4, files[i].name, text_col);
                
                if (hovered && win->clicked) {
                    selected_idx = i;
                    if (files[i].type != VFS_ATTR_DIR) {
                        strcpy(filename_buf, files[i].name);
                    }
                    
                    if (files[i].type == VFS_ATTR_DIR) {
                        if (strcmp(files[i].name, "..") == 0) {
                            char* ls = strrchr(current_path, '/');
                            if (ls && ls != current_path) *ls = '\0';
                            else strcpy(current_path, "/");
                        } else {
                            if (strcmp(current_path, "/") != 0) strcat(current_path, "/");
                            strcat(current_path, files[i].name);
                        }
                        load_directory();
                        win->clicked = 0;
                    }
                }
            }
            draw_count++;
        }

        if (max_s > 0) {
            int track_h = 240;
            int thumb_h = (max_visible * track_h) / num_files;
            if (thumb_h < 20) thumb_h = 20;
            int thumb_y = 45 + (scroll_y * (track_h - thumb_h)) / max_s;
            gui_draw_rounded_rect(win, win->w - 26, thumb_y, 10, thumb_h, 4, 0x00A0A0A0);
        }

        gui_draw_string(win, 10, 305, "File:", 0x00000000);
        gui_textfield(win, 50, 295, 200, 30, filename_buf, 60, &filename_focus);

        if (gui_button(win, 260, 295, 80, 30, "Cancel", 0)) {
            break; 
        }

        if (gui_button(win, 350, 295, 80, 30, "Select", 1) || (filename_focus && win->key_code == KEY_ENTER)) {
            if (strlen(filename_buf) > 0) {
                char final_path[256];
                if (strcmp(current_path, "/") == 0) sprintf(final_path, "/%s", filename_buf);
                else sprintf(final_path, "%s/%s", current_path, filename_buf);
                
                printf("%s", final_path);
                break;
            }
        }

        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}
