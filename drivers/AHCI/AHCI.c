#include "drivers/AHCI/AHCI.h"
#include "drivers/pci/pci.h"
#include "mm/memory.h"
#include "mm/paging.h"
#include "drivers/vga/vga.h"
#include "drivers/timer/timer.h"
#include "drivers/file/ATA/ATA.h"
#include "utils/utils.h"

HBA_MEM *abar;
HBA_PORT *active_sata_port = 0;

static int check_type(HBA_PORT *port) {
    uint32_t ssts = port->ssts;
    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
        return AHCI_DEV_NULL;

    switch (port->sig) {
        case SATA_SIG_ATAPI: return AHCI_DEV_SATAPI;
        case SATA_SIG_SEMB:  return AHCI_DEV_SEMB;
        case SATA_SIG_PM:    return AHCI_DEV_PM;
        default:             return AHCI_DEV_SATA;
    }
}

static void stop_cmd(HBA_PORT *port) {
    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;

    int timeout = 5000000; 
    while(timeout--) {
        if (port->cmd & HBA_PxCMD_FR) { __asm__ volatile("pause"); continue; }
        if (port->cmd & HBA_PxCMD_CR) { __asm__ volatile("pause"); continue; }
        break;
    }
}

static void start_cmd(HBA_PORT *port) {
    int timeout = 5000000; 
    while ((port->cmd & HBA_PxCMD_CR) && timeout--) {
        __asm__ volatile("pause");
    }
    
    if (port->cmd & HBA_PxCMD_CR) {
        klog("[AHCI] ERROR: Port stuck!");
        return;
    }
    
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST; 
}

static void port_rebase(HBA_PORT *port, int portno) {
    port->ie = 0; 
    stop_cmd(port);

    void *clb = kmalloc_a(1024);
    port->clb = (uint32_t)clb;
    port->clbu = 0;
    fast_memset((void*)port->clb, 0, 1024 / 4);

    void *fb = kmalloc_a(256);
    port->fb = (uint32_t)fb;
    port->fbu = 0;
    fast_memset((void*)port->fb, 0, 256 / 4);

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)port->clb;
    for (int i=0; i<32; i++) {
        cmdheader[i].prdtl = 1;
        void *cmd_tbl = kmalloc_a(256); 
        cmdheader[i].ctba = (uint32_t)cmd_tbl;
        cmdheader[i].ctbau = 0;
        fast_memset(cmd_tbl, 0, 256 / 4);
    }

    start_cmd(port);
}

int find_cmdslot(HBA_PORT *port) {
    uint32_t slots = (port->sact | port->ci);
    for (int i=0; i<32; i++) {
        if ((slots & (1<<i)) == 0) return i;
    }
    return -1;
}

int ahci_read_sector(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf) {
    if (!port) return 0;
    port->is = 0xFFFFFFFF;
    
    int slot = find_cmdslot(port);
    if (slot == -1) return 0;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)port->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t); 
    cmdheader->w = 0;
    cmdheader->prdtl = 1;

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
    fast_memset(cmdtbl, 0, sizeof(HBA_CMD_TBL)/4);

    cmdtbl->prdt_entry[0].dba = (uint32_t)buf;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[0].i = 1; 

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = 0x27;
    cmdfis->c = 1;
    cmdfis->command = 0x25;

    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl>>8);
    cmdfis->lba2 = (uint8_t)(startl>>16);
    cmdfis->device = 1<<6;
    cmdfis->lba3 = (uint8_t)(startl>>24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth>>8);

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    port->ci = 1<<slot; 

    int timeout = 20000000;
    while (timeout--) {
        if ((port->ci & (1<<slot)) == 0) break;
        if (port->is & (1<<30)) return 0;
        __asm__ volatile("pause");
    }

    if (timeout <= 0) return 0; 
    if (port->is & (1<<30)) return 0;
    
    port->is = 0xFFFFFFFF;
    return 1;
}

