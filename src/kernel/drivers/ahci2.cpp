// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================
// ahci.cpp
// AHCI driver implementation
// ========================================

#include <drivers/ahci2.hpp>
#include <mm/heap.hpp>
#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <drivers/pit.hpp>
#include <lib/data/list.hpp>

#pragma region Initialization

static data::list<AhciDriver2*> drivers = data::list<AhciDriver2*>();

/// @brief Initializes AHCI HBA for a specific PCI device
/// @param pci_dev PCI device (class_id 0x1 subclass_id 0x6)
void AhciDriver2::init_dev(PciDevice* pci_dev) {
    AhciDriver2* driver = new AhciDriver2();
    driver->pci_dev = pci_dev;
    drivers.add(driver);

    // Getting HBA
    uint32_t bar_base = pci_dev->get_bar(5);
    vmm::identity_map_region(bar_base, bar_base + 2 * PAGE_SIZE, PRESENT | WRITABLE | NOTCACHABLE);
    driver->hba = (HBA_MEM*)bar_base;

    /* The following code follows Intel's serial-ata-ahci-spec-rev1-3-1
     * 10.1.2 System Software Specific Initialization sequence */

    // 0. Reseting HBA
    driver->hba->ghc |= GHC_AE;
    driver->hba->ghc |= GHC_HR;
    while (driver->hba->ghc & GHC_HR) pit::delay(1); // Wait for reset to complete (hardware clears bit)

    // 1. Indicate that system software is AHCI
    driver->hba->ghc |= GHC_AE;

    // 2. Determine which ports are implemented by the HBA
    uint32_t pi = driver->hba->pi;

    // 3. Ensure that the controller is not in the running state
    for(int i = 0; i < 32; i++) {
        if (!(pi & (1 << i))) continue;

        HBA_PORT* port = &driver->hba->ports[i];
        int retries = 0;

    check:
        // Safety break: Don't loop forever if hardware is broken
        if (retries > 1) {
            kprintf(LOG_ERROR, "AHCI HBA (Bus %hhu Dev %hhu Funct %hhu) Port %u is stuck in running state, disabling port.", 
                pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function(), i);
            pi &= ~(1 << i); // Remove from implemented map so we don't try to use it later
            continue;
        }

        // 1. Check if currently running
        if ((port->cmd & (PxCMD_ST | PxCMD_CR | PxCMD_FRE | PxCMD_FR)) != 0) {
            
            // 2. Clear Start Bit (ST)
            port->cmd &= ~PxCMD_ST;

            // 3. Wait for Command List Running (CR) to clear
            // Don't sleep 500ms blindly. Poll for 500ms.
            int timeout = 500;
            while (timeout--) {
                if (!(port->cmd & PxCMD_CR)) break;
                pit::delay(1);
            }

            // If still running after timeout, Reset and Retry
            if (port->cmd & PxCMD_CR) {
                driver->port_reset(port);
                retries++;
                goto check;
            }

            // 4. Clear FIS Receive Enable (FRE)
            if (port->cmd & PxCMD_FRE) {
                port->cmd &= ~PxCMD_FRE;

                // 5. Wait for FIS Receive Running (FR) to clear
                timeout = 500;
                while (timeout--) {
                    if (!(port->cmd & PxCMD_FR)) break;
                    pit::delay(1);
                }

                // If still running, Reset and Retry
                if (port->cmd & PxCMD_FR) {
                    driver->port_reset(port);
                    retries++;
                    goto check;
                }
            }
        }
    }

    // 4. Determine how many command slots the HBA supports
    uint8_t ncs = (driver->hba->cap >> 8) & 0x1F; // Bits 8-12 (2^5 0-31)

    // 5. For each implemented port, system software shall allocate memory
    for(int i = 0; i < 32; i++) {
        if (!(pi & (1 << i))) continue;
        HBA_PORT* port = &driver->hba->ports[i];

        uint32_t frame_addr = (uint32_t)pmm::alloc_frame(1);
        memset((void*)frame_addr, 0, FRAME_SIZE);

        // Command List Base
        port->clb = frame_addr;
        if(driver->hba->cap & (1 << 31)) port->clbu = frame_addr >> 32;

        // FIS Base
        uint32_t fb_phys_addr = frame_addr + 1024;
        port->fb = fb_phys_addr;
        if(driver->hba->cap & (1 << 31)) port->fbu = fb_phys_addr >> 32;

        // "After setting PxFB and PxFBU ... set PxCMD.FRE to ‘1’"
        port->cmd |= PxCMD_FRE;
    }

    /* 6. For each implemented port, clear the PxSERR register, 
     * by writing ‘1s’ to each implemented bit location. */
    for(int i = 0; i < 32; i++) 
        if (pi & (1 << i))
            driver->hba->ports[i].serr = 0xFFFFFFFF;
    
    // 7. Determine which events should cause an interrupt
    for(int i = 0; i < 32; i++) {
        if (!(pi & (1 << i))) continue;
        HBA_PORT* port = &driver->hba->ports[i];

        /* system software must always ensure that the 
         * PxIS (clear this first) and IS.IPS (clear this second)
         * registers are cleared to ‘0’ before programming the PxIE and GHC */
        port->is = 0xFFFFFFFF;
        driver->hba->is = IS_IPS(i);

        port->ie = 0xFDC000FF; // Every bit set except reserved bits (bits 8-21, 25) 
    }

    // "To enable the HBA to generate interrupts, system software must also set GHC.IE to a ‘1’."
    driver->hba->ghc |= GHC_IE;

    kprintf(LOG_INFO, "Initialized AHCI HBA for PCI: Bus %hhu Device %hhu Function %hhu\n", 
        pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());
}

