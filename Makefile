CC      = gcc
CFLAGS  = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I src
LD      = ld
LDFLAGS = -m elf_i386 -T link.ld -z execstack
NASM    = nasm
NASMFLAGS = -f elf32
ISO     = os.iso

C_FILES = \
	src/kernel.c \
	src/interrupt/idt/idt.c \
    src/drivers/keyboard/keyboardDriver.c \
    src/drivers/vga/vga.c \
    src/drivers/timer/timer.c

ASM_FILES = \
	boot/kernel.asm \
	boot/gdt.asm \
	src/interrupt/interrupts.asm

C_OBJECTS   = $(C_FILES:.c=.o)
ASM_OBJECTS = $(ASM_FILES:.asm=.o)
OBJFILES    = $(ASM_OBJECTS) $(C_OBJECTS)

all: kernel

kernel: $(OBJFILES)
	@echo "[LD] Линковка ядра..."
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "✓ Ядро собрано: kernel"

%.o: %.c
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "[NASM] $<"
	$(NASM) $(NASMFLAGS) $< -o $@

iso: kernel
	@echo "[ISO] Создание образа..."
	@mkdir -p iso/boot/grub
	@cp kernel iso/boot/kernel.bin
	
	@echo "set timeout_style=hidden" > iso/boot/grub/grub.cfg
	@echo "set timeout=0" >> iso/boot/grub/grub.cfg
	@echo "set default=0" >> iso/boot/grub/grub.cfg
	@echo "" >> iso/boot/grub/grub.cfg
	@echo "menuentry '8086-OS' {" >> iso/boot/grub/grub.cfg
	@echo "    multiboot /boot/kernel.bin" >> iso/boot/grub/grub.cfg
	@echo "}" >> iso/boot/grub/grub.cfg

	@grub-mkrescue -o $(ISO) iso 2>/dev/null || grub-mkrescue -o $(ISO) iso
	@echo "✓ ISO создан: $(ISO)"

run: iso
	@echo "[QEMU] Запуск..."
	qemu-system-i386 -cdrom $(ISO)

debug: iso
	@echo "[QEMU] Запуск с отладкой..."
	qemu-system-i386 -cdrom $(ISO) -d int,cpu_reset -no-reboot

list:
	@echo "=== Си файлы ==="
	@echo "$(C_FILES)" | tr ' ' '\n' | grep -v '^$$'
	@echo ""
	@echo "=== Ассемблерные файлы ==="
	@echo "$(ASM_FILES)" | tr ' ' '\n' | grep -v '^$$'
	@echo ""
	@echo "=== Объектные файлы ==="
	@echo "$(OBJFILES)" | tr ' ' '\n' | grep -v '^$$'

clean:
	@echo "[CLEAN] Удаление файлов..."
	@rm -f $(OBJFILES) kernel
	@rm -f $(ISO)
	@rm -rf iso
	@echo "✓ Очистка завершена"

help:
	@echo "Доступные команды:"
	@echo "  make          - Собрать ядро"
	@echo "  make iso      - Создать ISO образ"
	@echo "  make run      - Собрать и запустить в QEMU"
	@echo "  make debug    - Запустить с отладочной информацией"
	@echo "  make list     - Показать список файлов проекта"
	@echo "  make clean    - Удалить все собранные файлы"
	@echo "  make help     - Показать эту справку"

.PHONY: all iso run debug list clean help