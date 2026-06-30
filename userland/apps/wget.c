/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/wget.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

#define BUFFER_SIZE 4096

int main(int argc, char** argv) {
    if (argc < 3) {
        set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
        printf("wget - Download files from the web\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        printf("Usage: wget <domain> <output_file>\n");
        printf("Example: wget google.com index.html\n");
        return 1;
    }

    char *domain = argv[1];
    char *outfile = argv[2];

    printf("Resolving %s...\n", domain);
    uint32_t target_ip = inet_addr(domain);
    if (target_ip == 0) {
        target_ip = gethostbyname(domain);
        if (target_ip == 0) {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("wget: cannot resolve %s\n", domain);
            set_color(COLOR_WHITE, COLOR_BLACK);
            return 1;
        }
    }

    uint8_t *ip = (uint8_t*)&target_ip;
    printf("Connecting to %d.%d.%d.%d:80...\n", ip[0], ip[1], ip[2], ip[3]);

    int sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock < 0) {
        printf("wget: failed to create socket\n");
        return 1;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(80); 
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("wget: connection failed\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        close(sock);
        return 1;
    }

    char request[256];
    sprintf(request, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", domain);
    
    send(sock, request, strlen(request), 0);
    printf("Request sent, waiting for reply...\n");
    
    int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        printf("wget: failed to open output file\n");
        close(sock);
        return 1;
    }

    char buf[BUFFER_SIZE];
    int bytes_received;
    int total_bytes = 0;
    int header_passed = 0;

    while ((bytes_received = recv(sock, buf, BUFFER_SIZE, 0)) > 0) {
        char *data_ptr = buf;
        int data_len = bytes_received;

        if (!header_passed) {
            for (int i = 0; i < bytes_received - 3; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n') {
                    header_passed = 1;
                    data_ptr = buf + i + 4;
                    data_len = bytes_received - (i + 4);
                    break;
                }
            }
            if (!header_passed) continue;
        }

        if (data_len > 0) {
            write(fd, data_ptr, data_len);
            total_bytes += data_len;
            printf("\rDownloaded: %d bytes", total_bytes);
        }
    }

    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("\nDone! Saved to %s\n", outfile);
    set_color(COLOR_WHITE, COLOR_BLACK);
    
    close(fd);
    close(sock); 
    return 0;
}
