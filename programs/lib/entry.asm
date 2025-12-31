bits 32
section .text.entry
global _start
extern main
extern exit

_start:
    call main

    ret