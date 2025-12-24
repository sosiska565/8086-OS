global syscall_handler
global system_division_handler
global keyboard_handler
global timer_handler

global ignore_handler

extern syscall_handler_c
extern system_division_handler_c
extern timer_handler_c
extern keyboard_handler_c

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
    call timer_handler_c

    popa
    iret

timer_handler:
    pusha
    call keyboard_handler_c
    
    popa
    iret

ignore_handler:
    iret