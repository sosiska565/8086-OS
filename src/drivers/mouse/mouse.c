#include "drivers/mouse/mouse.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "drivers/video/vesa.h"

MouseState mouse = {0, 0, 0, 0, 0};

// === Ожидание ===
void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
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

    mouse_write(0xF4);
    mouse_read();
    
    __asm__ volatile("sti");
}

void mouse_handler_c(void) {
    uint8_t status = inb(0x64);

    printf("B");

    if (!(status & 0x01)) return;

    uint8_t input = inb(0x60);

    if (!(status & 0x20)) return;
}