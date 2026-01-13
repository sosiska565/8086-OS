#include "drivers/video/vesa.h"
#include "multiboot.h"
#include "drivers/vga/vga.h"
#include "global.h"
#include "drivers/video/bga/font.h"

uint32_t *video_memory = 0;
int screen_width = 0;
int screen_height = 0;
int screen_pitch = 0;
int screen_bpp = 0;

void init_vesa(void) {
    if (mbi->flags & (1 << 12)) {
        video_memory = (uint32_t*)(uint32_t)mbi->framebuffer_addr;
        screen_width = mbi->framebuffer_width;
        screen_height = mbi->framebuffer_height;
        screen_pitch = mbi->framebuffer_pitch;
        screen_bpp = mbi->framebuffer_bpp;
    }
}

void put_pixel(int x, int y, uint32_t color) {
    if (video_memory == 0) return;
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) return;
    uint8_t *pixel_addr = (uint8_t*)video_memory + (y * screen_pitch) + (x * (screen_bpp / 8));
    *(uint32_t*)pixel_addr = color;
}

void clear_screen_vesa(uint32_t color) {
    if (video_memory == 0) return;
    
    for(int y = 0; y < screen_height; y++) {
        for(int x = 0; x < screen_width; x++) {
            put_pixel(x, y, color);
        }
    }
}

void vesa_draw_char(int x, int y, char c, uint32_t color, uint32_t bgcolor) {
    if (video_memory == 0) return;

    uint8_t *glyph = font8x8_basic[(int)((unsigned char)c)];

    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];
        
        if ((y + row) >= screen_height) break;

        for (int col = 0; col < 8; col++) {
            if ((x + col) >= screen_width) break;

            int is_pixel_set = (line >> col) & 1;
            
            int draw_x = x + col; 
            int draw_y = y + row;

            if (is_pixel_set) {
                put_pixel(draw_x, draw_y, color);
            } else {
                put_pixel(draw_x, draw_y, bgcolor);
            }
        }
    }
}

int get_screen_width(void){
    return screen_width;
}

int get_screen_height(void){
    return screen_height;
}