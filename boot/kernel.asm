;  SPDX-License-Identifier: MIT
;
;  8086-OS/boot/kernel.asm
;
;  Copyright (C) 2026  sosiska565
;
;  May be freely distributed as part of 8086-OS.



bits 32
section .text
    align 4
    
    MAGIC    equ 0x1BADB002
    FLAGS    equ 0x07 
    CHECKSUM equ -(MAGIC + FLAGS)

    dd MAGIC
    dd FLAGS
    dd CHECKSUM

    dd 0, 0, 0, 0, 0

    dd 0       
    dd 2560 ;
    dd 1440 ;
    dd 32

    ;2560 1440
    ;1280 1024
    ;800 600

global start
extern kmain
extern gdt_install

start:
    cli
    mov esp, stack_space
    
    push ebx
    push eax 
    
    call gdt_install
    call kmain
    
    cli
.hang:
    hlt
    jmp .hang

section .bss
    resb 8192
stack_space:
