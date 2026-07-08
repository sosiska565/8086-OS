CONFIG_TOML = config.toml

OS_NAME := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['os']['name'])")
OS_VERSION_MAJOR := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['os']['version_major'])")
OS_VERSION_MINOR := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['os']['version_minor'])")
OS_VERSION_PATCH := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['os']['version_patch'])")
OS_VERSION_EXTRA := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['os']['version_extra'])")
OS_RELEASE_STR = $(OS_VERSION_MAJOR).$(OS_VERSION_MINOR).$(OS_VERSION_PATCH)-$(OS_VERSION_EXTRA)

ISO := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build']['iso_name'])")
DISK_IMG := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build']['disk_name'])")
DISK_DIR := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build']['disk_dir'])")
KERNEL_BIN := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build']['kernel_bin'])")
DISK_SIZE := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build'].get('disk_size_mb', 64))")

CC := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build'].get('compiler', 'gcc'))")
LD := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build'].get('linker', 'ld'))")
NASM := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb'))['build'].get('assembler', 'nasm'))")

RES_W := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb')).get('display',{}).get('width', 1920))")
RES_H := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb')).get('display',{}).get('height', 1080))")
RES_BPP := $(shell python3 -c "import tomllib; print(tomllib.load(open('$(CONFIG_TOML)','rb')).get('display',{}).get('bpp', 32))")

QEMU_FLAGS := $(shell python3 -c "import tomllib; c=tomllib.load(open('$(CONFIG_TOML)', 'rb')).get('qemu',{}); print(' '.join([f'-{k} {i}' if not isinstance(i, bool) else (f'-{k}' if i else '') for k,v in c.items() for i in (v if isinstance(v,list) else [v])]))")

PATH_DIR = $(DISK_DIR)/path
GCC_INC := $(shell $(CC) -print-file-name=include)

KERNEL_CFLAGS = -O1 -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -nostdinc \
                -isystem $(GCC_INC) \
                -mno-sse -mno-sse2 -mno-mmx -mno-80387 -mgeneral-regs-only \
                -I . -I kernel -I kernel/include -I system_apps \
                -I./drivers/net/lwip/src/include -I./drivers/net
USER_CFLAGS = -O1 -m32 -fno-pie -fno-stack-protector -ffreestanding -nostdlib -nostartfiles -nostdinc -I userland/lib -I userland/lib/mbedtls/include -DMBEDTLS_CONFIG_FILE=\"mbedtls_config_8086.h\"
LDFLAGS = -m elf_i386 -T link.ld -z execstack
NASMFLAGS = -f elf32 -DSCREEN_WIDTH=$(RES_W) -DSCREEN_HEIGHT=$(RES_H) -DSCREEN_BPP=$(RES_BPP)
LIBGCC := $(shell $(CC) $(USER_CFLAGS) -print-libgcc-file-name)

