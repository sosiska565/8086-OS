/*
 *  SPDX-License-Identifier: MIT
 *
 *  8086-OS/drivers/net/ethernetif.c
 *
 *  Copyright (C) 2026  sosiska565
 *
 *  May be freely distributed as part of 8086-OS.
 */

#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"
#include "drivers/net/rtl8139.h"
#include "utils/utils.h"
#include "mm/memory.h" 
#include "drivers/timer/timer.h"

extern uint8_t rtl_mac[6]; 




sys_prot_t sys_arch_protect(void) {
    return save_flags(); 
}

void sys_arch_unprotect(sys_prot_t pval) {
    restore_flags(pval); 
}



static err_t low_level_output(struct netif *netif, struct pbuf *p) {
    uint8_t buffer[1514]; 
    uint16_t copied = 0;

    struct pbuf *q;
    for(q = p; q != NULL; q = q->next) {
        fast_memcpy(buffer + copied, q->payload, q->len);
        copied += q->len;
    }

    
    
    if (copied < 60) {
        fast_memset(buffer + copied, 0, 60 - copied);
        copied = 60;
    }

    
    rtl8139_send_packet(buffer, copied);

    return ERR_OK;
}


err_t ethernetif_init(struct netif *netif) {
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    netif->name[0] = 'e';
    netif->name[1] = 'n';

    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    for(int i = 0; i < 6; i++) {
        netif->hwaddr[i] = rtl_mac[i];
    }

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    return ERR_OK;
}

u32_t sys_now(void) {
    return (u32_t)get_ticks();
}
