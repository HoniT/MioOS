// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================
// ahci.cpp
// AHCI driver implementation
// ========================================

#include <drivers/ahci.hpp>
#include <drivers/pci.hpp>
#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <mm/heap.hpp>
#include <drivers/pit.hpp>
#include <x86/interrupts/idt.hpp>
#include <graphics/vga_print.hpp>
#include <lib/data/list.hpp>

static data::list<AhciDriver*> drivers = data::list<AhciDriver*>();

void AhciDriver::handle_irq_for_all(InterruptRegisters* regs) {
    for(AhciDriver* ahci : drivers) {
        ahci->irq_handler(regs);
    }
}

void AhciDriver::irq_handler(InterruptRegisters* regs) {
    // Check global interrupt status. Write back its value
    uint32_t is = hba->is;
    // If 0, the interrupt wasn't from this AHCI controller
    if (is == 0) return;

    // Checking every port
    for(int i = 0; i < 32; i++) {
        if(!(is & (1 << i))) continue;

        HBA_PORT* port = &hba->ports[i];

        // 3. Check the specific Port Interrupt Status
        uint32_t port_is = port->is;

        // Bit 0: DHRS (Device to Host Register FIS) - Command finished
        if (port_is & (1 << 0)) {
        }

        // Bit 30: TFES (Task File Error Status) - Something went wrong
        if (port_is & (1 << 30)) {
            kprintf(LOG_ERROR, "Port %d: Disk Error\n", i);
        }
        port->is = port_is;
    }
    hba->is = is;
}

void AhciDriver::start_cmd(HBA_PORT* port) {
    // Wait until CR (Command List Running) is clear
    while (port->cmd & (1 << 15));

    // Set FRE (FIS Receive Enable) and ST (Start)
    port->cmd |= (1 << 4);
    port->cmd |= (1 << 0);
}

void AhciDriver::stop_cmd(HBA_PORT* port) {
    // Clear ST (Start) and FRE (FIS Receive Enable)
    port->cmd &= ~(1 << 0);
    port->cmd &= ~(1 << 4);

    // Wait until FR (bit14), CR (bit15) are cleared
	while(true)
	{
		if (port->cmd & (1 << 14))
			continue;
		if (port->cmd & (1 << 15))
			continue;
		break;
	}
}

/// @brief Initializes a PCI device for AHCI
/// @param pci_dev PCI device
void AhciDriver::init_dev(PciDevice* pci_dev) {
    AhciDriver* driver = new AhciDriver();
    driver->pci_dev = pci_dev;

    // Getting command register (PCI_HEADER_0x0_COMMAND) of the PCI device
    uint16_t cmd = pci_dev->read_word(PCI_HEADER_0x0_COMMAND);

    pci::CommandRegister cmdreg;
    cmdreg.raw = cmd;
    // ==== 1. Enable interrupts, DMA, and memory space access ====
    cmdreg.interrupt_disable = 0;
    cmdreg.bus_master = 1;
    cmdreg.mem_space = 1;
    // Rewriting the modified command register
    pci_dev->write_word(PCI_HEADER_0x0_COMMAND, cmdreg.raw);


    // ==== 2. Mapping BAR5 as uncacheable ====
    uint32_t bar_base = pci_dev->get_bar(5);
    vmm::identity_map_region(bar_base, bar_base + 2 * PAGE_SIZE, PRESENT | WRITABLE | NOTCACHABLE);
    driver->hba = (HBA_MEM*)bar_base;


    // ==== 3. BIOS/OS handoff ====
    driver->bios_os_handoff();

    // ==== 4. Resetting controller ====
    // Set GHC.HR (bit 0) to 1 and wait for it to clear.
    driver->hba->ghc |= (1 << 31); 
    driver->hba->ghc |= 1;
    int timeout = 0;
    while ((driver->hba->ghc & 1)) {
        if (timeout >= 1000) {
            kprintf(LOG_ERROR, "Failed resetting the controller for PCI: Bus %hhu Dev %hhu Funct %hhu\n",
                pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());
            return;
        }
        pit::delay(1); // Wait 1ms
        timeout++;
    }

    drivers.add(driver);

    // ==== 5. Registering IRQ handler ====
    int irq = pci::pci_read8(*pci_dev, PCI_HEADER_0x0_INTERRUPT_LINE);
    idt::irq_install_handler(irq, AhciDriver::handle_irq_for_all);

    // ==== 6. Enabling AHCI mode and interrupts ====
    driver->hba->ghc |= (1 << 31); // Bit 31 (AE)
    driver->hba->ghc |= (1 << 1); // Bit 1 (IE)

    // ==== 7. Initializing ports ====
    uint32_t pi = driver->hba->pi; // Ports Implemented mask
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            HBA_PORT* port = &driver->hba->ports[i];
            
            // Check if device is present (SSTS)
            uint32_t ssts = port->ssts;
            uint8_t det = ssts & 0x0F;
            if(det != HBA_PORT_DET_PRESENT) continue;

            // For now only managing SATA
            if(port->sig != SATA_SIG_ATA) continue;

            driver->init_port(port);
            driver->identify_device(port);
        }
    }

    kprintf(LOG_INFO, "Initialized AHCI for PCI device: Bus %hhu Device %hhu Function %hhu\n", pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());
}

