global gdt_install

gdt_install:
    lgdt [gdt_ptr]
    jmp 0x08:flush_cs

flush_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

section .data
gdt_start:
    dd 0
    dd 0
    dw 0xFFFF    
    dw 0x0000    
    db 0x00      
    db 10011010b
    db 11001111b 
    db 0x00      
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start