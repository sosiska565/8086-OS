#include "drivers/pci/pci.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"
#include "utils/utils.h"

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset){
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}

void pci_scan(void){
    klog("[PCI] Scanning PCI buses for hardware...");

    for(uint16_t bus = 0; bus < 256; bus++){
        for(uint8_t dev = 0; dev < 32; dev++){
            for(uint8_t func = 0; func < 8; func++){ 
                uint32_t id = pci_read(bus, dev, func, 0);

                uint16_t vendor_id = id & 0xFFFF;
                if(vendor_id == 0xFFFF) continue; 

                uint32_t class_reg = pci_read(bus, dev, func, 0x08);
                uint8_t class_code = (class_reg >> 24) & 0xFF;
                uint8_t subclass = (class_reg >> 16) & 0xFF;

                char log_msg[128] = "[PCI] Dev: ";
                char hex[16];
                itoa(vendor_id, hex, 16); strcat(log_msg, hex);
                strcat(log_msg, " | Class: ");
                itoa(class_code, hex, 16); strcat(log_msg, hex);
                strcat(log_msg, " | Sub: ");
                itoa(subclass, hex, 16); strcat(log_msg, hex);
                
                klog(log_msg);
            }
        }
    }
    klog("[PCI] Hardware scan complete.");
}

void pci_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    outl(0xCFC, val);
}