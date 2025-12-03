#ifndef IDT_H
#define IDT_H

#include<stdint.h>

void idt_set_gate(uint8_t num, uint32_t handler);
void idt_install(void);
void idt_init(void);
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
void pic_remap(void);

#endif