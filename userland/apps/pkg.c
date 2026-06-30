/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/userland/apps/pkg.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include <oslib.h>

#define REPO_HOST "150.241.64.152" 
#define REPO_PORT 80
#define BUFFER_SIZE 4096

char* download_http(const char* filepath, int fd, int* out_size) {
    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    if (target_ip == 0 || target_ip == 0xFFFFFFFF) {
        printf("Error: Could not resolve repository host.\n");
        return NULL;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock < 0) return NULL;

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(REPO_PORT); 
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        close(sock);
        return NULL;
    }

    char request[256];
    sprintf(request, "GET /%s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n", filepath, REPO_HOST);
    send(sock, request, strlen(request), 0);

    char buf[BUFFER_SIZE];
    int bytes_received;
    int header_passed = 0;
    int total_bytes = 0;
    
    char* ram_buffer = NULL;
    if (fd == -1) ram_buffer = malloc(65536); 

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
            if (fd != -1) {
                write(fd, data_ptr, data_len);
                total_bytes += data_len;
                printf("\rDownloaded: %d bytes", total_bytes);
            } else {
                if (total_bytes + data_len < 65536) {
                    memcpy(ram_buffer + total_bytes, data_ptr, data_len);
                }
                total_bytes += data_len;
            }
        }
    }

    close(sock);
    
    if (out_size) *out_size = total_bytes;
    
    if (fd == -1 && ram_buffer != NULL) {
        ram_buffer[total_bytes] = '\0';
        return ram_buffer;
    }
    return NULL;
}

int upload_http(const char* local_path, const char* pkg_name) {
    int local_fd = open(local_path, O_RDONLY);
    if (local_fd < 0) {
        printf("Error: Cannot open local file %s\n", local_path);
        return -1;
    }
    int file_size = get_file_size(local_path);
    uint8_t* file_buf = malloc(file_size);
    read_file(local_path, file_buf);
    close(local_fd);

    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(REPO_PORT);
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        close(sock); free(file_buf);
        return -1;
    }

    const char* filename = strrchr(local_path, '/');
    if (filename) filename++; else filename = local_path;

    char header[512];
    sprintf(header, "POST /upload HTTP/1.0\r\n"
                    "Host: %s\r\n"
                    "X-File-Name: %s\r\n"
                    "X-Package-Name: %s\r\n"
                    "Content-Length: %d\r\n\r\n", 
                    REPO_HOST, filename, pkg_name, file_size);

    
    int h_sent = 0, h_len = strlen(header);
    while (h_sent < h_len) {
        int r = send(sock, header + h_sent, h_len - h_sent, 0);
        if (r > 0) h_sent += r; else yield();
    }

    
    int sent = 0;
    while(sent < file_size) {
        int chunk = file_size - sent;
        if (chunk > 2048) chunk = 2048; 
        
        int res = send(sock, file_buf + sent, chunk, 0);
        if (res > 0) {
            sent += res;
            printf("\rUploading: %d / %d bytes", sent, file_size);
        } else {
            
            yield(); 
        }
    }
    printf("\n");

    
    char resp_buf[128];
    while (recv(sock, resp_buf, sizeof(resp_buf), 0) > 0) {
        yield();
    }

    free(file_buf);
    close(sock);
    return 1;
}

