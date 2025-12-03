#include "interrupt/idt/idt.h"

extern void ignore_handler();

struct idt_entry{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_entry idt[256];

void idt_set_gate(uint8_t num, uint32_t handler){
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].offset_high = (handler >> 16) & 0xFFFF;

    idt[num].selector = 0x08;
    idt[num].zero = 0;
    idt[num].type_attr = 0x8E;
}

void idt_install(void){
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) idtp;

    idtp.limit = (256 * 8) - 1;
    idtp.base = (uint32_t)&idt;

    for(int i = 0; i < 256; i++){
        idt_set_gate(i, (uint32_t)ignore_handler);
    }

    __asm__ volatile("lidt %0" : : "m"(idtp));
}

void idt_init(void){
    extern void irq0_handler();
    extern void irq1_handler();

    idt_set_gate(32, (uint32_t)irq0_handler);
    idt_set_gate(33, (uint32_t)irq1_handler);
}

void outb(uint16_t port, uint8_t val){
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

uint8_t inb(uint16_t port){
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    
    outb(0x21, 32);
    outb(0xA1, 40);
    
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    
    // outb(0x21, 0xFC); 
    // outb(0xA1, 0xFF);

    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}