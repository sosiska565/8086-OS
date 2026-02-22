ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    SHELL := cmd.exe
    RM := del /Q /F
    RMDIR := rmdir /S /Q
    MKDIR := mkdir
    PATHSEP := \\
    DEVNULL := nul
    EXE_EXT := .exe
else
    DETECTED_OS := $(shell uname -s)
    RM := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir -p
    PATHSEP := /
    DEVNULL := /dev/null
    EXE_EXT :=
endif

GREEN := \033[0;32m
BLUE := \033[0;34m
RESET := \033[0m

ifeq ($(DETECTED_OS),Darwin)
    CC      = i686-elf-gcc
    LD      = i686-elf-ld
    NASM    = nasm
else
    CC      = gcc
    LD      = ld
    NASM    = nasm
endif

KERNEL_CFLAGS = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I src
USER_CFLAGS   = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I programs$(PATHSEP)lib

ifeq ($(DETECTED_OS),Darwin)
    LDFLAGS = -m elf_i386 -T link.ld -z execstack
else
    LDFLAGS = -m elf_i386 -T link.ld -z execstack
endif

NASMFLAGS = -f elf32

ISO     = os.iso
DISK_IMG = disk.img

DISK_DIR = disk
BIN_DIR  = programs$(PATHSEP)bin
LIB_DIR  = programs$(PATHSEP)lib
USER_DIR = programs$(PATHSEP)user

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
    src/fs/fat/fat32.c \
	src/programs/system/syscalls/syscalls.c \
	src/drivers/mouse/mouse.c \
	src/utils/utils.c \
	src/drivers/pci/pci.c \
	src/drivers/video/bga/gfx_console.c \
	src/drivers/video/graphics.c \
	src/drivers/video/vesa.c \
	src/drivers/video/bga/font.c \
	src/multitask/task.c \
	src/memory/paging.c \
	src/graphics/taskbar.c \
	src/programs/system/initd/initd.c \
	src/graphics/interface.c

ASM_FILES = boot/kernel.asm boot/gdt.asm src/interrupt/interrupts.asm src/multitask/switch.asm src/memory/paging_a.asm

C_OBJECTS   = $(C_FILES:.c=.o)
ASM_OBJECTS = $(ASM_FILES:.asm=.o)
OBJFILES    = $(ASM_OBJECTS) $(C_OBJECTS)

LIB_ENTRY_SRC = $(LIB_DIR)$(PATHSEP)entry.asm
LIB_ENTRY_OBJ = $(BIN_DIR)$(PATHSEP)entry.o

ifeq ($(DETECTED_OS),Windows)
    LIB_SOURCES = $(wildcard $(LIB_DIR)$(PATHSEP)*.c)
    LIB_C_OBJS  = $(patsubst $(LIB_DIR)$(PATHSEP)%.c, $(BIN_DIR)$(PATHSEP)%.o, $(LIB_SOURCES))
else
    LIB_SOURCES = $(wildcard $(LIB_DIR)/*.c)
    LIB_C_OBJS  = $(patsubst $(LIB_DIR)/%.c, $(BIN_DIR)/%.o, $(LIB_SOURCES))
endif

LIB_FINAL_OBJS = $(LIB_ENTRY_OBJ) $(LIB_C_OBJS)

USER_C_FILES = \
	programs/user/nani.c \
	programs/user/rasm.c \
	programs/user/imgvwr.c \
	programs/user/test.c \
	programs/user/test2.c \
	programs/user/wr.c \
	programs/user/eblo.c

ifeq ($(DETECTED_OS),Windows)
    USER_OBJS = $(patsubst $(USER_DIR)$(PATHSEP)%.c, $(BIN_DIR)$(PATHSEP)%.o, $(USER_C_FILES))
    USER_BINS = $(patsubst $(USER_DIR)$(PATHSEP)%.c, $(DISK_DIR)$(PATHSEP)%.bin, $(USER_C_FILES))
else
    USER_OBJS = $(patsubst $(USER_DIR)/%.c, $(BIN_DIR)/%.o, $(USER_C_FILES))
    USER_BINS = $(patsubst $(USER_DIR)/%.c, $(DISK_DIR)/%.bin, $(USER_C_FILES))
endif

.PHONY: all pre-build kernel user_programs build-all run debug clean oncerun \
        install-deps check-deps help

all: pre-build kernel user_programs

pre-build:
	@echo "Создание директорий..."
ifeq ($(DETECTED_OS),Windows)
	@if not exist "$(DISK_DIR)" $(MKDIR) "$(DISK_DIR)"
	@if not exist "$(BIN_DIR)" $(MKDIR) "$(BIN_DIR)"
else
	@$(MKDIR) $(DISK_DIR)
	@$(MKDIR) $(BIN_DIR)
endif

kernel: $(OBJFILES)
	@echo "[LD] Линковка ядра..."
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "✓ Ядро собрано"

user_programs: $(LIB_FINAL_OBJS) $(USER_BINS)
	@echo "✓ Все программы пользователя собраны в $(DISK_DIR)/"

$(BIN_DIR)$(PATHSEP)entry.o: $(LIB_ENTRY_SRC)
ifeq ($(DETECTED_OS),Windows)
	@if not exist "$(BIN_DIR)" $(MKDIR) "$(BIN_DIR)"
else
	@$(MKDIR) $(BIN_DIR)
endif
	$(NASM) $(NASMFLAGS) $< -o $@

ifeq ($(DETECTED_OS),Windows)
$(BIN_DIR)$(PATHSEP)%.o: $(LIB_DIR)$(PATHSEP)%.c
	@if not exist "$(BIN_DIR)" $(MKDIR) "$(BIN_DIR)"
	@echo [CC Lib] $<
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BIN_DIR)$(PATHSEP)%.o: $(USER_DIR)$(PATHSEP)%.c
	@if not exist "$(BIN_DIR)" $(MKDIR) "$(BIN_DIR)"
	@echo [CC App] $<
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(DISK_DIR)$(PATHSEP)%.bin: $(BIN_DIR)$(PATHSEP)%.o $(LIB_FINAL_OBJS)
	@echo [LD App] $@
	$(LD) -m elf_i386 -T programs$(PATHSEP)app.ld -o $@ $< $(LIB_FINAL_OBJS)
else
$(BIN_DIR)/%.o: $(LIB_DIR)/%.c
	@$(MKDIR) $(BIN_DIR)
	@echo "[CC Lib] $<"
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(USER_DIR)/%.c
	@$(MKDIR) $(BIN_DIR)
	@echo "[CC App] $<"
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(DISK_DIR)/%.bin: $(BIN_DIR)/%.o $(LIB_FINAL_OBJS)
	@echo "[LD App] $@"
	$(LD) -m elf_i386 -T programs/app.ld -o $@ $< $(LIB_FINAL_OBJS)
endif

%.o: %.c
	@echo "[CC Kernel] $<"
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "[NASM] $<"
	$(NASM) $(NASMFLAGS) $< -o $@

$(DISK_IMG): kernel user_programs
	@echo "[IMG] Создание диска (64MB)..."
ifeq ($(DETECTED_OS),Windows)
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64 2>$(DEVNULL)
	@echo [FS] Форматирование в FAT32...
	@mkfs.fat -F 32 -n "8086OS_HDD" $(DISK_IMG)
	@echo [DISK] Копирование файлов из папки $(DISK_DIR)...
	@mcopy -i $(DISK_IMG) -s $(DISK_DIR)/* ::/ 2>$(DEVNULL)
else
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64 2>$(DEVNULL)
	@echo "[FS] Форматирование в FAT32..."
	@mkfs.fat -F 32 -n "8086OS_HDD" $(DISK_IMG)
	@echo "[DISK] Копирование файлов из папки $(DISK_DIR)..."
	@mcopy -i $(DISK_IMG) -s $(DISK_DIR)/* ::/ 2>$(DEVNULL)
endif
	@echo "✓ Диск готов!"

iso: kernel
ifeq ($(DETECTED_OS),Windows)
	@if not exist "iso\boot\grub" $(MKDIR) "iso\boot\grub"
	@copy kernel iso\boot\kernel.bin >$(DEVNULL)
	@echo set timeout=0 > iso\boot\grub\grub.cfg
	@echo set default=0 >> iso\boot\grub\grub.cfg
	@echo "insmod vbe" >> iso/boot/grub/grub.cfg
	@echo "insmod vga" >> iso/boot/grub/grub.cfg
	@echo "insmod video_bochs" >> iso/boot/grub/grub.cfg
	@echo "insmod video_cirrus" >> iso/boot/grub/grub.cfg
	
	@echo "set gfxmode=auto" >> iso/boot/grub/grub.cfg
	@echo "set gfxpayload=keep" >> iso/boot/grub/grub.cfg
	@echo menuentry 'OS' { multiboot /boot/kernel.bin } >> iso\boot\grub\grub.cfg
	@grub-mkrescue -o $(ISO) iso 2>$(DEVNULL)
else ifeq ($(DETECTED_OS),Darwin)
	@$(MKDIR) iso/boot/grub
	@cp kernel iso/boot/kernel.bin
	@echo "set timeout=0" > iso/boot/grub/grub.cfg
	@echo "set default=0" >> iso/boot/grub/grub.cfg
	@echo "insmod vbe" >> iso/boot/grub/grub.cfg
	@echo "insmod vga" >> iso/boot/grub/grub.cfg
	@echo "insmod video_bochs" >> iso/boot/grub/grub.cfg
	@echo "insmod video_cirrus" >> iso/boot/grub/grub.cfg
	
	@echo "set gfxmode=auto" >> iso/boot/grub/grub.cfg
	@echo "set gfxpayload=keep" >> iso/boot/grub/grub.cfg
	@echo "menuentry 'OS' { multiboot /boot/kernel.bin }" >> iso/boot/grub/grub.cfg
	@i686-elf-grub-mkrescue -o $(ISO) iso 2>$(DEVNULL)
else
	@$(MKDIR) iso/boot/grub
	@cp kernel iso/boot/kernel.bin
	@echo "set timeout=0" > iso/boot/grub/grub.cfg
	@echo "set default=0" >> iso/boot/grub/grub.cfg
	@echo "insmod vbe" >> iso/boot/grub/grub.cfg
	@echo "insmod vga" >> iso/boot/grub/grub.cfg
	@echo "insmod video_bochs" >> iso/boot/grub/grub.cfg
	@echo "insmod video_cirrus" >> iso/boot/grub/grub.cfg
	
	@echo "set gfxmode=auto" >> iso/boot/grub/grub.cfg
	@echo "set gfxpayload=keep" >> iso/boot/grub/grub.cfg
	@echo "menuentry 'OS' { multiboot /boot/kernel.bin }" >> iso/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) iso 2>$(DEVNULL)
endif

build-all: iso $(DISK_IMG)

run: clean build-all
	@echo "[QEMU] Запуск..."
	qemu-system-i386 \
		-drive file=$(DISK_IMG),format=raw,index=0,if=ide,media=disk \
		-drive file=$(ISO),format=raw,index=1,if=ide,media=cdrom \
		-boot d \
		-rtc base=localtime \
		-m 2g

debug: clean build-all
	@echo "[QEMU] Отладка..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw -boot d -rtc base=localtime -d int,cpu_reset -no-reboot

oncerun:
	qemu-system-i386 \
		-drive file=$(DISK_IMG),format=raw,index=0,if=ide,media=disk \
		-drive file=$(ISO),format=raw,index=1,if=ide,media=cdrom \
		-boot d \
		-rtc base=localtime 

clean:
	@echo "[CLEAN] Очистка..."
ifeq ($(DETECTED_OS),Windows)
	@if exist kernel $(RM) kernel
	@if exist $(DISK_IMG) $(RM) $(DISK_IMG)
	@if exist $(ISO) $(RM) $(ISO)
	@if exist iso $(RMDIR) iso
	@if exist $(BIN_DIR) $(RMDIR) $(BIN_DIR)
	@for %%f in ($(OBJFILES)) do @if exist %%f $(RM) %%f
	@for %%f in ($(DISK_DIR)\*.bin) do @if exist %%f $(RM) %%f
else
	@$(RM) $(OBJFILES) kernel $(DISK_IMG) $(ISO)
	@$(RMDIR) iso $(BIN_DIR)
	@$(RM) $(DISK_DIR)/*.bin
endif
	@echo "✓ Готово"

check-deps:
	@echo "Проверка зависимостей для $(DETECTED_OS)..."
	@echo "----------------------------------------"
ifeq ($(DETECTED_OS),Darwin)
	@which i686-elf-gcc > $(DEVNULL) 2>&1 && echo "✓ i686-elf-gcc установлен" || echo "✗ i686-elf-gcc НЕ установлен (ТРЕБУЕТСЯ!)"
	@which i686-elf-ld > $(DEVNULL) 2>&1 && echo "✓ i686-elf-ld установлен" || echo "✗ i686-elf-ld НЕ установлен (ТРЕБУЕТСЯ!)"
else
	@which gcc$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ gcc установлен" || echo "✗ gcc НЕ установлен"
	@which ld$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ ld установлен" || echo "✗ ld НЕ установлен"
endif
	@which nasm$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ nasm установлен" || echo "✗ nasm НЕ установлен"
	@which qemu-system-i386$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ qemu установлен" || echo "✗ qemu НЕ установлен"
ifeq ($(DETECTED_OS),Darwin)
	@which i686-elf-grub-mkrescue > $(DEVNULL) 2>&1 && echo "✓ grub установлен (i686-elf-grub)" || echo "✗ grub НЕ установлен"
else
	@which grub-mkrescue$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ grub установлен" || echo "✗ grub НЕ установлен"
endif
	@which mkfs.fat$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ mkfs.fat установлен" || echo "✗ mkfs.fat НЕ установлен"
	@which mtools$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ mtools установлен" || echo "✗ mtools НЕ установлен"
	@which dd$(EXE_EXT) > $(DEVNULL) 2>&1 && echo "✓ dd установлен" || echo "✗ dd НЕ установлен"

install-deps:
ifeq ($(DETECTED_OS),Linux)
	@echo "Установка зависимостей для Linux..."
	@if command -v apt-get > /dev/null 2>&1; then \
		echo "Используется apt-get (Debian/Ubuntu)..."; \
		sudo apt-get update; \
		sudo apt-get install -y gcc gcc-multilib nasm qemu-system-x86 grub-pc-bin grub-common xorriso mtools dosfstools; \
	elif command -v dnf > /dev/null 2>&1; then \
		echo "Используется dnf (Fedora)..."; \
		sudo dnf install -y gcc gcc-multilib nasm qemu-system-x86 grub2-tools grub2-pc xorriso mtools dosfstools; \
	elif command -v pacman > /dev/null 2>&1; then \
		echo "Используется pacman (Arch Linux)..."; \
		sudo pacman -S --needed --noconfirm gcc gcc-multilib nasm qemu-system-x86 grub xorriso mtools dosfstools; \
	elif command -v zypper > /dev/null 2>&1; then \
		echo "Используется zypper (openSUSE)..."; \
		sudo zypper install -y gcc gcc-32bit nasm qemu-x86 grub2 xorriso mtools dosfstools; \
	else \
		echo "Неизвестный менеджер пакетов. Установите вручную:"; \
		echo "- gcc (с поддержкой 32-bit)"; \
		echo "- nasm"; \
		echo "- qemu-system-x86"; \
		echo "- grub"; \
		echo "- xorriso"; \
		echo "- mtools"; \
		echo "- dosfstools"; \
	fi
else ifeq ($(DETECTED_OS),Darwin)
	@echo "Установка зависимостей для macOS..."
	@if ! command -v brew > /dev/null 2>&1; then \
		echo "Homebrew не установлен. Устанавливаю..."; \
		/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; \
	fi
	@echo "Устанавливаю зависимости через Homebrew..."
	@brew install nasm qemu grub xorriso mtools dosfstools
	@echo ""
	@echo "========================================="
	@echo "ВАЖНО: Устанавливаю i686-elf toolchain..."
	@echo "========================================="
	@if ! command -v i686-elf-gcc > /dev/null 2>&1; then \
		echo "i686-elf-gcc не найден. Устанавливаю через Homebrew..."; \
		brew tap nativeos/i686-elf-toolchain; \
		brew install i686-elf-binutils i686-elf-gcc; \
	else \
		echo "✓ i686-elf-gcc уже установлен"; \
	fi
	@echo ""
	@echo "Проверка установки..."
	@make check-deps
else ifeq ($(DETECTED_OS),Windows)
	@echo "Установка зависимостей для Windows..."
	@echo "Для Windows рекомендуется использовать MSYS2 или WSL."
	@echo ""
	@echo "Вариант 1: MSYS2 (рекомендуется)"
	@echo "1. Скачайте MSYS2: https://www.msys2.org/"
	@echo "2. Установите и запустите MSYS2 MINGW32"
	@echo "3. Выполните команды:"
	@echo "   pacman -Syu"
	@echo "   pacman -S mingw-w64-i686-gcc mingw-w64-i686-nasm mingw-w64-i686-qemu mingw-w64-i686-grub mtools dosfstools"
	@echo ""
	@echo "Вариант 2: WSL (Windows Subsystem for Linux)"
	@echo "1. Установите WSL: wsl --install"
	@echo "2. Перезагрузите компьютер"
	@echo "3. В WSL выполните: make install-deps"
	@echo ""
	@echo "Вариант 3: Установка вручную"
	@echo "- MinGW-w64: https://www.mingw-w64.org/"
	@echo "- NASM: https://www.nasm.us/"
	@echo "- QEMU: https://www.qemu.org/download/#windows"
	@echo "- GRUB2: через MSYS2"
	@echo "- mtools, dosfstools: через MSYS2"
endif
	@echo ""
	@echo "После установки проверьте зависимости: make check-deps"

help:
	@echo "Доступные команды Makefile:"
	@echo "  make all          - Собрать ядро и пользовательские программы"
	@echo "  make kernel       - Собрать только ядро"
	@echo "  make user_programs- Собрать пользовательские программы"
	@echo "  make iso          - Создать ISO образ"
	@echo "  make build-all    - Собрать всё (ISO + disk.img)"
	@echo "  make run          - Очистить, собрать и запустить в QEMU"
	@echo "  make debug        - Запустить в режиме отладки"
	@echo "  make oncerun      - Запустить без пересборки"
	@echo "  make clean        - Удалить все собранные файлы"
	@echo "  make check-deps   - Проверить установленные зависимости"
	@echo "  make install-deps - Установить недостающие зависимости"
	@echo "  make help         - Показать эту справку"
	@echo ""
	@echo "Текущая ОС: $(DETECTED_OS)"