int remove_http(const char* pkg_name) {
    uint32_t target_ip = inet_addr(REPO_HOST);
    if (target_ip == 0) target_ip = gethostbyname(REPO_HOST);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(REPO_PORT);
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        close(sock);
        return -1;
    }

    char header[256];
    sprintf(header, "POST /remove HTTP/1.0\r\n"
                    "Host: %s\r\n"
                    "Content-Length: %d\r\n\r\n", 
                    REPO_HOST, (int)strlen(pkg_name));

    int h_sent = 0, h_len = strlen(header);
    while(h_sent < h_len) {
        int r = send(sock, header + h_sent, h_len - h_sent, 0);
        if (r > 0) h_sent += r; else yield();
    }
    
    int b_sent = 0, b_len = strlen(pkg_name);
    while(b_sent < b_len) {
        int r = send(sock, pkg_name + b_sent, b_len - b_sent, 0);
        if (r > 0) b_sent += r; else yield();
    }

    
    char resp_buf[128];
    while (recv(sock, resp_buf, sizeof(resp_buf), 0) > 0) yield();

    close(sock);
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
        printf("pkg-tool - 8086-OS Package Manager\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        printf("Usage: pkg install <package_name>\n");
        printf("  pkg search <name>  - Search for a package\n");
        printf("  pkg upload <file> <name> - Upload local file to repo\n");
        printf("  pkg remove <name>  - Remove package completely\n");
        return 1;
    }

    char* command = argv[1];
    char* pkg_name = argv[2];

    if (strcmp(command, "install") == 0) {
        printf("Syncing repository index...\n");
        
        int index_size = 0;
        char* index_data = download_http("packages.txt", -1, &index_size);
        
        if (!index_data) {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Error: Failed to fetch packages.txt from repo.\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
            return 1;
        }
        
        char target_filename[64] = "";
        char* line = index_data;
        int found = 0;

        while (*line) {
            char* next_line = strchr(line, '\n');
            if (next_line) *next_line = '\0';
            
            char entry_name[64];
            char entry_file[64];
            
            int i = 0, j = 0;
            while(line[i] && line[i] != ' ') entry_name[j++] = line[i++];
            entry_name[j] = '\0';
            
            while(line[i] == ' ') i++; 
            
            j = 0;
            while(line[i] && line[i] != '\r' && line[i] != '\n') entry_file[j++] = line[i++];
            entry_file[j] = '\0';

            if (strcmp(entry_name, pkg_name) == 0) {
                strcpy(target_filename, entry_file);
                found = 1;
                break;
            }

            if (!next_line) break;
            line = next_line + 1;
        }

        free(index_data);

        if (!found) {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Error: Package '%s' not found in repository.\n", pkg_name);
            set_color(COLOR_WHITE, COLOR_BLACK);
            return 1;
        }

        printf("Package found! Downloading %s...\n", target_filename);

        char out_path[128];
        sprintf(out_path, "/path/%s", target_filename);

        int fd = open(out_path, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0) {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Error: Could not open %s for writing. Are you in read-only mode?\n", out_path);
            set_color(COLOR_WHITE, COLOR_BLACK);
            return 1;
        }

        int file_size = 0;
        download_http(target_filename, fd, &file_size);
        close(fd);

        set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("\nSuccessfully installed '%s' (%d bytes) to %s\n", pkg_name, file_size, out_path);
        set_color(COLOR_WHITE, COLOR_BLACK);
        
    }
    else if (strcmp(command, "search") == 0) {
        printf("Searching for packages matching '%s'...\n", pkg_name);
        int index_size = 0;
        char* index_data = download_http("packages.txt", -1, &index_size);
        
        if (!index_data) {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Error: Failed to fetch packages.txt for searching.\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
            return 1;
        }

        char* line = index_data;
        int found = 0;
        printf("\nResult:\n");
        while (*line) {
            char* next_line = strchr(line, '\n');
            if (next_line) *next_line = '\0';
            if (strstr(line, pkg_name) != NULL) {
                printf("  %s\n", line);
                found = 1;
            }
            if (!next_line) break;
            line = next_line + 1;
        }
        free(index_data);
        if (!found) printf("No packages found.\n"); else printf("End of search.\n");
    }
    else if (strcmp(command, "upload") == 0) {
        if (argc < 4) {
            printf("Usage: pkg upload <local_file_path> <package_name>\n");
            return 1;
        }
        char* local_path = argv[2];
        char* pkg_name = argv[3];
        
        printf("Uploading %s as package '%s'...\n", local_path, pkg_name);
        if (upload_http(local_path, pkg_name) > 0) {
            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            printf("Uploaded successfully!\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Upload failed.\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
        }
    }
    else if (strcmp(command, "remove") == 0) {
        if (argc < 3) {
            printf("Usage: pkg remove <package_name>\n");
            return 1;
        }
        char* pkg_name = argv[2];
        
        printf("Removing package '%s' from repository...\n", pkg_name);
        if (remove_http(pkg_name) > 0) {
            set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
            printf("Removed successfully!\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
        } else {
            set_color(COLOR_LIGHT_RED, COLOR_BLACK);
            printf("Remove failed.\n");
            set_color(COLOR_WHITE, COLOR_BLACK);
        }
    }
    else {
        printf("Unknown command: %s\n", command);
    }
    return 0;
}
