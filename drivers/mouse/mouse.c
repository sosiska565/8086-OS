#include "drivers/mouse/mouse.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "drivers/video/vesa.h"

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

MouseState mouse = {0, 0, 0, 0, 0};
uint8_t mouse_cycle = 0;
uint8_t mouse_byte[3];

static uint8_t mouse_has_wheel = 0;

void mouse_init() {
    extern int screen_width, screen_height;
    mouse.x = screen_width / 2; mouse.y = screen_height / 2; mouse.z = 0;

    uint8_t status;
    mouse_wait(1); outb(0x64, 0xA8); 
    mouse_wait(1); outb(0x64, 0x20); mouse_wait(0); status = inb(0x60);
    status |= 2; status &= ~0x20;
    mouse_wait(1); outb(0x64, 0x60); mouse_wait(1); outb(0x60, status);
    mouse_write(0xF6); mouse_read(); 

    
    mouse_write(0xF3); mouse_read(); mouse_write(200); mouse_read();
    mouse_write(0xF3); mouse_read(); mouse_write(100); mouse_read();
    mouse_write(0xF3); mouse_read(); mouse_write(80); mouse_read();
    
    mouse_write(0xF2); mouse_read();
    uint8_t mouse_id = mouse_read();
    if (mouse_id == 3 || mouse_id == 4) mouse_has_wheel = 1;

    mouse_write(0xF4); mouse_read(); 
}

void mouse_handler_c(void) {
    uint8_t status = inb(0x64);
    if (!(status & 0x01) || !(status & 0x20)) { inb(0x60); return; } 
    
    uint8_t data = inb(0x60);
    if (mouse_cycle == 0 && (data & 0x08) == 0) return; 
    mouse_byte[mouse_cycle++] = data;

    int max_bytes = mouse_has_wheel ? 4 : 3;
    
    if (mouse_cycle == max_bytes) { 
        mouse_cycle = 0;
        mouse.left_button = mouse_byte[0] & 0x01;
        mouse.right_button = (mouse_byte[0] & 0x02) >> 1;
        mouse.middle_button = (mouse_byte[0] & 0x04) >> 2;

        int dx = mouse_byte[1] - ((mouse_byte[0] << 4) & 0x100);
        int dy = mouse_byte[2] - ((mouse_byte[0] << 3) & 0x100);
        
        mouse.x += dx; mouse.y -= dy; 
        
        
        if (mouse_has_wheel) {
            int dz = (int8_t)mouse_byte[3]; 
            mouse.z += dz; 
        }

        extern int screen_width, screen_height;
        if (mouse.x < 0) mouse.x = 0; if (mouse.y < 0) mouse.y = 0;
        if (mouse.x >= screen_width) mouse.x = screen_width - 1;
        if (mouse.y >= screen_height) mouse.y = screen_height - 1;
    }
}