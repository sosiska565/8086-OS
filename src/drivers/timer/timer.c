#include "drivers/timer/timer.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "multitask/task.h"
#include "drivers/video/vesa.h"

volatile unsigned long ticks = 0;

static int cpu_idle_ticks = 0;
static int cpu_total_ticks = 0;
uint32_t global_cpu_usage = 0;

void timer_handler_c(void){
    ticks++;
    cpu_total_ticks++;

    if (current_task && current_task->id == 0) {
        cpu_idle_ticks++;
    }

    if (cpu_total_ticks >= 1000) {
        global_cpu_usage = 100 - ((cpu_idle_ticks * 100) / cpu_total_ticks);
        if (global_cpu_usage > 100) global_cpu_usage = 100;
        cpu_idle_ticks = 0;
        cpu_total_ticks = 0;
    }

    outb(0x20, 0x20);

    check_kill_flag();
    task_scheduler();
}

unsigned long get_ticks(void){
    return ticks;
}

uint32_t get_cpu_usage(void) {
    return global_cpu_usage;
}

void timer_install(void) {
    unsigned int divisor = 1193180 / 1000;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);       
    outb(0x40, (divisor >> 8) & 0xFF);
}

void sleep(unsigned long ms) {
    unsigned long end_ticks = get_ticks() + ms;
    while (get_ticks() < end_ticks) {
        __asm__ volatile("hlt");
    }
}