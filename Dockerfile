FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-multilib \
    nasm \
    binutils \
    make \
    sed \
    git \
    
    grub-pc-bin \
    grub-common \
    xorriso \
    
    dosfstools \
    mtools \
    
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["make", "build-all"]