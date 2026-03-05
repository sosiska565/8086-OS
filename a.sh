cat << 'EOF' > migrate.sh
#!/bin/bash
set -e

echo "🚀 Начинаем Великое Переселение архитектуры..."

# 1. Создаем новые директории
mkdir -p kernel/{core,irq,idt,mm,task,syscalls,utils,graphics,include}
mkdir -p userland/{lib,apps}

# 2. Переносим драйверы и ФС в корень
mv src/drivers ./drivers
mv src/fs ./fs

# 3. Формируем Ядро (Ring 0)
mv src/memory/* ./kernel/mm/
mv src/multitask/* ./kernel/task/
mv src/interrupt/idt/* ./kernel/idt/

# Внимательно переносим прерывания
mv src/interrupt/interrupts.asm ./kernel/irq/
mv src/interrupt/interrupts/interrupts.c ./kernel/irq/
mv src/interrupt/interrupts/interrupts.h ./kernel/irq/

mv src/utils/* ./kernel/utils/
mv src/graphics/* ./kernel/graphics/
mv src/programs/system/syscalls/* ./kernel/syscalls/

mv src/kernel.c ./kernel/core/
mv src/global.h ./kernel/include/
mv src/multiboot.h ./kernel/include/

# 4. Выделяем системные утилиты
mv src/programs/system ./system_apps
rm -rf system_apps/syscalls # удаляем пустую папку, мы ее перенесли в ядро

# 5. Выделяем Юзерспейс (Ring 3)
mv programs/lib/* ./userland/lib/
mv programs/user/* ./userland/apps/
mv programs/app.ld ./userland/

# 6. Удаляем старые папки
rm -rf src programs

echo "🔧 Обновляем инклуды во всех файлах..."
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sed -i 's|"memory/|"mm/|g' {} +
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sed -i 's|"multitask/|"task/|g' {} +
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sed -i 's|"interrupt/idt/|"idt/|g' {} +
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sed -i 's|"interrupt/interrupts/|"irq/|g' {} +
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sed -i 's|"programs/system/syscalls/|"syscalls/|g' {} +
find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sed -i 's|"programs/system/|"system_apps/|g' {} +

echo "✅ Рефакторинг завершен! Структура Linux-way готова."
EOF

chmod +x migrate.sh
./migrate.sh