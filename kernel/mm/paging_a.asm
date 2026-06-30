;  SPDX-License-Identifier: MIT
;
;  8086-OS/kernel/mm/paging_a.asm
;
;  Copyright (C) 2026  sosiska565
;
;  May be freely distributed as part of 8086-OS.

global _loadPageDirectory
global _enablePaging

section .text

_loadPageDirectory:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8]
    mov cr3, eax

    mov esp, ebp
    pop ebp
    ret

_enablePaging:
    push ebp
    mov ebp, esp
    
    mov eax, cr4
    or eax, 0x00000010
    mov cr4, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    mov esp, ebp
    pop ebp
    ret