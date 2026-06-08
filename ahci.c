#include <stddef.h>
#include "ahci.h"
#include "screen.h"
#include "pci.h"

static void ahci_print_int(int num)
{
    char buffer[32];
    int i = 0;
    if (num == 0) {
        put_char('0');
        return;
    }
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        put_char(buffer[j]);
    }
}

// AHCI Register offsets
#define HBA_GHC 0x04
#define HBA_PI 0x0C
#define HBA_PORTS 0x100

// Port register offsets
#define PORT_CLB 0x00
#define PORT_FB 0x08
#define PORT_CMD 0x18
#define PORT_SSTS 0x28
#define PORT_CI 0x38

// Port CMD bits
#define PORT_CMD_ST (1 << 0)
#define PORT_CMD_FRE (1 << 4)
#define PORT_CMD_CR (1 << 15)
#define PORT_CMD_FR (1 << 14)

// ATA commands
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_WRITE_DMA 0xCA

// Command header structure (simplified)
typedef struct {
    uint32_t dword0;  // cfl, a, w, p, r, b, c, rsv0, pm_port, prdtl
    uint32_t prdbc;
    uint32_t ctba;
    uint32_t ctbau;
    uint32_t rsv1[4];
} __attribute__((packed)) HbaCmdHeader;

// Physical Region Descriptor
typedef struct {
    uint32_t dba;
    uint32_t dbau;
    uint32_t rsv0;
    uint32_t dbc;
} __attribute__((packed)) HbaPrdt;

// Command table
typedef struct {
    uint8_t cfis[64];
    uint8_t acmd[16];
    uint8_t rsv[48];
    HbaPrdt prdt_entry;
} __attribute__((packed)) HbaCmdTable;

static volatile void* hba_base = 0;
static int ahci_present = 0;
static int active_port = -1;

// Allocate aligned buffers
static HbaCmdHeader cmd_list[32] __attribute__((aligned(1024)));
static HbaCmdTable cmd_table __attribute__((aligned(128)));

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t*)addr = val;
}

static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t*)addr;
}

static uint32_t pci_read(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset)
{
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    asm volatile ("outl %0, %1" : : "a"(address), "Nd"(0xCF8));
    uint32_t result;
    asm volatile ("inl %1, %0" : "=a"(result) : "Nd"(0xCFC));
    return result;
}

static void pci_write(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint32_t value)
{
    uint32_t address = (1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC);
    asm volatile ("outl %0, %1" : : "a"(address), "Nd"(0xCF8));
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(0xCFC));
}

static int find_ahci_controller(void)
{
    print("Scanning for AHCI controller...\n");
    
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint32_t class_rev = pci_read(bus, slot, 0, 0x08);
            uint8_t prog_if = (class_rev >> 8) & 0xFF;
            uint8_t sub_class = (class_rev >> 16) & 0xFF;
            uint8_t base_class = (class_rev >> 24) & 0xFF;
            
            if (base_class == 0x01 && sub_class == 0x06 && prog_if == 0x01) {
                print("Found AHCI at bus ");
                ahci_print_int(bus);
                print(" slot ");
                ahci_print_int(slot);
                print("\n");
                
                uint32_t bar5 = pci_read(bus, slot, 0, 0x24);
                hba_base = (void*)(bar5 & 0xFFFFFFF0);
                print("AHCI base: 0x");
                ahci_print_int((uint32_t)hba_base);
                print("\n");
                
                uint32_t command = pci_read(bus, slot, 0, 0x04);
                command |= (1 << 2) | (1 << 1);
                pci_write(bus, slot, 0, 0x04, command);
                
                return 1;
            }
        }
    }
    return 0;
}

static void port_stop(int port)
{
    uintptr_t base = (uintptr_t)hba_base + HBA_PORTS + port * 0x80;
    uint32_t cmd = mmio_read32(base + PORT_CMD);
    cmd &= ~PORT_CMD_ST;
    mmio_write32(base + PORT_CMD, cmd);
    
    for (int i = 0; i < 1000000; i++) {
        if (!(mmio_read32(base + PORT_CMD) & PORT_CMD_CR)) break;
        asm volatile("pause");
    }
    
    cmd = mmio_read32(base + PORT_CMD);
    cmd &= ~PORT_CMD_FRE;
    mmio_write32(base + PORT_CMD, cmd);
}

static void port_start(int port)
{
    uintptr_t base = (uintptr_t)hba_base + HBA_PORTS + port * 0x80;
    uint32_t cmd = mmio_read32(base + PORT_CMD);
    cmd |= PORT_CMD_FRE;
    mmio_write32(base + PORT_CMD, cmd);
    cmd |= PORT_CMD_ST;
    mmio_write32(base + PORT_CMD, cmd);
}

static int port_rebase(int port)
{
    uintptr_t base = (uintptr_t)hba_base + HBA_PORTS + port * 0x80;
    
    port_stop(port);
    
    mmio_write32(base + PORT_CLB, (uint32_t)(uintptr_t)cmd_list);
    mmio_write32(base + PORT_FB, (uint32_t)(uintptr_t)&cmd_table);
    
    // Clear command list
    for (int i = 0; i < 32; i++) {
        cmd_list[i].dword0 = 0;
        cmd_list[i].ctba = 0;
    }
    
    // Clear command table
    for (unsigned int i = 0; i < sizeof(HbaCmdTable); i++) {
        ((uint8_t*)&cmd_table)[i] = 0;
    }
    
    port_start(port);
    return 0;
}

