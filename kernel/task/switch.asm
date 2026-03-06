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