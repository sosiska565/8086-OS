/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/net/rtl8139.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "drivers/net/rtl8139.h"
#include "drivers/pci/pci.h"
#include "drivers/io/io.h"
#include "mm/memory.h"
#include "utils/utils.h"
#include "drivers/vga/vga.h" 
#include "idt/idt.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

uint16_t rtl_io_base = 0;
uint8_t  rtl_mac[6];
uint8_t* rx_buffer;
uint8_t  tx_cur = 0;
static uint16_t rx_read_ptr = 0;


static uint8_t* rtl_tx_buffers[4];

extern struct netif rtl8139_netif;
extern void rtl8139_irq_handler();

void rtl8139_init(void) {
    klog("[NET] Searching for RTL8139 network card...");

    uint8_t target_bus = 0, target_dev = 0, target_func = 0;
    int found = 0;

    for(uint16_t bus = 0; bus < 256; bus++) {
        for(uint8_t dev = 0; dev < 32; dev++) {
            for(uint8_t func = 0; func < 8; func++) { 
                uint32_t id = pci_read(bus, dev, func, 0);
                if((id & 0xFFFF) == 0xFFFF) continue;
                
                uint16_t vendor = id & 0xFFFF;
                uint16_t device = (id >> 16) & 0xFFFF;

                if (vendor == 0x10EC && device == 0x8139) {
                    target_bus = bus;
                    target_dev = dev;
                    target_func = func; 
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }
        if (found) break;
    }

    if (!found) {
        klog("[NET] RTL8139 not found on PCI bus.");
        return;
    }

    uint32_t bar0 = pci_read(target_bus, target_dev, target_func, 0x10);
    rtl_io_base = bar0 & 0xFFFFFFFC; 

    uint8_t irq = pci_read(target_bus, target_dev, target_func, 0x3C) & 0xFF;

    uint32_t pci_cmd = pci_read(target_bus, target_dev, target_func, 0x04);
    pci_cmd |= 0x04; 
    pci_write(target_bus, target_dev, target_func, 0x04, pci_cmd);

    char msg[64];
    sprintf(msg, "[NET] RTL8139 found! I/O Base: 0x%x, IRQ: %d", rtl_io_base, irq);
    klog(msg);

    idt_set_gate(32 + irq, (uint32_t)rtl8139_irq_handler);

    if (irq < 8) {
        outb(0x21, inb(0x21) & ~(1 << irq));
    } else {
        outb(0xA1, inb(0xA1) & ~(1 << (irq - 8)));
        outb(0x21, inb(0x21) & ~(1 << 2)); 
    }

    outb(rtl_io_base + RTL_CONFIG_1, 0x00);
    outb(rtl_io_base + RTL_COMMAND, RTL_CMD_RESET);
    while((inb(rtl_io_base + RTL_COMMAND) & RTL_CMD_RESET) != 0) { }

    rx_buffer = (uint8_t*)kmalloc_a(RX_BUF_SIZE + 16 + 1500); 
    fast_memset(rx_buffer, 0, (RX_BUF_SIZE + 16 + 1500) / 4);
    outl(rtl_io_base + RTL_RX_BUF, (uint32_t)rx_buffer);

    
    for(int i = 0; i < 4; i++) {
        rtl_tx_buffers[i] = (uint8_t*)kmalloc_a(2048);
        fast_memset(rtl_tx_buffers[i], 0, 2048 / 4);
    }

    outw(rtl_io_base + RTL_INTR_MASK, 0x0005); 
    outl(rtl_io_base + RTL_RX_CONFIG, 0x8F | (2 << 11)); 
    outb(rtl_io_base + RTL_COMMAND, RTL_CMD_RX_EN | RTL_CMD_TX_EN);

    for(int i = 0; i < 6; i++) {
        rtl_mac[i] = inb(rtl_io_base + RTL_MAC0 + i);
    }

    char mac_msg[128];
    sprintf(mac_msg, "[NET] MAC Address: %x:%x:%x:%x:%x:%x", 
        rtl_mac[0], rtl_mac[1], rtl_mac[2], rtl_mac[3], rtl_mac[4], rtl_mac[5]);
    klog(mac_msg);
}

void rtl8139_send_packet(uint8_t* data, uint32_t len) {
    if(rtl_io_base == 0) return;

    
    
    while ((inl(rtl_io_base + RTL_TX_STATUS0 + (tx_cur * 4)) & (1 << 13)) == 0) {
        __asm__ volatile("pause"); 
    }

    fast_memcpy(rtl_tx_buffers[tx_cur], data, len);

    outl(rtl_io_base + RTL_TX_ADDR0 + (tx_cur * 4), (uint32_t)rtl_tx_buffers[tx_cur]);
    outl(rtl_io_base + RTL_TX_STATUS0 + (tx_cur * 4), len);

    tx_cur++;
    if(tx_cur > 3) tx_cur = 0;
}

void rtl8139_handler_c(void) {
    if (rtl_io_base == 0) return;
    
    uint16_t status = inw(rtl_io_base + RTL_INTR_STATUS);
    
    if (status & 0x01) { 
        rtl8139_poll();
    }
    
    outw(rtl_io_base + RTL_INTR_STATUS, status);
}

void rtl8139_poll(void) {
    if (rtl_io_base == 0) return;

    while ((inb(rtl_io_base + RTL_COMMAND) & RTL_CMD_EMPTY) == 0) {
        uint16_t *rx_header = (uint16_t*)(rx_buffer + rx_read_ptr);
        uint16_t packet_status = rx_header[0];
        uint16_t packet_length = rx_header[1];

        if ((packet_status & 1) && (packet_length > 4)) {
            uint16_t data_length = packet_length - 4; 
            uint8_t *packet_data = (uint8_t*)(rx_buffer + rx_read_ptr + 4);

            struct pbuf *p = pbuf_alloc(PBUF_RAW, data_length, PBUF_POOL);
            if (p != NULL) {
                pbuf_take(p, packet_data, data_length);
                if (rtl8139_netif.input(p, &rtl8139_netif) != 0) { 
                    pbuf_free(p);
                }
            } else {
                break; 
            }
        }

        rx_read_ptr = (rx_read_ptr + packet_length + 4 + 3) & ~3;
        if (rx_read_ptr >= RX_BUF_SIZE) {
            rx_read_ptr -= RX_BUF_SIZE;
        }

        outw(rtl_io_base + RTL_CAPR, rx_read_ptr - 16);
    }
}
