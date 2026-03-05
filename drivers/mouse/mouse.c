#include "drivers/mouse/mouse.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "drivers/video/vesa.h"
#include "drivers/video/graphics.h"

MouseState mouse = {0, 0, 0, 0, 0};
uint8_t mouse_cycle = 0;
int8_t mouse_byte[3];

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { while (timeout--) { if ((inb(0x64) & 1) == 1) return; } } 
    else { while (timeout--) { if ((inb(0x64) & 2) == 0) return; } }
}

void mouse_write(uint8_t write) {
    mouse_wait(1); outb(0x64, 0xD4);
    mouse_wait(1); outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0); return inb(0x60);
}

void mouse_init() {
    extern int screen_width, screen_height;
    mouse.x = screen_width / 2;
    mouse.y = screen_height / 2;

    uint8_t status;
    mouse_wait(1); outb(0x64, 0xA8);
    mouse_wait(1); outb(0x64, 0x20);
    mouse_wait(0); status = inb(0x60);
    status |= 2; status &= ~0x20;
    mouse_wait(1); outb(0x64, 0x60);
    mouse_wait(1); outb(0x60, status);
    mouse_write(0xF6); 
    mouse_read();
    mouse_write(0xF4); 
    mouse_read();
}

void mouse_handler_c(void) {
    uint8_t status = inb(0x64);
    if (!(status & 0x01)) return; 
    if (!(status & 0x20)) { inb(0x60); return; } 
    
    uint8_t data = inb(0x60);
    
    if (data == 0xFA) return; 

    
    
    if (mouse_cycle == 0 && (data & 0x08) == 0) return;

    mouse_byte[mouse_cycle++] = data;

    if (mouse_cycle == 3) { 
        mouse_cycle = 0;
        
        mouse.left_button = mouse_byte[0] & 0x01;
        mouse.right_button = (mouse_byte[0] & 0x02) >> 1;
        mouse.middle_button = (mouse_byte[0] & 0x04) >> 2;

        int dx = mouse_byte[1];
        if (mouse_byte[0] & 0x10) dx -= 256;

        int dy = mouse_byte[2];
        if (mouse_byte[0] & 0x20) dy -= 256;

        mouse.x += dx;
        mouse.y -= dy; 

        extern int screen_width, screen_height;
        if (mouse.x < 0) mouse.x = 0;
        if (mouse.y < 0) mouse.y = 0;
        if (mouse.x >= screen_width) mouse.x = screen_width - 2;
        if (mouse.y >= screen_height) mouse.y = screen_height - 2;

        wm_update_cursor();
    }
}