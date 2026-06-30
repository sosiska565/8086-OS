/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/lib/os_mbedtls_bind.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>
#include "mbedtls/net_sockets.h"
#include "mbedtls/entropy.h"



int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
    (void)data;
    
    
    uint32_t ticks = get_ticks();
    
    for (size_t i = 0; i < len; i++) {
        
        ticks = (ticks * 1103515245) + 12345;
        output[i] = (unsigned char)((ticks >> 16) & 0xFF);
    }
    
    *olen = len;
    return 0;
}



void mbedtls_net_init(mbedtls_net_context *ctx) {
    ctx->fd = -1;
}

int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host, const char *port, int proto) {
    
    uint32_t ip = inet_addr(host);
    if (ip == 0) ip = gethostbyname(host);
    if (ip == 0 || ip == 0xFFFFFFFF) return -0x0044; 

    int p = atoi(port);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -0x0042; 

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(p);
    dest.sin_addr.s_addr = ip;

    
    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        close(sock);
        return -0x004C; 
    }
    
    ctx->fd = sock;
    return 0;
}

int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len) {
    int fd = ((mbedtls_net_context *) ctx)->fd;
    if (fd < 0) return -0x004E; 
    
    int ret = send(fd, buf, len, 0);
    if (ret < 0) return -0x004E; 
    return ret;
}

int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len) {
    int fd = ((mbedtls_net_context *) ctx)->fd;
    if (fd < 0) return -0x004E;
    
    int ret = recv(fd, buf, len, 0);
    if (ret < 0) return -0x004C; 
    return ret;
}

void mbedtls_net_free(mbedtls_net_context *ctx) {
    if (ctx->fd >= 0) {
        close(ctx->fd);
    }
    ctx->fd = -1;
}
