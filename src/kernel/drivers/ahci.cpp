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

    kprintf(LOG_INFO, "Initialized AHCI for PCI device: Bus %hhu Device %hhu Function %hhu\n", pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());
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
