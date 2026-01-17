// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================
// ahci.cpp
// AHCI driver implementation
// ========================================

#include <drivers/ahci.hpp>
#include <mm/heap.hpp>
#include <mm/vmm.hpp>
#include <mm/pmm.hpp>
#include <drivers/pit.hpp>
#include <device.hpp>
#include <lib/data/list.hpp>
#include <lib/mem_util.hpp>

#pragma region Initialization

static data::list<AhciDriver*> drivers = data::list<AhciDriver*>();

/// @brief Initializes AHCI HBA for a specific PCI device
/// @param pci_dev PCI device (class_id 0x1 subclass_id 0x6)
void AhciDriver::init_dev(PciDevice* pci_dev) {
    AhciDriver* driver = new AhciDriver();
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
            kprintf(LOG_ERROR, "Port %u is stuck in running state, disabling port. ");
            pci_dev->log_pci_info(); 
                
            pi &= ~(1 << i); // Remove from implemented map so we don't try to use it later
            continue;
        }

        // 1. Check if currently running
        if ((port->cmd & (PxCMD_ST | PxCMD_CR | PxCMD_FRE | PxCMD_FR)) != 0) {
            
            // 2. Clear Start Bit (ST)
            port->cmd &= ~PxCMD_ST;

            // 3. Wait for Command List Running (CR) to clear
            // Don't sleep 500ms blindly. Poll for 500ms.
            wait_bit_clear(port->cmd, PxCMD_CR, 500);

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
                wait_bit_clear(port->cmd, PxCMD_FR, 500);

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

        // Allocating frame for Command List and FIS
        uint32_t frame_addr = (uint32_t)pmm::alloc_frame(1);
        memset((void*)frame_addr, 0, FRAME_SIZE);

        // Command List Base
        port->clb = frame_addr;
        if(driver->hba->cap & (1 << 31)) port->clbu = frame_addr >> 32;

        // FIS Base
        uint32_t fb_phys_addr = frame_addr + 1024;
        port->fb = fb_phys_addr;
        if(driver->hba->cap & (1 << 31)) port->fbu = fb_phys_addr >> 32;

        // Allocating Command Tables (Fix: Required for read/write operations)
        // 32 slots * ~256 bytes per slot = ~8KB (2 pages)
        uint32_t cmd_tbl_addr = (uint32_t)pmm::alloc_frame(2); 
        memset((void*)cmd_tbl_addr, 0, PAGE_SIZE * 2);

        HBA_CMD_HEADER* header_array = (HBA_CMD_HEADER*)port->clb;

        for (int j = 0; j < 32; j++) {
            header_array[j].prdtl = 8; // 8 PRDT entries per command
            
            // Calculate address for this specific table (256 byte steps)
            uint32_t tbl_addr = cmd_tbl_addr + (j * 256);
            
            // Link the header to the table
            header_array[j].ctba = tbl_addr;
            if(driver->hba->cap & (1 << 31)) header_array[j].ctbau = tbl_addr >> 32;
        }

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

    kprintf(LOG_INFO, "Initialized AHCI HBA for ");
    pci_dev->log_pci_info();

    // Scan for drives now that HBA is ready
    driver->probe_ports();
}

#pragma endregion

#pragma region Runtime Dynamics

/// @brief Starts command engine for specific port
/// @return Succession
bool AhciDriver::start_cmd(HBA_PORT* port) {
    // 1. Wait until CR (bit 15) is cleared
    while (port->cmd & PxCMD_CR);

    // 2. Set FRE (bit 4) - Enable FIS Receive
    port->cmd |= PxCMD_FRE;
    
    // 3. Set ST (bit 0) - Start Command Processing
    port->cmd |= PxCMD_ST;
    return true;
}

/// @brief Stops command engine for specific port
/// @return Succession
bool AhciDriver::stop_cmd(HBA_PORT* port) {
    // Clear ST (bit0)
    port->cmd &= ~PxCMD_ST;

    // Clear FRE (bit4)
    port->cmd &= ~PxCMD_FRE;

    // Wait until FR (bit14), CR (bit15) are cleared
    return wait_bit_clear(port->cmd, PxCMD_CR | PxCMD_FR, 500);
}

