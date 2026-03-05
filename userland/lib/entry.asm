bits 32
section .text.entry
global _start
extern main
extern exit

_start:
    mov ecx, [esp + 4]
    mov edx, [esp + 8]

    push edx
    push ecx

    call main

    add esp, 8

    call exit
    
    ret