int ahci_write_sector(HBA_PORT *port, uint32_t startl, uint32_t starth, uint32_t count, uint8_t *buf) {
    if (!port) return 0;
    port->is = 0xFFFFFFFF;
    
    int slot = find_cmdslot(port);
    if (slot == -1) return 0;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)port->clb;
    cmdheader += slot;
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t);
    cmdheader->w = 1;
    cmdheader->prdtl = 1;

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)(cmdheader->ctba);
    fast_memset(cmdtbl, 0, sizeof(HBA_CMD_TBL)/4);

    cmdtbl->prdt_entry[0].dba = (uint32_t)buf;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[0].i = 1;

    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = 0x27; 
    cmdfis->c = 1;           
    cmdfis->command = 0x35;

    cmdfis->lba0 = (uint8_t)startl;
    cmdfis->lba1 = (uint8_t)(startl>>8);
    cmdfis->lba2 = (uint8_t)(startl>>16);
    cmdfis->device = 1<<6; 
    cmdfis->lba3 = (uint8_t)(startl>>24);
    cmdfis->lba4 = (uint8_t)starth;
    cmdfis->lba5 = (uint8_t)(starth>>8);

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    port->ci = 1<<slot; 

    int timeout = 20000000; 
    while (timeout--) {
        if ((port->ci & (1<<slot)) == 0) break;
        if (port->is & (1<<30)) return 0;
        __asm__ volatile("pause");
    }
    
    if (timeout <= 0) return 0;
    if (port->is & (1<<30)) return 0;
    
    port->is = 0xFFFFFFFF;
    return 1;
}

void ahci_init(void) {
    klog("[AHCI] Scanning PCI for AHCI controller...");
    uint8_t bus = 0, dev = 0;
    int found = 0;

    for(bus = 0; bus < 255; bus++) {
        for(dev = 0; dev < 32; dev++) {
            uint32_t id = pci_read(bus, dev, 0, 0);
            if((id & 0xFFFF) == 0xFFFF) continue;
            
            uint32_t class_reg = pci_read(bus, dev, 0, 0x08);
            uint8_t class_code = (class_reg >> 24) & 0xFF;
            uint8_t subclass = (class_reg >> 16) & 0xFF;

            if (class_code == 0x01 && subclass == 0x06) {
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if(!found) {
        klog("[AHCI] Controller not found on PCI bus.");
        return;
    }

    klog("[AHCI] Controller found. Enabling PCI busmaster...");
    uint32_t pci_cmd = pci_read(bus, dev, 0, 0x04);
    pci_cmd |= 0x06;
    pci_write(bus, dev, 0, 0x04, pci_cmd);

    uint32_t abar_phys = pci_read(bus, dev, 0, 0x24) & 0xFFFFFFF0;
    if (abar_phys == 0) {
        klog("[AHCI] ERROR: ABAR memory address is 0.");
        return;
    }

    char log_msg[64] = "[AHCI] Mapping ABAR at ";
    char hex[16];
    itoa(abar_phys, hex, 16);
    strcat(log_msg, hex);
    klog(log_msg);

    uint32_t abar_page = abar_phys & 0xFFFFF000;
    paging_map(abar_page, abar_page, 3);
    paging_map(abar_page + 4096, abar_page + 4096, 3);
    switch_page_directory(kernel_dir);
    
    abar = (HBA_MEM*)abar_phys;
    abar->ghc |= (1 << 31); 
    abar->ghc &= ~(1 << 1); 

    klog("[AHCI] Scanning SATA ports...");
    uint32_t pi = abar->pi;
    for (int i = 0; i < 32; i++) {
        if (pi & (1<<i)) {
            int dt = check_type(&abar->ports[i]);
            if (dt == AHCI_DEV_SATA) {
                char port_msg[64] = "[AHCI] Rebased SATA Drive on Port ";
                char port_num[4];
                itoa(i, port_num, 10);
                strcat(port_msg, port_num);
                klog(port_msg);
                
                abar->ports[i].serr = 0xFFFFFFFF;
                abar->ports[i].is = 0xFFFFFFFF;
                
                abar->ports[i].cmd |= 0x02; 
                for(volatile int w=0; w<10000; w++);

                port_rebase(&abar->ports[i], i);
                
                if (active_sata_port == 0) {
                    active_sata_port = &abar->ports[i];
                }

                if (sys_drive_count < MAX_SYS_DRIVES) {
                    sys_drives[sys_drive_count].type = DRIVE_TYPE_AHCI;
                    sys_drives[sys_drive_count].ahci_port = &abar->ports[i];
                    strcpy(sys_drives[sys_drive_count].name, "AHCI SATA Drive");
                    sys_drive_count++;
                }
            }
        }
    }
    klog("[AHCI] Subsystem setup complete.");
}