/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/gui_protocol.h
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#ifndef GUI_PROTOCOL_H
#define GUI_PROTOCOL_H

#include <stdint.h>


#define WM_SHM_KEY 1000


#define WM_CMD_CREATE_WINDOW 1
#define WM_CMD_DESTROY_WINDOW 2
#define WM_CMD_RESIZE_WINDOW 3 
#define WM_CMD_SET_RESIZABLE 4 
#define WM_CMD_CREATE_FRAMELESS 5
#define WM_CMD_SET_WALLPAPER 6 
#define WM_CMD_SET_SCALE 7 

#define GUI_EV_MOUSE_MOVE 1
#define GUI_EV_MOUSE_CLICK 2
#define GUI_EV_KEY_PRESS 3
#define GUI_EV_CLOSE 4
#define GUI_EV_RESIZE 5
#define GUI_EV_MOUSE_SCROLL 6

typedef struct {
    int type;
    int shm_key; 
    int x, y, w, h;
    char title[32];
    int data;
} WM_Command;

typedef struct {
    volatile int head;
    volatile int tail;
    WM_Command commands[32];
    char active_window_title[32];
    float global_scale;
} WM_Queue;


typedef struct {
    int type;
    int x, y;
    int button;
    int keycode;
} GUI_Event;



typedef struct {
    volatile int event_head;
    volatile int event_tail;
    GUI_Event events[32];
    uint32_t pixels[]; 
} Client_SHM;

#endif
