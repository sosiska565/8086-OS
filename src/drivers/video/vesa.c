#include "drivers/video/vesa.h"
#include "multiboot.h"
#include "drivers/vga/vga.h"
#include "global.h"
#include "drivers/video/bga/font.h"
#include "memory/memory.h"
#include "utils/utils.h"

uint32_t *video_memory = 0;
uint32_t *back_buffer = 0;

int screen_width = 0;
int screen_height = 0;
int screen_pitch = 0;
int screen_bpp = 0;
int buffer_size_bytes = 0;

void init_vesa(void) {
    if (mbi->flags & (1 << 12)) {
        video_memory = (uint32_t*)(uint32_t)mbi->framebuffer_addr;
        screen_width = mbi->framebuffer_width;
        screen_height = mbi->framebuffer_height;
        screen_pitch = mbi->framebuffer_pitch;
        screen_bpp = mbi->framebuffer_bpp;
        
        buffer_size_bytes = screen_pitch * screen_height;
        
        back_buffer = (uint32_t*)kmalloc(buffer_size_bytes + 4096); 
        
        printf("VESA Init: %dx%d @ %d bpp (Pitch: %d)\n", screen_width, screen_height, screen_bpp, screen_pitch);
    }
}

void put_pixel(int x, int y, uint32_t color, int tar) {
    uint8_t *target = 0;
    if(tar != 0){
        target = (uint8_t*)back_buffer;
    }
     
    if (target == 0) target = (uint8_t*)video_memory;
    // if (target == 0) return;

    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height) return;

    uint8_t *pixel_addr = target + (y * screen_pitch) + (x * (screen_bpp / 8));
    
    if (screen_bpp == 32) {
        *(uint32_t*)pixel_addr = color;
    } 
    else if (screen_bpp == 24) {
        pixel_addr[0] = color & 0xFF;
        pixel_addr[1] = (color >> 8) & 0xFF;
        pixel_addr[2] = (color >> 16) & 0xFF;
    }
}

void clear_screen_vesa(uint32_t color) {
    if (back_buffer == 0) return;
    
    if (color == 0) {
        fast_memset(back_buffer, 0, buffer_size_bytes / 4);
    } else {
        for(int y = 0; y < screen_height; y++) {
            for(int x = 0; x < screen_width; x++) {
                put_pixel(x, y, color, 1);
            }
        }
    }
}

void vesa_render_buffer() {
    if (video_memory == 0 || back_buffer == 0) return;
    
    fast_memcpy(video_memory, back_buffer, buffer_size_bytes);
}

void vesa_render_rect(int x, int y, int w, int h) {
    if (video_memory == 0 || back_buffer == 0) return;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > screen_width) w = screen_width - x;
    if (y + h > screen_height) h = screen_height - y;
    if (w <= 0 || h <= 0) return;
    
    int bpp_bytes = screen_bpp / 8;
    uint32_t line_len = w * bpp_bytes;

    uint8_t *src_start = (uint8_t*)back_buffer + (y * screen_pitch) + (x * bpp_bytes);
    uint8_t *dst_start = (uint8_t*)video_memory + (y * screen_pitch) + (x * bpp_bytes);

    for (int i = 0; i < h; i++) {
        fast_memcpy(dst_start, src_start, line_len);
        
        src_start += screen_pitch;
        dst_start += screen_pitch;
    }
}

void vesa_draw_char(int x, int y, unsigned int c, uint32_t color, uint32_t bgcolor) {
    int idx = (int)c;
    if(idx >= 1104) idx = '?'; 
    uint8_t *glyph = font8x8_basic[idx];

    int bpp_bytes = screen_bpp / 8;
    uint8_t *target = back_buffer ? (uint8_t*)back_buffer : (uint8_t*)video_memory;

    for (int row = 0; row < 8; row++) {
        if ((y + row) >= screen_height) break;
        uint8_t line = glyph[row];
        uint8_t *pixel_addr = target + ((y + row) * screen_pitch) + (x * bpp_bytes);
        
        for (int col = 0; col < 8; col++) {
            if ((x + col) >= screen_width) break;
            
            if ((line >> col) & 1) {
                if (bpp_bytes == 4) *(uint32_t*)pixel_addr = color;
                else { pixel_addr[0] = color; pixel_addr[1] = color>>8; pixel_addr[2] = color>>16; }
            } else if ((bgcolor & 0xFF000000) == 0) {
                if (bpp_bytes == 4) *(uint32_t*)pixel_addr = bgcolor;
                else { pixel_addr[0] = bgcolor; pixel_addr[1] = bgcolor>>8; pixel_addr[2] = bgcolor>>16; }
            }
            pixel_addr += bpp_bytes;
        }
    }
}

int get_screen_width(void){ return screen_width; }
int get_screen_height(void){ return screen_height; }