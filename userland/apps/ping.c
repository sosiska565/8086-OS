/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/ping.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "oslib.h"

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;
    for (sum = 0; len > 1; len -= 2) sum += *buf++;
    if (len == 1) sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

struct icmp_echo_hdr {
    uint8_t type;
    uint8_t code;
    uint16_t chksum;
    uint16_t id;
    uint16_t seqno;
};

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: ping <domain or ip>\n");
        return 1;
    }

    uint32_t target_ip = inet_addr(argv[1]);
    
    if (target_ip == 0) {
        printf("Resolving %s...\n", argv[1]);
        target_ip = gethostbyname(argv[1]);
        if (target_ip == 0) {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("ping: cannot resolve %s: Unknown host\n", argv[1]);
            set_color(COLOR_WHITE, COLOR_BLACK);
            return 1;
        }
        uint8_t *ip_bytes = (uint8_t*)&target_ip;
        printf("Resolved to %d.%d.%d.%d\n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
    }

    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) {
        printf("ping: Failed to create RAW socket.\n");
        return 1;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("ping: Connection failed.\n");
        close(sock);
        return 1;
    }

    printf("PING %s 56(84) bytes of data.\n", argv[1]);

    char packet[64];
    char recv_buf[128];

    while (1) {
        if(poll_key() != 0) break;
        memset(packet, 0, sizeof(packet));
        struct icmp_echo_hdr *icmp = (struct icmp_echo_hdr *)packet;
        
        icmp->type = 8; 
        icmp->code = 0;
        icmp->id = htons(0x1234);
        icmp->seqno = htons(1);
        
        for(int j = sizeof(struct icmp_echo_hdr); j < sizeof(struct icmp_echo_hdr) + 32; j++) {
            packet[j] = 'A' + (j % 26);
        }

        int packet_size = sizeof(struct icmp_echo_hdr) + 32;
        icmp->chksum = checksum(packet, packet_size);

        uint32_t start_time = get_ticks();
        send(sock, packet, packet_size, 0);

        int res = recv(sock, recv_buf, sizeof(recv_buf), 0);

        if (res > 0) {
            uint32_t end_time = get_ticks();
            struct icmp_echo_hdr *reply = (struct icmp_echo_hdr *)(recv_buf + 20);
            
            if (reply->type == 0) { 
                set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
                printf("%d bytes from %s: icmp_seq=%d time=%d ms\n", 
                    res - 20, argv[1], 1, end_time - start_time);
                set_color(COLOR_WHITE, COLOR_BLACK);
            }
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Request timeout for icmp_seq %d\n", 1);
            set_color(COLOR_WHITE, COLOR_BLACK);
        }

        uint32_t wait_until = get_ticks() + 1000;
        while(get_ticks() < wait_until) yield();
    }

    close(sock);
    return 0;
}
