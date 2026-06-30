#include <oslib.h>
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#define BUFFER_SIZE 16384

void parse_url(char *url, char *host, int *port, char *path, int *is_https) {
    *port = 80;
    *is_https = 0;
    strcpy(path, "/");
    
    char *p = url;
    
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
    } else if (strncmp(p, "https://", 8) == 0) {
        p += 8;
        *port = 443;
        *is_https = 1;
    }

    char *slash = strchr(p, '/');
    if (slash) {
        strcpy(path, slash);
        int host_len = slash - p;
        strncpy(host, p, host_len);
        host[host_len] = '\0';
    } else {
        strcpy(host, p);
    }

    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        *port = atoi(colon + 1);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: curl <url>\n");
        printf("Try: curl http://ip-api.com/csv\n");
        printf("     curl https://example.com/\n");
        return 1;
    }

    char host[128] = {0};
    char path[256] = {0};
    int port = 80;
    int is_https = 0;

    parse_url(argv[1], host, &port, path, &is_https);

    int sock = -1;
    
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;

    if (is_https) {
        mbedtls_net_init(&server_fd);
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_entropy_init(&entropy);

        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)"curl", 4);
        
        char port_str[16];
        sprintf(port_str, "%d", port);

        if (mbedtls_net_connect(&server_fd, host, port_str, MBEDTLS_NET_PROTO_TCP) != 0) {
            printf("curl: Failed to connect to %s:%d (HTTPS)\n", host, port);
            return 1;
        }

        mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_NONE);
        mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
        mbedtls_ssl_setup(&ssl, &conf);
        mbedtls_ssl_set_hostname(&ssl, host);
        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        if (mbedtls_ssl_handshake(&ssl) != 0) {
            printf("curl: SSL handshake failed\n");
            return 1;
        }
    } else {
        uint32_t target_ip = inet_addr(host);
        if (target_ip == 0) target_ip = gethostbyname(host);
        
        if (target_ip == 0 || target_ip == 0xFFFFFFFF) {
            printf("curl: Could not resolve host: %s\n", host);
            return 1;
        }

        sock = socket(AF_INET, SOCK_STREAM, 0); 
        if (sock < 0) return 1;

        struct sockaddr_in dest;
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = htons(port); 
        dest.sin_addr.s_addr = target_ip;

        if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
            printf("curl: Failed to connect to %s:%d\n", host, port);
            close(sock);
            return 1;
        }
    }

    char request[512];
    sprintf(request, 
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: curl/7.81.0\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n\r\n", 
        path, host);

    if (is_https) {
        mbedtls_ssl_write(&ssl, (const unsigned char*)request, strlen(request));
    } else {
        send(sock, request, strlen(request), 0);
    }
    
    char buf[BUFFER_SIZE];
    int bytes_received;
    int header_passed = 0;

    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    
    while (1) {
        if (is_https) {
            bytes_received = mbedtls_ssl_read(&ssl, (unsigned char*)buf, BUFFER_SIZE - 1);
        } else {
            bytes_received = recv(sock, buf, BUFFER_SIZE - 1, 0);
        }

        if (bytes_received <= 0) break;

        buf[bytes_received] = '\0'; 
        char *data_to_print = buf;

        if (!header_passed) {
            char *body_start = strstr(buf, "\r\n\r\n");
            if (body_start) {
                header_passed = 1;
                data_to_print = body_start + 4; 
            } else {
                continue; 
            }
        }
        write(1, data_to_print, strlen(data_to_print));
    }

    set_color(COLOR_WHITE, COLOR_BLACK);
    printf("\n");

    if (is_https) {
        mbedtls_ssl_close_notify(&ssl);
        mbedtls_ssl_free(&ssl);
        mbedtls_ssl_config_free(&conf);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        mbedtls_net_free(&server_fd);
    } else {
        close(sock);
    }
    
    return 0;
}