/// @brief Probes implemented ports for connected devices
void AhciDriver::probe_ports() {
    uint32_t pi = hba->pi; 

    for(int i = 0; i < 32; i++) {
        if (!(pi & (1 << i))) continue;

        HBA_PORT* port = &hba->ports[i];
        
        // Check SATA Status (SSTS) DET (bits 0-3)
        // 3 means "Device detected and Phy communication established"
        if ((port->ssts & 0x0F) != 3) continue;

        // Check IPM (bits 8-11): 1 means "Active state"
        if (((port->ssts >> 8) & 0x0F) != 1) continue; 

        // Check Device Signature
        switch (port->sig) {
            case SATA_SIG_ATAPI:
                kprintf(LOG_INFO, "AHCI Port %d: Found ATAPI Drive (CD-ROM). Skipping...\n", i);
                break;
            case SATA_SIG_ATA:
                kprintf(LOG_INFO, "AHCI Port %d: Found ATA Drive (HDD/SSD). Initializing...\n", i);
                configure_drive(port); 
                break;
            case SATA_SIG_PM:
                kprintf(LOG_INFO, "AHCI Port %d: Found Port Multiplier. Not supported.\n", i);
                break;
            default:
                kprintf(LOG_INFO, "AHCI Port %d: Unknown device (SIG: %x)\n", i, port->sig);
                break;
        }
    }
}

static void swap_string(char* str, int length) {
    for (int i = 0; i < length; i += 2) {
        char temp = str[i];
        str[i] = str[i+1];
        str[i+1] = temp;
    }
}

/// @brief Configures a found drive and prints identity
void AhciDriver::configure_drive(HBA_PORT* port) {
    // Allocate a buffer for Identify (must be phys contiguous)
    SATA_IDENTIFY_DATA* buffer = (SATA_IDENTIFY_DATA*)pmm::alloc_frame(1); 
    memset(buffer, 0, 512);

    if (identify(port, buffer)) {
        char model_str[41];
        char serial_str[21];
        char firmware_str[9];

        memcpy(model_str, buffer->model, 40);
        memcpy(serial_str, buffer->serial, 20);
        memcpy(firmware_str, buffer->firmware, 8);

        swap_string(model_str, 40);
        model_str[40] = 0;
        swap_string(serial_str, 20);
        serial_str[20] = 0;
        swap_string(firmware_str, 8);
        firmware_str[8] = 0;

        uint64_t sectors = buffer->lba48_sectors;
        if (sectors == 0) {
            sectors = buffer->lba28_sectors;
        }
        uint64_t size_bytes = sectors * 512;

        ahci::save_ahci_device(model_str, serial_str, firmware_str, sectors, this, port);

        kprintf(LOG_INFO, "AHCI: Registered Drive. Model: %s, Serial: %s, Firmware: %s, Size: %S\n", 
            model_str, serial_str, firmware_str, get_units(size_bytes));
    } else {
        kprintf(LOG_ERROR, "AHCI: Identify failed for device.\n");
    }

    pmm::free_frame(buffer);
}

#pragma endregion

#pragma region IO Operations

