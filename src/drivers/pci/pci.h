#ifndef PCI_H
#define PCI_H

#include <stdint.h>

uint32_t pci_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);
void pci_scan(void);
void pci_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t val);

#endif