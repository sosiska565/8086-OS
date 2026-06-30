/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/calc.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "libgui.h"


#define C_DISPLAY_BG 0x002D2D2D 
#define C_DISP_TEXT  0x00FFFFFF 
#define C_BTN_NUM    0x005D5D5F 
#define C_BTN_TOP    0x00454546 
#define C_BTN_ORG    0x00FF9F0A 
#define C_BORDER     0x00222222 
#define C_PRESS_NUM  0x007A7A7C 
#define C_PRESS_ORG  0x00C97400 
#define C_PRESS_TOP  0x00666666 

typedef struct {
    int x, y, w, h;
    char label[4];
    int type; 
} CalcBtn;


CalcBtn buttons[] = {
    {0,   80,  58,  48, "AC", 1}, {58,  80,  58,  48, "+/-", 1}, {116, 80,  58,  48, "%", 1}, {174, 80,  58,  48, "/", 2},
    {0,   128, 58,  48, "7",  0}, {58,  128, 58,  48, "8",  0}, {116, 128, 58,  48, "9", 0}, {174, 128, 58,  48, "x", 2},
    {0,   176, 58,  48, "4",  0}, {58,  176, 58,  48, "5",  0}, {116, 176, 58,  48, "6", 0}, {174, 176, 58,  48, "-", 2},
    {0,   224, 58,  48, "1",  0}, {58,  224, 58,  48, "2",  0}, {116, 224, 58,  48, "3", 0}, {174, 224, 58,  48, "+", 2},
    {0,   272, 116, 48, "0",  0}, {116, 272, 58,  48, ".",  0}, {174, 272, 58,  48, "=", 2}
};

int current_val = 0;
int stored_val = 0;
int current_op = 0; 
int new_input = 1;


void gui_draw_string_scaled(gui_window_t *win, int x, int y, const char *str, uint32_t fg, int scale) {
    extern char font8x8_basic[1104][8]; 
    int cx = x;
    while (*str) {
        char c = *str++;
        if ((unsigned char)c > 127) c = '?';
        char *glyph = font8x8_basic[(int)c];
        for (int ry = 0; ry < 8; ry++) {
            for (int rx = 0; rx < 8; rx++) {
                if ((glyph[ry] >> rx) & 1) {
                    gui_draw_rect(win, cx + rx * scale, y + ry * scale, scale, scale, fg);
                }
            }
        }
        cx += 8 * scale;
    }
}

void handle_click(char* label) {
    if (label[0] >= '0' && label[0] <= '9') {
        if (new_input) { current_val = label[0] - '0'; new_input = 0; }
        else current_val = (current_val * 10) + (label[0] - '0');
    } else if (strcmp(label, "AC") == 0) {
        current_val = 0; stored_val = 0; current_op = 0; new_input = 1;
    } else if (strcmp(label, "+/-") == 0) {
        current_val = -current_val;
    } else if (strcmp(label, "%") == 0) {
        current_val = current_val / 100;
    } else if (strcmp(label, "+") == 0) { stored_val = current_val; current_op = 1; new_input = 1; }
    else if (strcmp(label, "-") == 0) { stored_val = current_val; current_op = 2; new_input = 1; }
    else if (strcmp(label, "x") == 0) { stored_val = current_val; current_op = 3; new_input = 1; }
    else if (strcmp(label, "/") == 0) { stored_val = current_val; current_op = 4; new_input = 1; }
    else if (strcmp(label, "=") == 0) {
        if (current_op == 1) current_val = stored_val + current_val;
        else if (current_op == 2) current_val = stored_val - current_val;
        else if (current_op == 3) current_val = stored_val * current_val;
        else if (current_op == 4 && current_val != 0) current_val = stored_val / current_val;
        current_op = 0; new_input = 1;
    }
}

int main() {
    gui_window_t *win = gui_create_window("Calculator", 232, 320);
    if (!win) return 1;
    gui_set_resizable(win, 0);

    while (!win->closed) {
        gui_update(win);

        
        gui_draw_rect(win, 0, 0, win->w, 80, C_DISPLAY_BG);

        
        char display_str[32];
        sprintf(display_str, "%d", current_val);
        int scale = 4;
        int text_w = strlen(display_str) * 8 * scale;
        int text_x = win->w - text_w - 10; 
        int text_y = 80 - (8 * scale) - 10; 
        gui_draw_string_scaled(win, text_x, text_y, display_str, C_DISP_TEXT, scale);

        
        for (int i = 0; i < sizeof(buttons)/sizeof(CalcBtn); i++) {
            CalcBtn b = buttons[i];
            int hovered = (win->mx >= b.x && win->mx <= b.x + b.w && win->my >= b.y && win->my <= b.y + b.h);
            int pressed = hovered && (win->mbtn & 1);

            uint32_t bg_color;
            if (b.type == 0) bg_color = pressed ? C_PRESS_NUM : C_BTN_NUM;
            else if (b.type == 1) bg_color = pressed ? C_PRESS_TOP : C_BTN_TOP;
            else bg_color = pressed ? C_PRESS_ORG : C_BTN_ORG;

            
            gui_draw_rect(win, b.x, b.y, b.w, b.h, bg_color);
            
            
            gui_draw_rect(win, b.x + b.w - 1, b.y, 1, b.h, C_BORDER);
            gui_draw_rect(win, b.x, b.y + b.h - 1, b.w, 1, C_BORDER);

            
            int lbl_scale = 2;
            int label_x = b.x + (b.w - strlen(b.label) * 8 * lbl_scale) / 2;
            int label_y = b.y + (b.h - 8 * lbl_scale) / 2;
            gui_draw_string_scaled(win, label_x, label_y, b.label, C_DISP_TEXT, lbl_scale);

            if (hovered && win->clicked) {
                handle_click(b.label);
            }
        }

        gui_render(win);
        yield();
    }

    gui_destroy_window(win);
    return 0;
}
