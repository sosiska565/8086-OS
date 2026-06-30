;  SPDX-License-Identifier: MIT
;
;  8086-OS/kernel/task/switch.asm
;
;  Copyright (C) 2026  sosiska565
;
;  May be freely distributed as part of 8086-OS.

global switch_to_task

switch_to_task:
    pusha
    pushf
    
    mov eax, [esp + 44] 
    mov [eax + 4], esp  

    mov esi, [esp + 40]
    mov esp, [esi + 4]
    
    popf
    popa
    
    ret
