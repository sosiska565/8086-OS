bits 32
global _start
extern main
extern exit

_start:
    call main
    call exit

    hlt