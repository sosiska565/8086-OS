#include "drivers/video/bga/bga.h"
#include "drivers/vga/vga.h"
#include "drivers/io/io.h"
#include "drivers/pci/pci.h"
#include "drivers/video/bga/font.h"

uint32_t *video_memory = 0;
int screenW = 0;
int screenH = 0;

void bga_write_register(uint16_t index, uint16_t data){
    outw(0x01CE, index);
    outw(0x01CF, data);
}

void find_bga_pci(void){
    for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint8_t dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0);
            if ((id & 0xFFFF) == 0x1234 && (id >> 16) == 0x1111) {
                print("BGA Found!\n");
                
                uint32_t bar0 = pci_read(bus, dev, 0, 0x10);
                
                video_memory = (uint32_t*)(bar0 & 0xFFFFFFF0);
                
                print("LFB Address: ");
                printhex((uint32_t)video_memory);
                print("\n");
                return;
            }
        }
    }

    print("Error: BGA Device not found!\n");
}

void init_bga(int width, int height){
    screenW = width;
    screenH = height;

    find_bga_pci();

    if(video_memory == 0) return;

    bga_write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    bga_write_register(VBE_DISPI_INDEX_XRES, screenW);
    bga_write_register(VBE_DISPI_INDEX_YRES, screenH);
    bga_write_register(VBE_DISPI_INDEX_BPP, BPP);
    
    bga_write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

void put_pixel(int x, int y, uint32_t color){
    if(video_memory == 0) return;

    if(x < 0 || x >= screenW || y < 0 || y >= screenH) return;

    //offset = y * width + x
    video_memory[y * screenW + x] = color;
}

void clear_screen_bga(uint32_t color){
    if(video_memory == 0) return;

    for(int i = 0; i < screenW * screenH; i++){
        video_memory[i] = color;
    }
}

void set_video_mode(int mode){
    if(mode == VIDEO_MODE_GRAPHICS){
        init_bga(800, 600);
        clear_screen_bga(0x00000000);
    } else {
        bga_write_register(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    }
}

void bga_draw_char(int x, int y, char c, uint32_t color, uint32_t bgcolor){
    uint8_t* glyph = font8x8_basic[(int)((unsigned char)c)];

    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];

        for (int col = 0; col < 8; col++) {
            int is_pixel_set = (line >> col) & 1;
            int draw_x = x + col;
            
            if (is_pixel_set) {
                put_pixel(draw_x, y + row, color);
            } else {
                put_pixel(draw_x, y + row, bgcolor);
            }
        }
    }
}

void bga_print_string(int x, int y, char* str, uint32_t color) {
    int cursor_x = x;
    int cursor_y = y;
    
    for(int i=0; str[i] != 0; i++) {
        bga_draw_char(cursor_x, cursor_y, str[i], color, 0);
        cursor_x += 8;
    }
}