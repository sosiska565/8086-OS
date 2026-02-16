#include "programs/system/memory_viewer/memory_viewer.h"
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "drivers/video/graphics.h"
#include "multitask/task.h"

static const char *hex_chars = "0123456789ABCDEF";

void mainProc(){
    Window *win = wm_create_window(VGA32_COLOR_BLUE);
    set_current_output_window(win);
    for(int i = 0; i < ((win->cols - 18) / 2); i++){
        printf(" ");
    }
    printf("%C%s", VGA32_COLOR_YELLOW, "MEMORY VIEWER V1.0\n");

    int i = 0;
}

static int main(void){
    create_process((void (*)(int, char**))mainProc, 0, 0);
    return 0;
}

memory_viewer_t memoryViewer = {
    .main = main
};