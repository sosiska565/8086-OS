OS_NAME          = 8086-OS
OS_VERSION_MAJOR = 0
OS_VERSION_MINOR = 9
OS_VERSION_PATCH = 2
OS_VERSION_EXTRA = beta

OS_RELEASE_STR   = $(OS_VERSION_MAJOR).$(OS_VERSION_MINOR).$(OS_VERSION_PATCH)-$(OS_VERSION_EXTRA)

# =============================

DETECTED_OS := $(shell uname -s)
CC      = gcc
LD      = ld
NASM    = nasm

GCC_INC := $(shell $(CC) -print-file-name=include)

KERNEL_CFLAGS = -O1 -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -nostdinc \
                -isystem $(GCC_INC) \
                -mno-sse -mno-sse2 -mno-mmx -mno-80387 -mgeneral-regs-only \
                -I . -I kernel -I kernel/include -I system_apps \
                -I./drivers/net/lwip/src/include -I./drivers/net

USER_CFLAGS   = -O1 -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -nostdinc \
                -I userland/lib -I userland/lib/mbedtls/include \
                -DMBEDTLS_CONFIG_FILE=\"mbedtls_config_8086.h\"

LDFLAGS   = -m elf_i386 -T link.ld -z execstack
NASMFLAGS = -f elf32
LIBGCC    := $(shell $(CC) $(USER_CFLAGS) -print-libgcc-file-name)

ISO      = os.iso
DISK_IMG = disk.img
DISK_DIR = disk

PATH_DIR = $(DISK_DIR)/path
KERNEL_BIN = os_kernel.bin

KERNEL_C_SOURCES := $(shell find kernel drivers fs system_apps -path 'drivers/net/lwip' -prune -o -name '*.c' -print)

