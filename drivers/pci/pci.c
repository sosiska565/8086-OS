#include "drivers/pci/pci.h"
#include "drivers/io/io.h"
#include "drivers/vga/vga.h"

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset){
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    return inl(0xCFC);
}

void pci_scan(void){
    printf("PCI scanning...\n");

    for(uint16_t bus = 0; bus < 256; bus++){
        for(uint8_t dev = 0; dev < 32; dev++){
            uint32_t id = pci_read(bus, dev, 0, 0);

            uint16_t vendor_id = id & 0xFFFF;
            uint16_t device_id = (id >> 16) & 0xFFFF;

            if(vendor_id == 0xFFFF) continue;

            printf("Found device! Bus: ");
            printf("%d", bus);
            printf(" device: ");
            printf("%d", dev);
            printf(" [Vendor: ");
            printf("%X", vendor_id);
            printf(" Device: ");
            printf("%X", device_id);

            uint32_t class_reg = pci_read(bus, dev, 0, 0x08);
            uint8_t class_code = (class_reg >> 24) & 0xFF;
            uint8_t subclass = (class_reg >> 16) & 0xFF;
            
            printf(" Class: ");
            printf("%X", class_code);
            printf("]");
            printf("\n");
        }
    }
}

void pci_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (func << 8) | (offset & 0xFC);
    outl(0xCF8, address);
    outl(0xCFC, val);
}