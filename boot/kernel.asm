bits 32
section .text
	align 4
	dd 0x1BADB002
	dd 0x00000003
	dd -(0x1BADB002 + 0x00000003)

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
	hlt

section .bss
resb 8192
stack_space