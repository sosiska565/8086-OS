;  SPDX-License-Identifier: MIT
;
;  8086-OS/kernel/irq/interrupts_a.asm
;
;  Copyright (C) 2026  sosiska565
;
;  May be freely distributed as part of 8086-OS.

global syscall_handler
global system_division_handler
global keyboard_handler
global timer_handler
global mouse_handler
global page_fault_handler

global ignore_handler
global invalid_opcode_handler
global rtl8139_irq_handler
global signal_trampoline_entry

extern syscall_handler_c
extern system_division_handler_c
extern timer_handler_c
extern keyboard_handler_c
extern mouse_handler_c
extern page_fault_handler_c
extern invalid_opcode_handler_c
extern rtl8139_handler_c

syscall_handler:
    pusha

    push esp
    call syscall_handler_c
    add esp, 4

    popa
    iret

system_division_handler:
    push esp
    
    call system_division_handler_c

    add esp, 4
    iret

keyboard_handler:
    pusha
    call keyboard_handler_c

    mov al, 0x20
    out 0x20, al

    popa
    iret

timer_handler:
    pusha
    push esp
    call timer_handler_c
    add esp, 4
    popa
    iret

mouse_handler:
    pusha
    call mouse_handler_c
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    popa
    iret

rtl8139_irq_handler:
    pusha
    call rtl8139_handler_c
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    popa
    iret

page_fault_handler:
    push 14
    pusha
    
    xor eax, eax
    mov ax, ds
    push eax 

    push esp
    call page_fault_handler_c
    add esp, 4

    pop eax
    popa 
    add esp, 8
    iret

ignore_handler:
    iret

invalid_opcode_handler:
    push 6
    pusha
    push esp
    call invalid_opcode_handler_c
    add esp, 4
    popa
    add esp, 4
    iret

signal_trampoline_entry:
    push ebx
    call eax
    add esp, 4
    mov eax, 119
    int 0x80

.hang:
    jmp .hang