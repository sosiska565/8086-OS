#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

#define RTL_MAC0        0x00
#define RTL_MAR0        0x08
#define RTL_TX_STATUS0  0x10
#define RTL_TX_ADDR0    0x20
#define RTL_RX_BUF      0x30
#define RTL_COMMAND     0x37
#define RTL_CAPR        0x38  
#define RTL_INTR_MASK   0x3C
#define RTL_INTR_STATUS 0x3E
#define RTL_TX_CONFIG   0x40
#define RTL_RX_CONFIG   0x44
#define RTL_CONFIG_1    0x52

#define RTL_CMD_RESET   0x10
#define RTL_CMD_RX_EN   0x08
#define RTL_CMD_TX_EN   0x04
#define RTL_CMD_EMPTY   0x01

#define RX_BUF_SIZE     32768

void rtl8139_init(void);
void rtl8139_send_packet(uint8_t* data, uint32_t len);
void rtl8139_test_send(void);
void rtl8139_handler_c(void);
void rtl8139_poll(void);

#endif