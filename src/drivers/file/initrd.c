#include "drivers/file/initrd.h"
#include "drivers/vga/vga.h"

void initrd_files(struct multiboot_info* mbi){
    if(!(mbi->flags & 0x08)) {
        print_info("INFO", "No modules loaded via GRUB.", VGA_COLOR_YELLOW, VGA_COLOR_LIGHT_GREY);
        printn_void();
        return;
    }

    print("Modules found: ");
    printnumber(mbi->mods_count);
    printn_void();

    struct multiboot_module* mod = (struct multiboot_module*) mbi->mods_addr;

    for(unsigned int i = 0; i < mbi->mods_count; i++){
        print("Module ");
        printnumber(i);
        print(" loaded at: ");
        printhex(mod[i].mod_start);
        print(" - ");
        printhex(mod[i].mod_end);

        print(" Name: ");
        print((char*)mod[i].string);
        printn_void();

        char* file_content = (char*) mod[i].mod_start;
        print("Content prev: ");
        for(int k = 0; k < 10; k++){
            char c = file_content[k];
            char str[2] = {c, '\0'};
            print(str);
        }
        printn_void();

        mod++;
    }
}