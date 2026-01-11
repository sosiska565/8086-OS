#ifndef BGA_H
#define BGA_H

#include <stdint.h>

#define VBE_DISPI_INDEX_ID          0
#define VBE_DISPI_INDEX_XRES        1
#define VBE_DISPI_INDEX_YRES        2
#define VBE_DISPI_INDEX_BPP         3
#define VBE_DISPI_INDEX_ENABLE      4
#define VBE_DISPI_INDEX_BANK        5
#define VBE_DISPI_INDEX_VIRT_WIDTH  6
#define VBE_DISPI_INDEX_VIRT_HEIGHT 7
#define VBE_DISPI_INDEX_X_OFFSET    8
#define VBE_DISPI_INDEX_Y_OFFSET    9

#define VBE_DISPI_DISABLED          0x00
#define VBE_DISPI_ENABLED           0x01
#define VBE_DISPI_LFB_ENABLED       0x40

#define BPP           32 

#define VIDEO_MODE_TEXT 0
#define VIDEO_MODE_GRAPHICS 1

extern int screenW;
extern int screenH;
extern uint32_t *video_memory;

void init_bga(int width, int height);
void put_pixel(int x, int y, uint32_t color);
void clear_screen_bga(uint32_t color);
void set_video_mode(int mode);
void bga_draw_char(int x, int y, char c, uint32_t color, uint32_t bgcolor);
void bga_print_string(int x, int y, char* str, uint32_t color);
int getScreenWidth(void);
int getScreenHeight(void);

#endif