#pragma endregion

#pragma region Resets

/// @brief Resets port using COMRESET
/// @param port Port to reset
/// @return Reset succession
bool AhciDriver2::port_reset(HBA_PORT* port) {
    kprintf(LOG_INFO, "Calling AHCI port reset. HBA: Bus %hhu Device %hhu Function %hhu\n",
        pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());

    // Clearing PxCMD.ST (bit 0)
    port->cmd &= ~1U;

    // Waiting for PxCMD.CR (bit 15), it should be cleared before 500 ms
    int timeout = 500;
    while (timeout--) {
        if ((port->cmd & (PxCMD_CR | PxCMD_FR)) == 0)
            break;
        pit::delay(1);
    }

    // Software causes a port reset (COMRESET) by writing 1h to the PxSCTL.DET
    port->sctl &= ~0xFU; // Clear current DET (bits 0-3)
    port->sctl |= 1;     // Set DET = 1 (Perform Reset)

    // "Software shall wait at least 1 millisecond before clearing PxSCTL.DET to 0h"
    pit::delay(1);
    port->sctl &= ~0xFU;

    // "Wait for communication... indicated by PxSSTS.DET being set to 3h"
    timeout = 1000;
    bool connected = false;
    while (timeout--) {
        if (port->ssts & 0x0F == 3) {
            connected = true;
            break;
        }
        pit::delay(1);
    }

    if (!connected) {
        // No device detected, or reset failed.
        return false;
    }

    // "Then software should write all 1s to the PxSERR register"
    port->serr = 0xFFFFFFFF;
    return true;
}

/// @brief Resets HBA
/// @return Reset succession
bool AhciDriver2::hba_reset() {
    kprintf(LOG_INFO, "Calling AHCI HBA reset. HBA: Bus %hhu Device %hhu Function %hhu\n",
        pci_dev->get_bus(), pci_dev->get_device(), pci_dev->get_function());

    // Setting GHC.HR to 1
    this->hba->ghc |= 1;

    uint32_t timeout = 1000; // Waiting 1 second for GHC.HR to clear
    while (timeout--) {
        if (!(hba->ghc & 1)) {
            return true;
        }
        pit::delay(1);
    }

    // The HBA is in a hung or locked state.
    return false;
}

#pragma endregion
