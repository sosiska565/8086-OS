#include <oslib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: nslookup <domain>\n");
        return 1;
    }

    printf("Server:    Local DNS via lwIP\n");
    printf("Address:   %s\n\n", argv[1]);

    uint32_t ip = gethostbyname(argv[1]);

    if (ip == 0 || ip == 0xFFFFFFFF) {
        set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("*** Can't find %s: No answer\n", argv[1]);
        set_color(COLOR_WHITE, COLOR_BLACK);
        return 1;
    }

    uint8_t *ip_bytes = (uint8_t*)&ip;
    set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
    printf("Name:      %s\n", argv[1]);
    printf("Address:   %d.%d.%d.%d\n", ip_bytes[0], ip_bytes[1], ip_bytes[2], ip_bytes[3]);
    set_color(COLOR_WHITE, COLOR_BLACK);

    return 0;
}