LWIP_DIR = drivers/net/lwip/src
LWIP_SOURCES = $(wildcard $(LWIP_DIR)/core/*.c) \
               $(wildcard $(LWIP_DIR)/core/ipv4/*.c) \
               $(wildcard $(LWIP_DIR)/netif/ethernet.c)

KERNEL_C_SOURCES += $(LWIP_SOURCES)

KERNEL_ASM_SOURCES := boot/kernel.asm boot/gdt.asm $(shell find kernel -name '*.asm')

KERNEL_C_OBJS   := $(KERNEL_C_SOURCES:.c=.o)
KERNEL_ASM_OBJS := $(KERNEL_ASM_SOURCES:.asm=.o)
KERNEL_OBJS     := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

LIB_C_SOURCES   := $(wildcard userland/lib/*.c) $(wildcard userland/lib/mbedtls/library/*.c)
LIB_ASM_SOURCES := userland/lib/entry.asm
LIB_C_OBJS      := $(LIB_C_SOURCES:.c=.o)
LIB_ASM_OBJS    := $(LIB_ASM_SOURCES:.asm=.o)
LIB_OBJS        := $(LIB_ASM_OBJS) $(LIB_C_OBJS)

USER_SOURCES    := $(wildcard userland/apps/*.c)
USER_BINS       := $(patsubst userland/apps/%.c, $(PATH_DIR)/%.elf, $(USER_SOURCES))

TCC_DIR    := tinycc
TCC_OUT    := $(PATH_DIR)/tcc.elf
TCC_CFLAGS := $(USER_CFLAGS) -DONE_SOURCE=1 -DTCC_TARGET_I386 -DCONFIG_TCCDIR=\"/path\" -DCONFIG_TCC_CRIPLED

.PHONY: all clean run run-nobuild build-all iso prep_tcc generate_version help

all: generate_version prep_tcc $(KERNEL_BIN) $(USER_BINS) $(TCC_OUT)
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

$(PATH_DIR)/%.elf: userland/apps/%.c $(LIB_OBJS)
	@mkdir -p $(PATH_DIR)
	@echo "CC App $<"
	@$(CC) $(USER_CFLAGS) -c $< -o userland/apps/$*.o
	@$(LD) -m elf_i386 -T userland/app.ld -o $@ userland/apps/$*.o $(LIB_OBJS) $(LIBGCC)

generate_version:
	@echo "⚙️  Генерация kernel/include/version.h..."
	@mkdir -p kernel/include
	@printf '#ifndef _GENERATED_VERSION_H\n#define _GENERATED_VERSION_H\n\n' > kernel/include/version.h
	@printf '#define OS_NAME          "%s"\n' $(OS_NAME) >> kernel/include/version.h
	@printf '#define OS_VERSION_MAJOR %s\n' $(OS_VERSION_MAJOR) >> kernel/include/version.h
	@printf '#define OS_VERSION_MINOR %s\n' $(OS_VERSION_MINOR) >> kernel/include/version.h
	@printf '#define OS_VERSION_PATCH %s\n' $(OS_VERSION_PATCH) >> kernel/include/version.h
	@printf '#define OS_VERSION_EXTRA "%s"\n' $(OS_VERSION_EXTRA) >> kernel/include/version.h
	@printf '#define OS_RELEASE       "%s"\n' $(OS_RELEASE_STR) >> kernel/include/version.h
	@printf '\n#endif\n' >> kernel/include/version.h
	@echo "📝 Синхронизация версии в README.md..."
	@sed -i -E "s|<!--VERSION-->.*<!--/VERSION-->|<!--VERSION-->$(OS_RELEASE_STR)<!--/VERSION-->|" README.md

prep_tcc:
	@if [ ! -d "$(TCC_DIR)" ]; then echo "❌ Скачайте TCC: git clone https://repo.or.cz/tinycc.git"; exit 1; fi
	@echo "⚙️  Генерация изолированных заголовков в userland/lib..."
	@mkdir -p userland/lib/sys

$(TCC_OUT): $(LIB_OBJS)
	@mkdir -p $(PATH_DIR)
	@echo "CC Compiling TCC for 8086-OS..."
	@$(CC) $(TCC_CFLAGS) -c $(TCC_DIR)/tcc.c -o userland/apps/tcc.o
	@echo "LD Linking TCC..."
	@$(LD) -m elf_i386 -T userland/app.ld -o $@ userland/apps/tcc.o $(LIB_OBJS) $(LIBGCC)

$(DISK_IMG): $(USER_BINS) $(TCC_OUT)
	@echo "Создание окружения разработчика (SDK) на диске..."
	@mkdir -p $(DISK_DIR)/lib $(DISK_DIR)/include
	@cp userland/lib/entry.o $(DISK_DIR)/lib/
	@cp userland/lib/oslib.o $(DISK_DIR)/lib/
	@cp userland/lib/*.h $(DISK_DIR)/include/
	@echo "Создание диска FAT32..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64 status=none
	@mkfs.fat -F 32 -n "8086OS" $(DISK_IMG) > /dev/null
	@echo "Копируем структуру папок и программы в образ..."
	@mcopy -v -i $(DISK_IMG) -s $(DISK_DIR)/* ::/
	@echo "Образ диска успешно собран!"

iso: $(KERNEL_BIN) $(DISK_IMG)
	@echo "Создание загрузочного ISO с Live OS..."
	@mkdir -p iso/boot/grub
	@cp $(KERNEL_BIN) iso/boot/os_kernel.bin
	@cp $(DISK_IMG) iso/boot/disk.img
	@echo "Генерация grub.cfg..."
	@printf "set timeout=3\nset default=0\ninsmod all_video\nmenuentry '8086-OS' {\n    multiboot /boot/os_kernel.bin\n    module /boot/disk.img disk.img\n    boot\n}\n" > iso/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) iso

build-all: iso $(DISK_IMG)

run: generate_version build-all
	@echo "Запуск QEMU..."
	qemu-system-i386 -accel kvm -accel whpx -accel hvf -accel tcg \
		-device ahci,id=ahci -drive file=disk.img,format=raw,if=none,id=disk1 \
		-device ide-hd,drive=disk1,bus=ahci.0 -drive file=os.iso,format=raw,if=none,id=cd1 \
		-device ide-cd,drive=cd1,bus=ahci.1 -boot d -rtc base=localtime -m 2g \
		-display sdl,grab-mod=lctrl-lalt \
		-netdev bridge,id=mynet0,br=virbr0 \
		-device rtl8139,netdev=mynet0 \
		-object filter-dump,id=f1,netdev=mynet0,file=network_dump.pcap

run-nobuild:
	qemu-system-i386 -accel kvm -accel whpx -accel hvf -accel tcg \
		-device ahci,id=ahci -drive file=disk.img,format=raw,if=none,id=disk1 \
		-device ide-hd,drive=disk1,bus=ahci.0 -drive file=os.iso,format=raw,if=none,id=cd1 \
		-device ide-cd,drive=cd1,bus=ahci.1 -boot d -rtc base=localtime -m 2g \
		-display sdl,grab-mod=lctrl-lalt \
		-netdev bridge,id=mynet0,br=virbr0 \
		-device rtl8139,netdev=mynet0 \
		-object filter-dump,id=f1,netdev=mynet0,file=network_dump.pcap

clean:
	@echo "Очистка..."
	@find . -name "*.o" -type f -delete
	@rm -f $(KERNEL_BIN) $(DISK_IMG) $(ISO) kernel/include/version.h
	@rm -rf iso
	@rm -rf userland/include

help:
	@echo "Usage: make [TARGET]"
	@echo ""
	@echo "Targets for building and development of 8086-OS."
	@echo ""
	@echo "Main targets:"
	@echo "  all            Build core OS components, userland apps, and prepare TCC"
	@echo "  build-all      Build both bootable ISO image and FAT32 disk image"
	@echo "  clean          Remove all generated object files, binaries, and disk images"
	@echo ""
	@echo "Emulation and testing:"
	@echo "  run            Build the whole project from scratch and launch in QEMU"
	@echo "  run-nobuild    Launch QEMU immediately using existing ISO and disk images"
	@echo ""
	@echo "Component targets:"
	@echo "  prep_tcc       Verify Tiny C Compiler directory and isolate C headers"
	@echo "  iso            Generate a bootable Live-OS ISO file using GRUB"
	@echo ""
	@echo "Report bugs and suggestions to the project repository."