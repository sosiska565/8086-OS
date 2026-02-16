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
    dd 900
    dd 900
    dd 32

    ;1280x1024

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