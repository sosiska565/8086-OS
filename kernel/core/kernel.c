#include <stdint.h>
#include "drivers/vga/vga.h"
#include "drivers/keyboard/keyboardDriver.h"
#include "idt/idt.h"
#include "system_apps/console/console.h"
#include "drivers/timer/timer.h"
#include "mm/memory.h"
#include "drivers/speaker/speaker.h"
#include "multiboot.h"
#include "drivers/file/initrd.h"
#include "global.h"
#include "fs/fat/fat32.h"
#include "drivers/mouse/mouse.h"
#include "utils/utils.h"
#include "drivers/pci/pci.h"
#include "drivers/video/bga/gfx_console.h"
#include "drivers/video/vesa.h"
#include "task/task.h"
#include "mm/paging.h"
#include "system_apps/initd/initd.h"
#include "drivers/AHCI/AHCI.h"
#include "drivers/file/ATA/ATA.h"
#include "fs/vfs.h"
#include "fs/fd.h"
#include "fs/cache.h"
#include "utils/sysconfig.h"

#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/timeouts.h"
#include "drivers/net/rtl8139.h"

struct netif rtl8139_netif;
extern err_t ethernetif_init(struct netif *netif);

unsigned short isReadMode = 0;
int $;
char* path = "/";
struct multiboot_info* mbi;

void enable_fpu_sse(void) {
    uint32_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); 
    cr0 |= (1 << 1);  
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);  
    cr4 |= (1 << 10); 
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
    __asm__ volatile("fninit");
}

void kmain(unsigned long magic, unsigned long mb_info_addr){
    if(magic != 0x2BADB002){ panic("Magic value is not correct"); }

    pic_remap();
    idt_install();
    idt_init();
    timer_install();
    keyboard_init();

    __asm__ volatile("sti");
    mbi = (struct multiboot_info*) mb_info_addr;

    klog("[INIT] Kernel started booting.");

    heap_init();
    klog("[INIT] Heap memory manager initialized.");
    
    init_disk_cache();
    fd_init();

    init_vesa();
    init_gfx_console();
    klog("[INIT] VESA Graphics and BGA Console initialized.");

    init_paging();
    klog("[INIT] Paging and Virtual Memory enabled.");

    enable_fpu_sse();
    klog("[INIT] FPU and SSE extensions enabled.");

    pci_scan();
    disk_manager_init();
    ahci_init();

    rtl8139_init();

    lwip_init();
    dns_init(); 
    
    ip4_addr_t ipaddr, netmask, gw;
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);
    ip4_addr_set_zero(&gw);

    netif_add(&rtl8139_netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, netif_input);
    netif_set_default(&rtl8139_netif);
    netif_set_up(&rtl8139_netif);

    dhcp_start(&rtl8139_netif);
    klog("[NET] DHCP Client started!");
    
    initrd_files(mbi);
    vfs_init();

    srand(get_ticks());
    init_tasking();
    mouse_init();

    klog("[INIT] Spawning Initd process...");
    
    create_process(initd, 0, 0, "initd", kernel_dir, -1, -1);

    int dns_forced = 0;
    while(1) { 
        int32_t flags = save_flags(); 
        
        
        rtl8139_poll();       
        
        
        sys_check_timeouts(); 

        
        restore_flags(flags); 

        // if (!dns_forced && rtl8139_netif.ip_addr.addr != 0) {
        //     ip_addr_t dns_server;
        //     dns_server.addr = 0x08080808; 
        //     dns_setserver(0, &dns_server); 
        //     dns_forced = 1;
        //     klog("[NET] DHCP complete. Forced direct DNS to 8.8.8.8");
        // }
        __asm__ volatile("hlt"); 
    }
}