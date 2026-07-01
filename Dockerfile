FROM --platform=linux/amd64 archlinux:latest

RUN pacman -Syu --noconfirm && pacman -S --noconfirm --needed \
    base-devel \
    nasm \
    git \
    grub \
    xorriso \
    dosfstools \
    mtools \
    && pacman -Scc --noconfirm

WORKDIR /workspace

CMD ["make", "build-all"]
