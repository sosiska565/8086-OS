#include "oslib.h"
#include "strings.h"

#pragma pack(push, 1)

typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset_data;
} BMPFileHeader;

typedef struct {
    uint32_t size;
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bit_count;
    uint32_t compression;
    uint32_t size_image;
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t colors_used;
    uint32_t colors_important;
} BMPInfoHeader;

#pragma pack(pop)

void* memset(void *ptr, int value, int num) {
    uint8_t *p = (uint8_t*)ptr;
    for(int i=0; i<num; i++) p[i] = value;
    return ptr;
}

void main(int argc, char **argv){
    cls();
    if(argc < 2) return;
    
    int filesize = get_file_size(argv[1]);
    if(filesize <= 0) return;

    uint8_t *buffer = (uint8_t*)malloc(filesize);
    if(!buffer) return;

    read_file(argv[1], buffer);

    BMPFileHeader *file_header = (BMPFileHeader*)buffer;
    BMPInfoHeader *info_header = (BMPInfoHeader*)(buffer + sizeof(BMPFileHeader));

    if (file_header->type != 0x4D42) {
        free(buffer);
        return;
    }
    
    if (info_header->bit_count != 24 && info_header->bit_count != 32) {
        printf("Error: Only 24 or 32 bit BMPs are supported.\n");
        free(buffer);
        return;
    }

    int width = info_header->width;
    int height = info_header->height; 
    int bpp = info_header->bit_count / 8;
    
    int flip_y = 1;
    if (height < 0) {
        height = -height;
        flip_y = 0; 
    }

    printf("Image: %dx%d, %d bpp\n", width, height, info_header->bit_count);

    uint8_t *pixel_data = buffer + file_header->offset_data;

    int row_padded = (width * bpp + 3) & (~3);

    int start_x = (get_screen_width() - width) / 2;
    int start_y = (get_screen_height() - height) / 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            
            int offset = (y * row_padded) + (x * bpp);

            uint8_t blue  = pixel_data[offset + 0];
            uint8_t green = pixel_data[offset + 1];
            uint8_t red   = pixel_data[offset + 2];

            uint32_t color = (red << 16) | (green << 8) | blue;

            int screen_y;
            if (flip_y) {
                screen_y = start_y + (height - 1 - y);
            } else {
                screen_y = start_y + y;
            }
            int screen_x = start_x + x;

            Rect r;
            r.x = screen_x; r.y = screen_y; r.width = 1; r.height = 1; r.color = color;
            draw_rect_filled(&r);
        }
    }

    free(buffer);
    
    getc();
    cls();
}