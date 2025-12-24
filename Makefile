CC      = gcc
KERNEL_CFLAGS = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I src
USER_CFLAGS   = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I programs/lib

LD      = ld
LDFLAGS = -m elf_i386 -T link.ld -z execstack
NASM    = nasm
NASMFLAGS = -f elf32

ISO     = os.iso
DISK_IMG = disk.img

C_FILES = src/kernel.c \
	src/interrupt/idt/idt.c \
    src/drivers/keyboard/keyboardDriver.c \
	src/drivers/file/initrd.c \
	src/drivers/io/io.c \
	src/drivers/file/ATA/ATA.c \
    src/drivers/rtc/rtc.c \
    src/drivers/speaker/speaker.c \
    src/drivers/vga/vga.c \
    src/drivers/timer/timer.c \
    src/programs/system/console/console.c \
    src/programs/system/console/system.c \
    src/programs/system/setup/setup.c \
    src/programs/system/disk_viewer/disk_viewer.c \
    src/memory/memory.c \
	src/programs/system/memory_viewer/memory_viewer.c \
	src/interrupt/interrupts/interrupts.c \
    src/fs/fat/fat16.c \
	src/programs/system/syscalls/syscalls.c

ASM_FILES = boot/kernel.asm boot/gdt.asm src/interrupt/interrupts.asm

USER_C_FILES = \
	programs/user/hello.c

C_OBJECTS   = $(C_FILES:.c=.o)
ASM_OBJECTS = $(ASM_FILES:.asm=.o)
OBJFILES    = $(ASM_OBJECTS) $(C_OBJECTS)

LIB_SRC   = programs/lib/oslib.c
LIB_ENTRY = programs/lib/entry.asm
LIB_OBJS  = programs/bin/entry.o programs/bin/oslib.o

USER_OBJS = $(patsubst programs/user/%.c, programs/bin/%.o, $(USER_C_FILES))
USER_BINS = $(patsubst programs/user/%.c, programs/bin/%.bin, $(USER_C_FILES))

all: kernel user_programs

kernel: $(OBJFILES)
	@echo "[LD] Линковка ядра..."
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "✓ Ядро собрано"

user_programs: $(LIB_OBJS) $(USER_BINS)
	@echo "✓ Все программы пользователя собраны"

programs/bin/entry.o: $(LIB_ENTRY)
	@mkdir -p programs/bin
	$(NASM) $(NASMFLAGS) $< -o $@

programs/bin/oslib.o: $(LIB_SRC)
	@mkdir -p programs/bin
	$(CC) $(USER_CFLAGS) -c $< -o $@

programs/bin/%.o: programs/user/%.c
	@mkdir -p programs/bin
	@echo "[CC User] $<"
	$(CC) $(USER_CFLAGS) -c $< -o $@

programs/bin/%.bin: programs/bin/%.o $(LIB_OBJS)
	@echo "[LD User] $@"
	$(LD) -m elf_i386 -T programs/app.ld -o $@ $< $(LIB_OBJS)

%.o: %.c
	@echo "[CC Kernel] $<"
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "[NASM] $<"
	$(NASM) $(NASMFLAGS) $< -o $@

$(DISK_IMG): kernel user_programs
	@echo "[IMG] Создание диска..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=10 2>/dev/null
	@mkfs.fat -F 16 -n "8086OS_HDD" $(DISK_IMG) 2>/dev/null
	
	@echo "Test file" > readme.txt
	@mcopy -i $(DISK_IMG) readme.txt ::readme.txt 2>/dev/null
	
	@echo "[DISK] Копирование программ..."
	@# Пробегаем по списку всех скомпилированных .bin файлов и копируем их
	@for bin in $(USER_BINS); do \
		filename=$$(basename $$bin); \
		echo "  -> $$filename"; \
		mcopy -i $(DISK_IMG) $$bin ::$$filename 2>/dev/null; \
	done
	
	@rm -f readme.txt
	@echo "✓ Диск готов!"

iso: kernel
	@mkdir -p iso/boot/grub
	@cp kernel iso/boot/kernel.bin
	@echo "set timeout=0" > iso/boot/grub/grub.cfg
	@echo "set default=0" >> iso/boot/grub/grub.cfg
	@echo "menuentry 'OS' { multiboot /boot/kernel.bin }" >> iso/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) iso 2>/dev/null

build-all: iso $(DISK_IMG)

run: clean build-all
	@echo "[QEMU] Запуск..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw -boot d -rtc base=localtime

debug: clean build-all
	@echo "[QEMU] Отладка..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw -boot d -rtc base=localtime -d int,cpu_reset -no-reboot

clean:
	@echo "[CLEAN] Очистка..."
	@rm -f $(OBJFILES) kernel $(DISK_IMG) $(ISO)
	@rm -rf iso programs/bin
	@echo "✓ Готово"

.PHONY: all iso run clean user_programs build-all debug