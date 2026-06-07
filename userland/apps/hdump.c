#include <oslib.h>

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("Usage: hdump <file>\n");
        return 1;
    }

    int sz = get_file_size(argv[1]);
    if(sz <= 0) {
        printf("File not found or empty.\n");
        return 1;
    }

    uint8_t* buf = malloc(sz);
    read_file(argv[1], buf);

    set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("Hex dump of %s (%d bytes):\n", argv[1], sz);
    set_color(COLOR_WHITE, COLOR_BLACK);

    
    int limit = sz > 256 ? 256 : sz;
    for(int i = 0; i < limit; i++) {
        
        if (buf[i] < 16) printf("0"); 
        printf("%x ", buf[i]);
        
        if((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");

    free(buf);
    return 0;
}