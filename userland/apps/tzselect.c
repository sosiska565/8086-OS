/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/tzselect.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

#define TIME_API_HOST "worldtimeapi.org"
#define TIME_API_PORT 80


void save_tz_to_config(const char* new_tz) {
    int sz = get_file_size("/kernel.cfg");
    if (sz < 0) sz = 0;
    
    char* buf = malloc(sz + 128);
    char* out = malloc(sz + 128);
    out[0] = '\0';

    if (sz > 0) {
        read_file("/kernel.cfg", (uint8_t*)buf);
        buf[sz] = '\0';
        
        char* line = buf;
        while (*line) {
            char* next = strchr(line, '\n');
            if (next) *next = '\0';
            
            
            if (strncmp(line, "TZ=", 3) != 0 && strlen(line) > 0) {
                strcat(out, line);
                strcat(out, "\n");
            }
            if (!next) break;
            line = next + 1;
        }
    }
    
    
    strcat(out, "TZ=");
    strcat(out, new_tz);
    strcat(out, "\n");

    write_file("/kernel.cfg", (uint8_t*)out, strlen(out));
    free(buf);
    free(out);
}

int fetch_network_time(const char* tz) {
    printf("Resolving %s...\n", TIME_API_HOST);
    uint32_t target_ip = inet_addr(TIME_API_HOST);
    if (target_ip == 0) target_ip = gethostbyname(TIME_API_HOST);
    
    if (target_ip == 0 || target_ip == 0xFFFFFFFF) {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("Error: Could not resolve time API.\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        return 0;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(TIME_API_PORT);
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("Error: Connection failed.\n");
        close(sock);
        return 0;
    }

    char request[256];
    sprintf(request, "GET /api/timezone/%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", tz, TIME_API_HOST);
    send(sock, request, strlen(request), 0);

    char buf[2048];
    int bytes = 0;
    int total = 0;
    
    
    while ((bytes = recv(sock, buf + total, sizeof(buf) - total - 1, 0)) > 0) {
        total += bytes;
        if (total >= sizeof(buf) - 1) break;
    }
    buf[total] = '\0';
    close(sock);

    
    char* time_ptr = strstr(buf, "\"datetime\":\"");
    if (time_ptr) {
        time_ptr += 12; 
        char date[32];
        int i = 0;
        while (time_ptr[i] && time_ptr[i] != '.' && time_ptr[i] != '+' && time_ptr[i] != '"' && i < 31) {
            date[i] = time_ptr[i] == 'T' ? ' ' : time_ptr[i]; 
            i++;
        }
        date[i] = '\0';
        
        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("Success! Current time in %s is: %s\n", tz, date);
        set_color(COLOR_WHITE, COLOR_BLACK);
        return 1;
    } else {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("Error: Region '%s' not found or API error.\n", tz);
        set_color(COLOR_WHITE, COLOR_BLACK);
        return 0;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
        printf("tzselect - Region & Timezone Utility\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        char* curr_tz = getenv("TZ");
        printf("Current Region: %s\n\n", curr_tz ? curr_tz : "Not set");
        printf("Usage: tzselect <Region/City>\n");
        printf("Examples:\n  tzselect Europe/Moscow\n  tzselect America/New_York\n  tzselect Asia/Tokyo\n");
        return 1;
    }

    char* new_tz = argv[1];
    printf("Testing region: %s...\n", new_tz);
    
    if (fetch_network_time(new_tz)) {
        save_tz_to_config(new_tz);
        printf("Region saved to /kernel.cfg successfully.\n");
    }

    return 0;
}
