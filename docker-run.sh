#!/bin/bash

export DOCKER_BUILDKIT=1
IMAGE_NAME="8086-os-builder"

check_installed() {
    if ! command -v "$1" &> /dev/null; then
        echo "Ошибка: На вашей машине не установлен '$1'."
        echo "$2"
        exit 1
    fi
}

check_installed "docker" "Пожалуйста, установите Docker для сборки проекта."

DETECTED_OS=$(uname -s)
if [[ "$DETECTED_OS" == *"MINGW"* || "$DETECTED_OS" == *"MSYS"* ]]; then
    check_installed "qemu-system-i386.exe" "Установите QEMU для Windows и добавьте его в переменную PATH."
else
    check_installed "qemu-system-i386" "Установите пакет qemu-system-x86 через ваш менеджер пакетов (pacman, apt, brew)."
fi

if [[ "$(docker images -q $IMAGE_NAME 2> /dev/null)" == "" ]]; then
    echo "Docker-образ не найден. Начинаем сборку окружения (это произойдет один раз)..."
    docker build -t $IMAGE_NAME .
fi

echo "[Docker] Запуск компиляции и сборки образов ОС..."

docker run -it --rm -v "$(pwd)":/workspace $IMAGE_NAME make build-all

if [ $? -ne 0 ]; then
    echo "Ошибка компиляции внутри Docker контейнера."
    exit 1
fi

echo "[Host] Сборка завершена! Запуск 8086-OS в локальном QEMU..."

make run-nobuild
