/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/doomgeneric_8086os.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */



#include <oslib.h>
#include "libgui.h"
#include "doomkeys.h" 
#include "doomgeneric.h"

gui_window_t *doom_win = NULL;

#define DOOM_KEY_RIGHT_ARROW    KEY_RIGHTARROW
#define DOOM_KEY_LEFT_ARROW     KEY_LEFTARROW
#define DOOM_KEY_UP_ARROW       KEY_UPARROW
#define DOOM_KEY_DOWN_ARROW     KEY_DOWNARROW

#define DOOM_KEY_STRAFE_L       KEY_STRAFE_L
#define DOOM_KEY_STRAFE_R       KEY_STRAFE_R

#define DOOM_KEY_USE            KEY_USE
#define DOOM_KEY_FIRE           KEY_FIRE

#define DOOM_KEY_ESCAPE         KEY_ESCAPE
#define DOOM_KEY_ENTER          KEY_ENTER
#define DOOM_KEY_TAB            KEY_TAB

#define DOOM_KEY_F1             KEY_F1
#define DOOM_KEY_F2             KEY_F2
#define DOOM_KEY_F3             KEY_F3
#define DOOM_KEY_F4             KEY_F4
#define DOOM_KEY_F5             KEY_F5
#define DOOM_KEY_F6             KEY_F6
#define DOOM_KEY_F7             KEY_F7
#define DOOM_KEY_F8             KEY_F8
#define DOOM_KEY_F9             KEY_F9
#define DOOM_KEY_F10            KEY_F10
#define DOOM_KEY_F11            KEY_F11
#define DOOM_KEY_F12            KEY_F12

#define DOOM_KEY_BACKSPACE      KEY_BACKSPACE
#define DOOM_KEY_PAUSE          KEY_PAUSE

#define DOOM_KEY_EQUALS         KEY_EQUALS
#define DOOM_KEY_MINUS          KEY_MINUS

#define DOOM_KEY_RSHIFT         KEY_RSHIFT
#define DOOM_KEY_RCTRL          KEY_RCTRL
#define DOOM_KEY_RALT           KEY_RALT
#define DOOM_KEY_LALT           KEY_LALT

#define DOOM_KEY_CAPSLOCK       KEY_CAPSLOCK
#define DOOM_KEY_NUMLOCK        KEY_NUMLOCK
#define DOOM_KEY_SCRLCK         KEY_SCRLCK
#define DOOM_KEY_PRTSCR         KEY_PRTSCR

#define DOOM_KEY_HOME           KEY_HOME
#define DOOM_KEY_END            KEY_END
#define DOOM_KEY_PGUP           KEY_PGUP
#define DOOM_KEY_PGDN           KEY_PGDN
#define DOOM_KEY_INS            KEY_INS
#define DOOM_KEY_DEL            KEY_DEL

#define DOOM_KEY_KP0            KEYP_0
#define DOOM_KEY_KP1            KEYP_1
#define DOOM_KEY_KP2            KEYP_2
#define DOOM_KEY_KP3            KEYP_3
#define DOOM_KEY_KP4            KEYP_4
#define DOOM_KEY_KP5            KEYP_5
#define DOOM_KEY_KP6            KEYP_6
#define DOOM_KEY_KP7            KEYP_7
#define DOOM_KEY_KP8            KEYP_8
#define DOOM_KEY_KP9            KEYP_9

#define DOOM_KEY_KP_DIVIDE      KEYP_DIVIDE
#define DOOM_KEY_KP_PLUS        KEYP_PLUS
#define DOOM_KEY_KP_MINUS       KEYP_MINUS
#define DOOM_KEY_KP_MULTIPLY    KEYP_MULTIPLY
#define DOOM_KEY_KP_PERIOD      KEYP_PERIOD
#define DOOM_KEY_KP_EQUALS      KEYP_EQUALS
#define DOOM_KEY_KP_ENTER       KEYP_ENTER