/// @brief Identification Command (ATA_CMD_IDENTIFY)
/// @param buffer Must be 512 bytes, physically contiguous/identity mapped
bool AhciDriver::identify(HBA_PORT* port, SATA_IDENTIFY_DATA* buffer) {
    // 1. Clear interrupt status to clear old pending events
    port->is = 0xFFFFFFFF;
    port->ie = 0;

    // 2. Find a free command slot
    int slot = find_cmdslot(port);
    if (slot == -1) return false;

    // 3. Get Command Header
    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)port->clb;
    cmd_header += slot;

    // 4. Setup Header
    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t); 
    cmd_header->w = 0;      
    cmd_header->prdtl = 1;  

    // 5. Get Command Table
    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));

    // 6. Setup PRDT
    cmd_tbl->prdt_entry[0].dba = (uint32_t)buffer;
    cmd_tbl->prdt_entry[0].dbau = 0; 
    cmd_tbl->prdt_entry[0].dbc = 511; 
    cmd_tbl->prdt_entry[0].i = 1;     

    // 7. Setup SetupFIS (H2D)
    FIS_REG_H2D* cmd_fis = (FIS_REG_H2D*)(&cmd_tbl->cfis);
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c = 1; 
    cmd_fis->command = ATA_CMD_IDENTIFY;
    cmd_fis->device = 0; 

    // 8. Issue Command
    while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ));

    // Ensure engine is running
    if ((port->cmd & PxCMD_ST) == 0) {
        start_cmd(port);
        pit::delay(1);
    }

    // Issue the command
    port->ci = 1 << slot; 
    
    // 9. Wait for completion
    while (true) {
        if (port->is & (1 << 30)) return false; // Error
        if (!(port->ci & (1 << slot))) break;   // Success
    }
    
    return true;
}

/// @brief Read using DMA LBA48
bool AhciDriver::read(HBA_PORT* port, uint64_t sector, uint32_t count, void* buffer) {
    port->is = 0xFFFFFFFF; // Clear interrupts
    int slot = find_cmdslot(port);
    if (slot == -1) return false;

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)port->clb;
    cmd_header += slot;

    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmd_header->w = 0;      // Read
    cmd_header->prdtl = 1;  // Assuming contiguous buffer for simplicity

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));

    // PRDT Setup
    cmd_tbl->prdt_entry[0].dba = (uint32_t)buffer;
    cmd_tbl->prdt_entry[0].dbau = 0; 
    cmd_tbl->prdt_entry[0].dbc = (count * 512) - 1; // 512 bytes per sector
    cmd_tbl->prdt_entry[0].i = 1;

    // FIS Setup
    FIS_REG_H2D* cmd_fis = (FIS_REG_H2D*)(&cmd_tbl->cfis);
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c = 1;
    cmd_fis->command = ATA_CMD_READ_DMA_EX; // LBA48 Read
    
    // LBA48 Encoding
    cmd_fis->lba0 = (uint8_t)sector;
    cmd_fis->lba1 = (uint8_t)(sector >> 8);
    cmd_fis->lba2 = (uint8_t)(sector >> 16);
    cmd_fis->device = 1 << 6; // LBA mode

    cmd_fis->lba3 = (uint8_t)(sector >> 24);
    cmd_fis->lba4 = (uint8_t)(sector >> 32);
    cmd_fis->lba5 = (uint8_t)(sector >> 40);

    cmd_fis->countl = count & 0xFF;
    cmd_fis->counth = (count >> 8) & 0xFF;

    // Issue
    while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ));
    port->ci = 1 << slot;

    // Wait
    while (true) {
        if (port->is & (1 << 30)) return false; 
        if (!(port->ci & (1 << slot))) break;
    }
    return true;
}

/// @brief Write using DMA LBA48
bool AhciDriver::write(HBA_PORT* port, uint64_t sector, uint32_t count, void* buffer) {
    port->is = 0xFFFFFFFF;
    int slot = find_cmdslot(port);
    if (slot == -1) return false;

    HBA_CMD_HEADER* cmd_header = (HBA_CMD_HEADER*)port->clb;
    cmd_header += slot;

    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmd_header->w = 1; // Write bit set
    cmd_header->prdtl = 1;

    HBA_CMD_TBL* cmd_tbl = (HBA_CMD_TBL*)(cmd_header->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL));

    cmd_tbl->prdt_entry[0].dba = (uint32_t)buffer;
    cmd_tbl->prdt_entry[0].dbau = 0;
    cmd_tbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmd_tbl->prdt_entry[0].i = 1;

    FIS_REG_H2D* cmd_fis = (FIS_REG_H2D*)(&cmd_tbl->cfis);
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c = 1;
    cmd_fis->command = ATA_CMD_WRITE_DMA_EX; // LBA48 Write

    cmd_fis->lba0 = (uint8_t)sector;
    cmd_fis->lba1 = (uint8_t)(sector >> 8);
    cmd_fis->lba2 = (uint8_t)(sector >> 16);
    cmd_fis->device = 1 << 6; 

    cmd_fis->lba3 = (uint8_t)(sector >> 24);
    cmd_fis->lba4 = (uint8_t)(sector >> 32);
    cmd_fis->lba5 = (uint8_t)(sector >> 40);

    cmd_fis->countl = count & 0xFF;
    cmd_fis->counth = (count >> 8) & 0xFF;

    while (port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ));
    port->ci = 1 << slot;

    while (true) {
        if (port->is & (1 << 30)) return false;
        if (!(port->ci & (1 << slot))) break;
    }
    return true;
}