void AhciDriver::init_port(HBA_PORT* port) {
    this->stop_cmd(port);

    // 1. Allocating physical memory
    uintptr_t base_phys = (uintptr_t)pmm::alloc_frame(1, false); // Command list (1KB) + FIS (256B)
    uintptr_t cmd_tbl_phys = (uintptr_t)pmm::alloc_frame(1, false); // Command table

    // 2. Memory Map as UNCACHEABLE
    vmm::identity_map_region(base_phys, base_phys + PAGE_SIZE, PRESENT | WRITABLE | NOTCACHABLE);
    vmm::identity_map_region(cmd_tbl_phys, cmd_tbl_phys + PAGE_SIZE, PRESENT | WRITABLE | NOTCACHABLE);
    memset((void*)base_phys, 0, FRAME_SIZE);
    memset((void*)cmd_tbl_phys, 0, FRAME_SIZE);

    // 3. Set command list and received FIS address registers
    // Command List at offset 0
    port->clb = (uint32_t)base_phys;
    port->clbu = (uint32_t)(base_phys >> 32);

    // FIS at offset 1024 (1KB)
    port->fb = (uint32_t)(base_phys + 1024);
    port->fbu = (uint32_t)((base_phys + 1024) >> 32);

    // 5. Setup Command List Entry (Header)
    // We get the pointer to the list we just allocated
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)base_phys;
    
    for (int i = 0; i < 32; i++) {
        // 8 PRDT entries per command table is a safe default
        cmdheader[i].prdtl = 8; 
        
        cmdheader[i].ctba = (uint32_t)cmd_tbl_phys;
        cmdheader[i].ctbau = (uint32_t)(cmd_tbl_phys >> 32);
    }

    this->start_cmd(port);

    // 7. Enable Port Interrupts
    port->ie = (1 << 0) | (1 << 2) | (1 << 29) | (1 << 30);
    
    // 8. Clear Error Status (SERR) to "Reset" the port status
    port->serr = 0xFFFFFFFF;
}

void AhciDriver::identify_device(HBA_PORT* port) {
    // 1. Prepare Buffer (Physical Memory) to receive 512 bytes
    uintptr_t buf_phys = (uintptr_t)pmm::alloc_frame(1, false);
    vmm::identity_map_region(buf_phys, buf_phys + PAGE_SIZE, PRESENT | WRITABLE | NOTCACHABLE);
    memset((void*)buf_phys, 0, 512);

    // 2. Clear Interrupt Status
    port->is = 0xFFFFFFFF;

    // 3. Find Free Slot
    int slot = find_cmd_slot(port);
    if (slot == -1) {
        kprintf(LOG_ERROR, "AHCI: No free command slots\n");
        return;
    }

    // 4. Setup Command Header
    uintptr_t cl_phys = (uintptr_t)port->clb; // Get Command List Address
    HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)cl_phys;
    
    cmdheader[slot].cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t); // FIS Length (5 DWORDS)
    cmdheader[slot].w = 0; // Read
    cmdheader[slot].prdtl = 1; // 1 PRDT Entry

    // 5. Setup Command Table
    HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)(uintptr_t)cmdheader[slot].ctba;
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));

    // 6. Setup PRDT (Point to our buffer)
    cmdtbl->prdt_entry[0].dba = (uint32_t)buf_phys;
    cmdtbl->prdt_entry[0].dbau = (uint32_t)(buf_phys >> 32);
    cmdtbl->prdt_entry[0].dbc = 511; // 512 bytes - 1
    cmdtbl->prdt_entry[0].i = 1; // Interrupt on completion

    // 7. Setup FIS (The Command)
    FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = 0x27; // H2D
    cmdfis->c = 1;           // Command
    cmdfis->command = ATA_CMD_IDENTIFY;

    // 8. Issue Command
    // Wait for BSY and DRQ to be clear
    while ((port->tfd & (AHCI_DEV_BUSY | AHCI_DEV_DRQ)));

    port->ci = (1 << slot); // Issue command to slot!

    // 9. Wait for Completion
    kprintf(LOG_INFO, "AHCI: Sending IDENTIFY...\n");
    
    int timeout = 0;
    while (true) {
        if ((port->ci & (1 << slot)) == 0) break;
        
        // Check for Errors
        if (port->is & (1 << 30)) {
            kprintf(LOG_ERROR, "AHCI: Identify Failed (Disk Error)\n");
            return;
        }

        if (timeout++ > 1000000) {
            kprintf(LOG_ERROR, "AHCI: Identify Timeout\n");
            return;
        }
    }

    kprintf(LOG_INFO, "AHCI: IDENTIFY Complete. Drive Found!\n");

    // TODO: Parse 'buf_phys' to get Model Name and Size
}

/// @brief Performs BIOS/OS handoff for PCI device
void AhciDriver::bios_os_handoff() {
    // If BOH (bit 0) bit is set we can do the handoff
    if(hba->cap2 & (1 << 0)) {
        // Set OS owned semaphore (bit 1 of BOHC register)
        hba->bohc |= (1 << 1);

        // Wait for BIOS to clear "BIOS Owned Semaphore" (bit 0)
        // Sleeping for 2 seconds so we don't stay here forever
        for(int timeout = 0; timeout < 2000; timeout++) {
            if(!(hba->bohc & (1 << 0))) return;

            pit::delay(1);
        }
        failed_handoff = true;
        kprintf(LOG_ERROR, "BIOS/OS handoff failed for pci device: Bus %hhu Dev %hhu Funct %hhu\n",
            pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());
    }
}

// Helper to find a free command slot (bit 0 in SACT and CI registers)
int AhciDriver::find_cmd_slot(HBA_PORT* port) {
    // If bit is 0 in both SACT and CI, the slot is free
    uint32_t slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) return i;
    }
    return -1;
}