struct {
    uint8_t scancode;
    unsigned char doom_key;
    int last_state;
} key_map[] = {
    {0x48, DOOM_KEY_UP_ARROW,      0},
    {0x50, DOOM_KEY_DOWN_ARROW,    0},
    {0x4B, DOOM_KEY_LEFT_ARROW,    0},
    {0x4D, DOOM_KEY_RIGHT_ARROW,   0},

    {0x2A, DOOM_KEY_STRAFE_L,      0},
    {0x36, DOOM_KEY_STRAFE_R,      0},

    {0x39, DOOM_KEY_USE,           0},
    {0x1D, DOOM_KEY_FIRE,          0},

    {0x01, DOOM_KEY_ESCAPE,        0},
    {0x1C, DOOM_KEY_ENTER,         0},
    {0x0F, DOOM_KEY_TAB,           0},

    {0x3B, DOOM_KEY_F1,            0},
    {0x3C, DOOM_KEY_F2,            0},
    {0x3D, DOOM_KEY_F3,            0},
    {0x3E, DOOM_KEY_F4,            0},
    {0x3F, DOOM_KEY_F5,            0},
    {0x40, DOOM_KEY_F6,            0},
    {0x41, DOOM_KEY_F7,            0},
    {0x42, DOOM_KEY_F8,            0},
    {0x43, DOOM_KEY_F9,            0},
    {0x44, DOOM_KEY_F10,           0},
    {0x57, DOOM_KEY_F11,           0},
    {0x58, DOOM_KEY_F12,           0},

    {0x0E, DOOM_KEY_BACKSPACE,     0},
    {0x45, DOOM_KEY_PAUSE,         0},    
    {0x0D, DOOM_KEY_EQUALS,        0},
    {0x0C, DOOM_KEY_MINUS,         0},

    {0x36, DOOM_KEY_RSHIFT,        0},
    {0x1D, DOOM_KEY_RCTRL,         0},    
    {0x38, DOOM_KEY_RALT,          0},    
    {0x38, DOOM_KEY_LALT,          0},

    {0x3A, DOOM_KEY_CAPSLOCK,      0},
    {0x45, DOOM_KEY_NUMLOCK,       0},
    {0x46, DOOM_KEY_SCRLCK,        0},
    {0x37, DOOM_KEY_PRTSCR,        0},    

    {0x47, DOOM_KEY_HOME,          0},
    {0x4F, DOOM_KEY_END,           0},
    {0x49, DOOM_KEY_PGUP,          0},
    {0x51, DOOM_KEY_PGDN,          0},
    {0x52, DOOM_KEY_INS,           0},
    {0x53, DOOM_KEY_DEL,           0},

    {0x52, DOOM_KEY_KP0,           0},
    {0x4F, DOOM_KEY_KP1,           0},
    {0x50, DOOM_KEY_KP2,           0},
    {0x51, DOOM_KEY_KP3,           0},
    {0x4B, DOOM_KEY_KP4,           0},
    {0x4C, DOOM_KEY_KP5,           0},
    {0x4D, DOOM_KEY_KP6,           0},
    {0x47, DOOM_KEY_KP7,           0},
    {0x48, DOOM_KEY_KP8,           0},
    {0x49, DOOM_KEY_KP9,           0},

    {0x35, DOOM_KEY_KP_DIVIDE,     0},    
    {0x4E, DOOM_KEY_KP_PLUS,       0},
    {0x4A, DOOM_KEY_KP_MINUS,      0},
    {0x37, DOOM_KEY_KP_MULTIPLY,   0},
    {0x53, DOOM_KEY_KP_PERIOD,     0},
    {0x59, DOOM_KEY_KP_EQUALS,     0},
    {0x1C, DOOM_KEY_KP_ENTER,      0},    

    {0x11, 'w',                    0},
    {0x1E, 'a',                    0},
    {0x1F, 's',                    0},
    {0x20, 'd',                    0},
};


#define QUEUE_SIZE 64
static unsigned char key_queue[QUEUE_SIZE];
static int press_queue[QUEUE_SIZE];
static int q_head = 0, q_tail = 0;

void push_key(unsigned char key, int pressed) {
    if ((q_tail + 1) % QUEUE_SIZE == q_head) return; 
    key_queue[q_tail] = key;
    press_queue[q_tail] = pressed;
    q_tail = (q_tail + 1) % QUEUE_SIZE;
}


void poll_doom_input() {
    int map_size = sizeof(key_map) / sizeof(key_map[0]);
    for (int i = 0; i < map_size; i++) {
        int state = get_key_state(key_map[i].scancode); 
        if (state != key_map[i].last_state) {
            push_key(key_map[i].doom_key, state);
            key_map[i].last_state = state;
        }
    }
}


int DG_GetKey(int* pressed, unsigned char* key) {
    if (q_head == q_tail) return 0;
    *key = key_queue[q_head];
    *pressed = press_queue[q_head];
    q_head = (q_head + 1) % QUEUE_SIZE;
    return 1;
}



void DG_DrawFrame() {
    if (!doom_win) return;

    int win_w = doom_win->w;
    int win_h = doom_win->h;

    
    for (int y = 0; y < win_h; y++) {
        
        int src_y = (y * DOOMGENERIC_RESY) / win_h; 
        uint32_t *src_row = &DG_ScreenBuffer[src_y * DOOMGENERIC_RESX];
        
        for (int x = 0; x < win_w; x++) {
            
            int src_x = (x * DOOMGENERIC_RESX) / win_w; 
            
            
            gui_put_pixel(doom_win, x, y, src_row[src_x]);
        }
    }
    
    gui_render(doom_win);
}



void DG_Init() {
    
}

void DG_SleepMs(uint32_t ms) {
    uint32_t target = get_ticks() + ms;
    while(get_ticks() < target) {
        yield();
    }
}

uint32_t DG_GetTicksMs() {
    return get_ticks();
}

void DG_SetWindowTitle(const char * title) {
    
}



int main(int argc, char **argv) {
    
    doom_win = gui_create_window("DOOM", DOOMGENERIC_RESX, DOOMGENERIC_RESY);
    if (!doom_win) return 1;

    gui_set_resizable(doom_win, 1); 

    doomgeneric_Create(argc, argv);

    while (!doom_win->closed) {
        GUI_Event ev;
        
        
        while (gui_poll_event(doom_win, &ev)) {
            if (ev.type == GUI_EV_CLOSE) {
                doom_win->closed = 1;
            }
            if (ev.type == GUI_EV_RESIZE) {
                
                gui_resize_buffer(doom_win, ev.x, ev.y);
            }
            
            
        }

        
        poll_doom_input();
        
        
        doomgeneric_Tick();
        
        yield();
    }

    gui_destroy_window(doom_win);
    return 0;
}
