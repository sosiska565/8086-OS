#include <oslib.h>

#define BUFFER_SIZE 1024

int main(int argc, char** argv) {
    if (argc < 2) {
        set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
        printf("dict - Online Dictionary Tool\n");
        set_color(COLOR_WHITE, COLOR_BLACK);
        printf("Usage: dict <word>\n");
        return 1;
    }

    char *word = argv[1];
    char *domain = "dict.org";
    
    printf("Looking up '%s'...\n", word);
    
    uint32_t target_ip = gethostbyname(domain);
    if (target_ip == 0 || target_ip == 0xFFFFFFFF) {
        printf("dict: Could not resolve %s\n", domain);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock < 0) return 1;

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(2628); 
    dest.sin_addr.s_addr = target_ip;

    if (connect(sock, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        printf("dict: Connection failed\n");
        close(sock);
        return 1;
    }

    char request[128];
    
    sprintf(request, "DEFINE ! \"%s\"\r\nQUIT\r\n", word);
    send(sock, request, strlen(request), 0);
    
    char buf[BUFFER_SIZE];
    int bytes_received;

    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    
    while ((bytes_received = recv(sock, buf, BUFFER_SIZE - 1, 0)) > 0) {
        buf[bytes_received] = '\0';
        
        
        
        printf("%s", buf);
    }

    set_color(COLOR_WHITE, COLOR_BLACK);
    close(sock); 
    return 0;
}