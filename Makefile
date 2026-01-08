CC      = gcc
KERNEL_CFLAGS = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I src
USER_CFLAGS   = -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -I programs/lib

LD      = ld
LDFLAGS = -m elf_i386 -T link.ld -z execstack
NASM    = nasm
NASMFLAGS = -f elf32

ISO     = os.iso
DISK_IMG = disk.img

# Папки
DISK_DIR = disk
BIN_DIR  = programs/bin
LIB_DIR  = programs/lib
USER_DIR = programs/user

# ==========================================
# 1. ЯДРО
# ==========================================
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
	src/utils/utils.c

ASM_FILES = boot/kernel.asm boot/gdt.asm src/interrupt/interrupts.asm

C_OBJECTS   = $(C_FILES:.c=.o)
ASM_OBJECTS = $(ASM_FILES:.asm=.o)
OBJFILES    = $(ASM_OBJECTS) $(C_OBJECTS)

# ==========================================
# 2. БИБЛИОТЕКА ПОЛЬЗОВАТЕЛЯ (АВТОМАТИКА)
# ==========================================

# Точка входа (ASM) всегда одна
LIB_ENTRY_SRC = $(LIB_DIR)/entry.asm
LIB_ENTRY_OBJ = $(BIN_DIR)/entry.o

# Находим ВСЕ .c файлы в папке programs/lib (oslib.c, string.c и т.д.)
LIB_SOURCES = $(wildcard $(LIB_DIR)/*.c)

# Создаем список объектных файлов для библиотеки (меняем .c на .o и папку на bin)
LIB_C_OBJS  = $(patsubst $(LIB_DIR)/%.c, $(BIN_DIR)/%.o, $(LIB_SOURCES))

# Итоговый список объектов, которые нужно прилинковать к программе пользователя
# Сначала entry.o, потом всё остальное
LIB_FINAL_OBJS = $(LIB_ENTRY_OBJ) $(LIB_C_OBJS)

# ==========================================
# 3. ПРОГРАММЫ ПОЛЬЗОВАТЕЛЯ
# ==========================================

# Список программ (добавляй сюда новые)
USER_C_FILES = \
	programs/user/game.c \
	programs/user/nani.c

# Генерируем пути для .o и .bin файлов
USER_OBJS = $(patsubst $(USER_DIR)/%.c, $(BIN_DIR)/%.o, $(USER_C_FILES))
USER_BINS = $(patsubst $(USER_DIR)/%.c, $(DISK_DIR)/%.bin, $(USER_C_FILES))

# ==========================================
# 4. ПРАВИЛА СБОРКИ
# ==========================================

all: pre-build kernel user_programs

# Создаем папки, если их нет
pre-build:
	@mkdir -p $(DISK_DIR)
	@mkdir -p $(BIN_DIR)

kernel: $(OBJFILES)
	@echo "[LD] Линковка ядра..."
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "✓ Ядро собрано"

user_programs: $(LIB_FINAL_OBJS) $(USER_BINS)
	@echo "✓ Все программы пользователя собраны в $(DISK_DIR)/"

# --- Сборка библиотеки ---

# Компиляция entry.asm
$(BIN_DIR)/entry.o: $(LIB_ENTRY_SRC)
	@mkdir -p $(BIN_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

# УНИВЕРСАЛЬНОЕ ПРАВИЛО для всех .c файлов библиотеки
$(BIN_DIR)/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	@echo "[CC Lib] $<"
	$(CC) $(USER_CFLAGS) -c $< -o $@

# --- Сборка программ пользователя ---

# Компиляция программы пользователя (.c -> .o)
$(BIN_DIR)/%.o: $(USER_DIR)/%.c
	@mkdir -p $(BIN_DIR)
	@echo "[CC App] $<"
	$(CC) $(USER_CFLAGS) -c $< -o $@

# Линковка программы (.o + библиотека -> .bin)
$(DISK_DIR)/%.bin: $(BIN_DIR)/%.o $(LIB_FINAL_OBJS)
	@echo "[LD App] $@"
	$(LD) -m elf_i386 -T programs/app.ld -o $@ $< $(LIB_FINAL_OBJS)

# --- Сборка ядра ---

%.o: %.c
	@echo "[CC Kernel] $<"
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

%.o: %.asm
	@echo "[NASM] $<"
	$(NASM) $(NASMFLAGS) $< -o $@

# ==========================================
# 5. ДИСК И ЗАПУСК
# ==========================================

$(DISK_IMG): kernel user_programs
	@echo "[IMG] Создание диска (64MB)..."
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=64 2>/dev/null
	
	@echo "[FS] Форматирование в FAT32..."
	@mkfs.fat -F 32 -n "8086OS_HDD" $(DISK_IMG)
	
	@echo "[DISK] Копирование файлов из папки $(DISK_DIR)..."
	@mcopy -i $(DISK_IMG) -s $(DISK_DIR)/* ::/
	
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
	qemu-system-i386 \
		-drive file=$(DISK_IMG),format=raw,index=0,if=ide,media=disk \
		-drive file=$(ISO),format=raw,index=1,if=ide,media=cdrom \
		-boot d \
		-rtc base=localtime

debug: clean build-all
	@echo "[QEMU] Отладка..."
	qemu-system-i386 -cdrom $(ISO) -drive file=$(DISK_IMG),format=raw -boot d -rtc base=localtime -d int,cpu_reset -no-reboot

clean:
	@echo "[CLEAN] Очистка..."
	@rm -f $(OBJFILES) kernel $(DISK_IMG) $(ISO)
	@rm -rf iso programs/bin
	@rm -f $(DISK_DIR)/*.bin
	@echo "✓ Готово"

oncerun:
	qemu-system-i386 \
		-drive file=$(DISK_IMG),format=raw,index=0,if=ide,media=disk \
		-drive file=$(ISO),format=raw,index=1,if=ide,media=cdrom \
		-boot d \
		-rtc base=localtime

.PHONY: all iso run clean user_programs build-all debug pre-build