DETECTED_OS := $(shell uname -s)
CC      = gcc
LD      = ld
NASM    = nasm


KERNEL_CFLAGS = -O1 -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles \
                -I . -I kernel -I kernel/include -I system_apps


USER_CFLAGS   = -O1 -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles \
                -I userland/lib

LDFLAGS   = -m elf_i386 -T link.ld -z execstack
NASMFLAGS = -f elf32

ISO      = os.iso
DISK_IMG = disk.img
DISK_DIR = disk
KERNEL_BIN = os_kernel.bin



KERNEL_C_SOURCES   := $(shell find kernel drivers fs system_apps -name '*.c')
KERNEL_ASM_SOURCES := boot/kernel.asm boot/gdt.asm $(shell find kernel -name '*.asm')

KERNEL_C_OBJS   := $(KERNEL_C_SOURCES:.c=.o)
KERNEL_ASM_OBJS := $(KERNEL_ASM_SOURCES:.asm=.o)
KERNEL_OBJS     := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

LIB_C_SOURCES   := $(wildcard userland/lib/*.c)
LIB_ASM_SOURCES := userland/lib/entry.asm
LIB_C_OBJS      := $(LIB_C_SOURCES:.c=.o)
LIB_ASM_OBJS    := $(LIB_ASM_SOURCES:.asm=.o)
LIB_OBJS        := $(LIB_ASM_OBJS) $(LIB_C_OBJS)

USER_SOURCES    := $(wildcard userland/apps/*.c)
USER_BINS       := $(patsubst userland/apps/%.c, $(DISK_DIR)/%.bin, $(USER_SOURCES))

.PHONY: all clean run build-all iso

all: $(KERNEL_BIN) $(USER_BINS)
	@echo "✅ Сборка успешно завершена! ✅"


$(KERNEL_BIN): $(KERNEL_OBJS)
	@echo "Линковка ядра..."
	@$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	@echo "CC Kernel $<"
	@$(CC) $(KERNEL_CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "NASM Kernel $<"
	@$(NASM) $(NASMFLAGS) $< -o $@


userland/lib/%.o: userland/lib/%.c
	@echo "CC Lib $<"
	@$(CC) $(USER_CFLAGS) -c $< -o $@

userland/lib/%.o: userland/lib/%.asm
	@echo "NASM Lib $<"
	@$(NASM) $(NASMFLAGS) $< -o $@

$(DISK_DIR)/%.bin: userland/apps/%.c $(LIB_OBJS)
	@mkdir -p $(DISK_DIR)
	@echo "CC App $<"
	@$(CC) $(USER_CFLAGS) -c $< -o userland/apps/$*.o
	@$(LD) -m elf_i386 -T userland/app.ld -o $@ userland/apps/$*.o $(LIB_OBJS)


$(DISK_IMG): $(USER_BINS)
	@echo "Создание диска FAT32..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64 status=none
	@mkfs.fat -F 32 -n "8086OS" $(DISK_IMG) > /dev/null
	@mcopy -i $(DISK_IMG) -s $(DISK_DIR)/* ::/

iso: $(KERNEL_BIN)
	@mkdir -p iso/boot/grub
	@cp $(KERNEL_BIN) iso/boot/os_kernel.bin
	@echo "Генерация мгновенного grub.cfg..."
	@printf "set timeout=0\n" > iso/boot/grub/grub.cfg
	@printf "set timeout_style=hidden\n" >> iso/boot/grub/grub.cfg 
	@printf "set default=0\n" >> iso/boot/grub/grub.cfg
	@printf "insmod all_video\n" >> iso/boot/grub/grub.cfg
	@printf "set gfxmode=auto\n" >> iso/boot/grub/grub.cfg
	@printf "set gfxpayload=keep\n" >> iso/boot/grub/grub.cfg
	@printf "menuentry '8086-OS' {\n" >> iso/boot/grub/grub.cfg
	@printf "    multiboot /boot/os_kernel.bin\n" >> iso/boot/grub/grub.cfg
	@printf "    boot\n" >> iso/boot/grub/grub.cfg
	@printf "}\n" >> iso/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) iso

build-all: iso $(DISK_IMG)

run: clean build-all
	@echo "Запуск QEMU..."
	qemu-system-i386 -accel kvm -accel whpx -accel hvf -accel tcg \
		-device ahci,id=ahci \
		-drive file=$(DISK_IMG),format=raw,if=none,id=disk1 \
		-device ide-hd,drive=disk1,bus=ahci.0 \
		-drive file=$(ISO),format=raw,if=none,id=cd1 \
		-device ide-cd,drive=cd1,bus=ahci.1 \
		-boot d -rtc base=localtime -m 2g

clean:
	@echo "Очистка..."
	@find . -name "*.o" -type f -delete
	@rm -f $(KERNEL_BIN) $(DISK_IMG) $(ISO)
	@rm -rf iso
	@rm -f $(DISK_DIR)/*.bin