KERNEL_C_SOURCES := $(shell find kernel drivers fs system_apps -path 'drivers/net/lwip' -prune -o -name '*.c' -print)
LWIP_DIR = drivers/net/lwip/src
LWIP_SOURCES = $(wildcard $(LWIP_DIR)/core/*.c) $(wildcard $(LWIP_DIR)/core/ipv4/*.c) $(wildcard $(LWIP_DIR)/netif/ethernet.c)
KERNEL_C_SOURCES += $(LWIP_SOURCES)
KERNEL_ASM_SOURCES := boot/kernel.asm boot/gdt.asm $(shell find kernel -name '*.asm')

KERNEL_C_OBJS := $(KERNEL_C_SOURCES:.c=.o)
KERNEL_ASM_OBJS := $(KERNEL_ASM_SOURCES:.asm=.o)
KERNEL_OBJS := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

LIB_C_SOURCES := $(wildcard userland/lib/*.c) $(wildcard userland/lib/mbedtls/library/*.c)
LIB_ASM_SOURCES := userland/lib/entry.asm
LIB_C_OBJS := $(LIB_C_SOURCES:.c=.o)
LIB_ASM_OBJS := $(LIB_ASM_SOURCES:.asm=.o)
LIB_OBJS := $(LIB_ASM_OBJS) $(LIB_C_OBJS)

# === Разделение APPS и SYSAPPS ===
USER_SOURCES := $(wildcard userland/apps/*.c)
USER_BINS := $(patsubst userland/apps/%.c, $(PATH_DIR)/%.elf, $(USER_SOURCES))

SYSAPP_SOURCES := $(wildcard userland/sysapps/*.c)
SYSAPP_BINS := $(patsubst userland/sysapps/%.c, $(DISK_DIR)/system/bin/%.elf, $(SYSAPP_SOURCES))

.PHONY: all clean run run-nobuild build-all iso generate_version

all: generate_version $(KERNEL_BIN) $(USER_BINS) $(SYSAPP_BINS)

$(KERNEL_BIN): $(KERNEL_OBJS)
	@$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	@$(CC) $(KERNEL_CFLAGS) -c $< -o $@

%.o: %.asm
	@$(NASM) $(NASMFLAGS) $< -o $@

userland/lib/%.o: userland/lib/%.c
	@$(CC) $(USER_CFLAGS) -c $< -o $@

userland/lib/%.o: userland/lib/%.asm
	@$(NASM) $(NASMFLAGS) $< -o $@

$(PATH_DIR)/%.elf: userland/apps/%.c $(LIB_OBJS)
	@mkdir -p $(PATH_DIR)
	@$(CC) $(USER_CFLAGS) -c $< -o userland/apps/$*.o
	@$(LD) -m elf_i386 -T userland/app.ld -o $@ userland/apps/$*.o $(LIB_OBJS) $(LIBGCC)

$(DISK_DIR)/system/bin/%.elf: userland/sysapps/%.c $(LIB_OBJS)
	@mkdir -p $(DISK_DIR)/system/bin
	@$(CC) $(USER_CFLAGS) -c $< -o userland/sysapps/$*.o
	@$(LD) -m elf_i386 -T userland/app.ld -o $@ userland/sysapps/$*.o $(LIB_OBJS) $(LIBGCC)

generate_version:
	@mkdir -p kernel/include
	@printf '#ifndef _GENERATED_VERSION_H\n#define _GENERATED_VERSION_H\n\n#define OS_NAME          "%s"\n#define OS_VERSION_MAJOR %s\n#define OS_VERSION_MINOR %s\n#define OS_VERSION_PATCH %s\n#define OS_VERSION_EXTRA "%s"\n#define OS_RELEASE       "%s"\n\n#endif\n' "$(OS_NAME)" "$(OS_VERSION_MAJOR)" "$(OS_VERSION_MINOR)" "$(OS_VERSION_PATCH)" "$(OS_VERSION_EXTRA)" "$(OS_RELEASE_STR)" > kernel/include/version.h
	@sed -i -E "s|(<!--VERSION-->).*<!--/VERSION-->|\1$(OS_RELEASE_STR)<!--\/VERSION-->|g" README.md

$(DISK_IMG): $(USER_BINS) $(SYSAPP_BINS)
	@mkdir -p $(DISK_DIR)/lib $(DISK_DIR)/include
	@cp userland/lib/entry.o $(DISK_DIR)/lib/
	@cp userland/lib/oslib.o $(DISK_DIR)/lib/
	@cp userland/lib/*.h $(DISK_DIR)/include/
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=$(DISK_SIZE) status=none
	@mkfs.fat -F 32 -n "8086OS" $(DISK_IMG) > /dev/null
	@mcopy -v -i $(DISK_IMG) -s $(DISK_DIR)/* ::/

iso: $(KERNEL_BIN) $(DISK_IMG)
	@mkdir -p iso/boot/grub
	@cp $(KERNEL_BIN) iso/boot/os_kernel.bin
	@cp $(DISK_IMG) iso/boot/disk.img
	@printf "set timeout=3\nset default=0\ninsmod all_video\nmenuentry '$(OS_NAME)' {\n    multiboot /boot/$(KERNEL_BIN)\n    module /boot/$(DISK_IMG) disk.img\n    boot\n}\n" > iso/boot/grub/grub.cfg
	@grub-mkrescue -o $(ISO) iso

build-all: generate_version iso $(DISK_IMG)

run: build-all
	qemu-system-i386 $(QEMU_FLAGS)

run-nobuild:
	qemu-system-i386 $(QEMU_FLAGS)

clean:
	@find . -name "*.o" -type f -delete
	@rm -f $(KERNEL_BIN) $(DISK_IMG) $(ISO) kernel/include/version.h
	@rm -rf iso
	@rm -rf userland/include