global irq0_handler
global irq1_handler

global ignore_handler

extern timer_handler_c
extern keyboard_handler_c

irq0_handler:
    pusha
    call timer_handler_c

    popa
    iret

irq1_handler:
    pusha
    call keyboard_handler_c
    
    popa
    iret

ignore_handler:
    iret