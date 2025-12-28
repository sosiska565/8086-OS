#include "drivers/mouse/mouse.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"

int mouse_x = 40;
int mouse_y = 12;

uint8_t mouse_left_pressed = 0;
uint8_t mouse_right_pressed = 0;

uint8_t mouse_cycle = 0;
int8_t mouse_byte[3];

uint16_t saved_under_mouse = 0x0720;

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) if ((inb(0x64) & 1) == 1) return;
    } else {
        while (timeout--) if ((inb(0x64) & 2) == 0) return;
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init() {
    __asm__ volatile("cli"); 

    uint8_t status;

    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = inb(0x60);
    status |= 2; 
    status &= ~0x20; 
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();

    saved_under_mouse = vga_get_entry(mouse_x, mouse_y);
    vga_set_entry(mouse_x, mouse_y, (saved_under_mouse & 0x00FF) | 0x2000);

    __asm__ volatile("sti");
}

void mouse_handler_c() {
    uint8_t status = inb(0x64);
    
    if (!(status & 1)) return;

    uint8_t input = inb(0x60);

    if (!(status & 0x20)) return;

    mouse_byte[mouse_cycle] = input;
    mouse_cycle++;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        if ((mouse_byte[0] & 0x80) || (mouse_byte[0] & 0x40)) {
            return;
        }

        vga_set_entry(mouse_x, mouse_y, saved_under_mouse);

        int diff_x = (int8_t)mouse_byte[1];
        int diff_y = (int8_t)mouse_byte[2];

        mouse_x += diff_x; 
        mouse_y -= diff_y; 

        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= 79) mouse_x = 79;
        
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= 24) mouse_y = 24;

        mouse_left_pressed = (mouse_byte[0] & 0x01);
        mouse_right_pressed = (mouse_byte[0] & 0x02);

        saved_under_mouse = vga_get_entry(mouse_x, mouse_y);
        
        uint16_t style = (saved_under_mouse & 0x00FF);
        if(mouse_left_pressed) style |= 0x4000;
        else style |= 0x2000;
        
        vga_set_entry(mouse_x, mouse_y, style);
    }
}