#include "idt/idt.h"
#include "drivers/io/io.h"

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
    extern void system_division_handler();
    extern void keyboard_handler();
    extern void timer_handler();
    extern void syscall_handler();
    extern void mouse_handler();
    extern void page_fault_handler();
    extern void invalid_opcode_handler();
    extern void rtl8139_irq_handler();

    idt_set_gate(0, (uint32_t)system_division_handler);
    idt_set_gate(32, (uint32_t)timer_handler);
    idt_set_gate(33, (uint32_t)keyboard_handler);
    idt_set_gate(0x80, (uint32_t)syscall_handler);
    idt_set_gate(44, (uint32_t)mouse_handler);
    idt_set_gate(14, (uint32_t)page_fault_handler);
    idt_set_gate(6, (uint32_t)invalid_opcode_handler);
    idt_set_gate(43, (uint32_t)rtl8139_irq_handler);
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

    // outb(0x21, 0x00);
    // outb(0xA1, 0x00);

    outb(0x21, 0xF8);
    outb(0xA1, 0xEF);

    outb(0x21, inb(0x21) & ~(1 << 2));
    outb(0xA1, inb(0xA1) & ~(1 << 3)); 
}