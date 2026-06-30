;  SPDX-License-Identifier: MIT
;
;  8086-OS/userland/lib/entry.asm
;
;  Copyright (C) 2026  sosiska565
;
;  May be freely distributed as part of 8086-OS.

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