#pragma endregion

#pragma region Resets

/// @brief Resets port using COMRESET
/// @param port Port to reset
/// @return Reset succession
bool AhciDriver::port_reset(HBA_PORT* port) {
    kprintf(LOG_INFO, "Calling AHCI port reset ");
    this->pci_dev->log_pci_info();

    // Clearing PxCMD.ST (bit 0)
    port->cmd &= ~1U;

    // Waiting for PxCMD.CR (bit 15), it should be cleared before 500 ms
    this->wait_bit_clear(port->cmd, PxCMD_CR | PxCMD_FR, 500);

    // Software causes a port reset (COMRESET) by writing 1h to the PxSCTL.DET
    port->sctl &= ~0xFU; // Clear current DET (bits 0-3)
    port->sctl |= 1;     // Set DET = 1 (Perform Reset)

    // "Software shall wait at least 1 millisecond before clearing PxSCTL.DET to 0h"
    pit::delay(1);
    port->sctl &= ~0xFU;

    // "Wait for communication... indicated by PxSSTS.DET being set to 3h"
    if (!this->check_connection(port)) {
        // No device detected, or reset failed.
        return false;
    }

    // "Then software should write all 1s to the PxSERR register"
    port->serr = 0xFFFFFFFF;
    return true;
}

/// @brief Resets HBA
/// @return Reset succession
bool AhciDriver::hba_reset() {
    kprintf(LOG_INFO, "Calling AHCI HBA reset. ");
    this->pci_dev->log_pci_info();

    // Setting GHC.HR to 1
    this->hba->ghc |= 1;

    // Waiting 1 second for GHC.HR to clear
    return this->wait_bit_clear(hba->ghc, 1, 1000);
}

#pragma endregion

#pragma region Helpers

/// @brief Checks for connection by checking PxSSTS.DET
/// @param port HBA port to check
/// @return Succession (true if connected, false if not)
bool AhciDriver::check_connection(HBA_PORT* port) {
    int timeout = 1000;
    while (timeout--) {
        // Fix: operator precedence error (added parens)
        if ((port->ssts & 0x0F) == 3) {
            return true;
        }
        pit::delay(1);
    }
    return false;
}

/// @brief Waits a certain time for a bit to clear
/// @param reg Register where the bit resides
/// @param mask Mask to get the bit
/// @param timeout_ms Time to wait for clearing
/// @return Succession
bool AhciDriver::wait_bit_clear(uint32_t reg, uint32_t mask, int timeout_ms) {
    while(--timeout_ms) {
        if(!(reg & mask)) return true;
        pit::delay(1);
    }
    return false;
}

/// @brief Finds a command slot
/// @param port Port to find on
/// @return 0-31 or -1 if none
int8_t AhciDriver::find_cmdslot(HBA_PORT *port) {
    // If not set in SACT and CI, the slot is free
    uint32_t slots = (port->sact | port->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & 1) == 0)
            return i;
        slots >>= 1;
    }
    return -1;
}

#pragma endregion

PciDevice* AhciDriver::get_pci_dev() { return this->pci_dev; }