static int probe_ports(void)
{
    if (!hba_base) return -1;
    
    uint32_t pi = mmio_read32((uintptr_t)hba_base + HBA_PI);
    print("Ports implemented: 0x");
    ahci_print_int(pi);
    print("\n");
    
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            uintptr_t base = (uintptr_t)hba_base + HBA_PORTS + i * 0x80;
            uint32_t ssts = mmio_read32(base + PORT_SSTS);
            
            if ((ssts & 0xF) == 0x3) {
                print("Drive found on port ");
                ahci_print_int(i);
                print("\n");
                return i;
            }
        }
    }
    return -1;
}

static int ahci_read_sector_dma(uint32_t lba, uint8_t* buffer)
{
    if (active_port == -1) return -1;
    
    uintptr_t base = (uintptr_t)hba_base + HBA_PORTS + active_port * 0x80;
    
    // Setup command header (cfl=5, w=0, prdtl=1)
    cmd_list[0].dword0 = (5 << 0) | (0 << 7) | (1 << 8);
    cmd_list[0].ctba = (uint32_t)(uintptr_t)&cmd_table;
    cmd_list[0].ctbau = 0;
    
    // Setup PRDT
    cmd_table.prdt_entry.dba = (uint32_t)(uintptr_t)buffer;
    cmd_table.prdt_entry.dbau = 0;
    cmd_table.prdt_entry.dbc = 512 - 1;  // Byte count - 1
    
    // Setup command FIS (Host to Device register FIS)
    uint8_t fis[20] = {0};
    fis[0] = 0x27;                    // FIS type
    fis[1] = 0x80;                    // Command
    fis[2] = ATA_CMD_READ_DMA;        // Command
    fis[4] = lba & 0xFF;
    fis[5] = (lba >> 8) & 0xFF;
    fis[6] = (lba >> 16) & 0xFF;
    fis[7] = (lba >> 24) & 0xFF;
    fis[12] = 1;                       // Sector count
    
    // Copy FIS to command table
    for (int i = 0; i < 20; i++) {
        cmd_table.cfis[i] = fis[i];
    }
    
    // Start command
    mmio_write32(base + PORT_CI, 1);
    
    // Wait for completion
    int timeout = 10000000;
    while (timeout-- > 0) {
        if ((mmio_read32(base + PORT_CI) & 1) == 0) {
            return 0;
        }
        asm volatile("pause");
    }
    
    return -1;
}

static int ahci_write_sector_dma(uint32_t lba, const uint8_t* buffer)
{
    if (active_port == -1) return -1;
    
    uintptr_t base = (uintptr_t)hba_base + HBA_PORTS + active_port * 0x80;
    
    // Setup command header (cfl=5, w=1, prdtl=1)
    cmd_list[0].dword0 = (5 << 0) | (1 << 7) | (1 << 8);
    cmd_list[0].ctba = (uint32_t)(uintptr_t)&cmd_table;
    cmd_list[0].ctbau = 0;
    
    // Setup PRDT
    cmd_table.prdt_entry.dba = (uint32_t)(uintptr_t)buffer;
    cmd_table.prdt_entry.dbau = 0;
    cmd_table.prdt_entry.dbc = 512 - 1;
    
    // Setup command FIS
    uint8_t fis[20] = {0};
    fis[0] = 0x27;
    fis[1] = 0x80;
    fis[2] = ATA_CMD_WRITE_DMA;
    fis[4] = lba & 0xFF;
    fis[5] = (lba >> 8) & 0xFF;
    fis[6] = (lba >> 16) & 0xFF;
    fis[7] = (lba >> 24) & 0xFF;
    fis[12] = 1;
    
    for (int i = 0; i < 20; i++) {
        cmd_table.cfis[i] = fis[i];
    }
    
    mmio_write32(base + PORT_CI, 1);
    
    int timeout = 10000000;
    while (timeout-- > 0) {
        if ((mmio_read32(base + PORT_CI) & 1) == 0) {
            return 0;
        }
        asm volatile("pause");
    }
    
    return -1;
}

void ahci_init(void)
{
    print("Initializing AHCI...\n");
    
    if (!find_ahci_controller()) {
        return;
    }
    
    // Enable AHCI
    uint32_t ghc = mmio_read32((uintptr_t)hba_base + HBA_GHC);
    ghc |= (1 << 31);
    mmio_write32((uintptr_t)hba_base + HBA_GHC, ghc);
    
    active_port = probe_ports();
    if (active_port != -1) {
        port_rebase(active_port);
        ahci_present = 1;
        print("AHCI drive ready on port ");
        ahci_print_int(active_port);
        print("\n");
    } else {
        print("No AHCI drives found\n");
    }
}

int ahci_drive_present(void)
{
    return ahci_present;
}

int ahci_read_sector(uint32_t lba, uint8_t* buffer)
{
    if (!ahci_present) return -1;
    return ahci_read_sector_dma(lba, buffer);
}

int ahci_write_sector(uint32_t lba, const uint8_t* buffer)
{
    if (!ahci_present) return -1;
    return ahci_write_sector_dma(lba, buffer);
}