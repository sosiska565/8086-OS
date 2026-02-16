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
    
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    mov esp, ebp
    pop ebp
    ret