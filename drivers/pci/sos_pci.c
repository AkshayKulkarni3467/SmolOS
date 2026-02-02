#include "sos_pci.h"
#include "sos_io.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_string.h"
#include "sos_memory.h"
#include "sos_stddef.h"


static PCIDevice devices[MAX_PCI_DEVICES];
static int device_count = 0;
static int pci_initialized = 0;

static const PCIVendor known_vendors[] = {
    {0x1022, "AMD"}, {0x1002, "AMD/ATI"}, {0x10DE, "NVIDIA"}, {0x8086, "Intel"},
    {0x1234, "QEMU"}, {0x1AF4, "Red Hat (Virtio)"}, {0x15AD, "VMware"}, {0x80EE, "VirtualBox"},
    {0x1106, "VIA Technologies"}, {0x10EC, "Realtek"}, {0x1180, "Ricoh"}, {0x1217, "O2 Micro"},
    {0x14E4, "Broadcom"}, {0x168C, "Qualcomm Atheros"}, {0x197B, "JMicron"}, {0x1B21, "ASMedia"},
    {0x1969, "Qualcomm"}, {0x11AB, "Marvell"}, {0x104C, "Texas Instruments"}, {0x1013, "Cirrus Logic"},
    {0x5333, "S3 Graphics"}, {0x102B, "Matrox"}, {0x1039, "Silicon Integrated Systems"}, {0x10B9, "ALi"},
    {0x1095, "Silicon Image"}, {0x8139, "Realtek (RTL8139)"}, {0, NULL}
};

static const PCIClass known_classes[] = {
    {0x01, 0x00, "SCSI Bus Controller"}, {0x01, 0x01, "IDE Controller"}, {0x01, 0x02, "Floppy Disk Controller"},
    {0x01, 0x03, "IPI Bus Controller"}, {0x01, 0x04, "RAID Controller"}, {0x01, 0x05, "ATA Controller"},
    {0x01, 0x06, "SATA Controller"}, {0x01, 0x07, "SAS Controller"}, {0x01, 0x08, "NVM Controller"},
    {0x01, 0x80, "Mass Storage Controller"},
    {0x02, 0x00, "Ethernet Controller"}, {0x02, 0x01, "Token Ring Controller"}, {0x02, 0x02, "FDDI Controller"},
    {0x02, 0x03, "ATM Controller"}, {0x02, 0x04, "ISDN Controller"}, {0x02, 0x80, "Network Controller"},
    {0x03, 0x00, "VGA Compatible Controller"}, {0x03, 0x01, "XGA Controller"}, {0x03, 0x02, "3D Controller"},
    {0x03, 0x80, "Display Controller"},
    {0x04, 0x00, "Video Device"}, {0x04, 0x01, "Audio Device"}, {0x04, 0x02, "Telephony Device"},
    {0x04, 0x03, "HD Audio Device"}, {0x04, 0x80, "Multimedia Device"},
    {0x05, 0x00, "RAM Controller"}, {0x05, 0x01, "Flash Controller"}, {0x05, 0x80, "Memory Controller"},
    {0x06, 0x00, "Host Bridge"}, {0x06, 0x01, "ISA Bridge"}, {0x06, 0x02, "EISA Bridge"},
    {0x06, 0x03, "MCA Bridge"}, {0x06, 0x04, "PCI-to-PCI Bridge"}, {0x06, 0x05, "PCMCIA Bridge"},
    {0x06, 0x06, "NuBus Bridge"}, {0x06, 0x07, "CardBus Bridge"}, {0x06, 0x08, "RACEway Bridge"},
    {0x06, 0x80, "Bridge Device"},
    {0x07, 0x00, "Serial Controller"}, {0x07, 0x01, "Parallel Port"}, {0x07, 0x02, "Multiport Serial Controller"},
    {0x07, 0x03, "Modem"}, {0x07, 0x80, "Communication Controller"},
    {0x08, 0x00, "PIC"}, {0x08, 0x01, "DMA Controller"}, {0x08, 0x02, "Timer"}, {0x08, 0x03, "RTC"},
    {0x08, 0x04, "PCI Hot-Plug Controller"}, {0x08, 0x05, "SD Host Controller"}, {0x08, 0x80, "System Peripheral"},
    {0x09, 0x00, "Keyboard Controller"}, {0x09, 0x01, "Digitizer"}, {0x09, 0x02, "Mouse Controller"},
    {0x09, 0x03, "Scanner Controller"}, {0x09, 0x04, "Gameport Controller"}, {0x09, 0x80, "Input Device"},
    {0x0A, 0x00, "Generic Docking Station"}, {0x0A, 0x80, "Docking Station"},
    {0x0B, 0x00, "386 Processor"}, {0x0B, 0x01, "486 Processor"}, {0x0B, 0x02, "Pentium Processor"},
    {0x0B, 0x10, "Alpha Processor"}, {0x0B, 0x20, "PowerPC Processor"}, {0x0B, 0x30, "MIPS Processor"},
    {0x0B, 0x40, "Co-Processor"},
    {0x0C, 0x00, "FireWire (IEEE 1394)"}, {0x0C, 0x01, "ACCESS Bus"}, {0x0C, 0x02, "SSA"},
    {0x0C, 0x03, "USB Controller"}, {0x0C, 0x04, "Fibre Channel"}, {0x0C, 0x05, "SMBus"},
    {0x0C, 0x06, "InfiniBand"}, {0x0C, 0x07, "IPMI Interface"}, {0x0C, 0x08, "SERCOS Interface"},
    {0x0C, 0x09, "CANbus"},
    {0xFF, 0xFF, "Unknown Device"}
};


uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (uint32_t)(
        (1U << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC)
    );
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)(
        (1U << 31) |
        ((uint32_t)bus << 16) |
        ((uint32_t)device << 11) |
        ((uint32_t)function << 8) |
        (offset & 0xFC)
    );
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t data = pci_config_read(bus, device, function, offset & 0xFC);
    return (uint16_t)((data >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t data = pci_config_read(bus, device, function, offset & 0xFC);
    return (uint8_t)((data >> ((offset & 3) * 8)) & 0xFF);
}


static void pci_check_function(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_config_read_word(bus, device, function, PCI_VENDOR_ID);
    if (vendor_id == 0xFFFF) return;
    if (device_count >= MAX_PCI_DEVICES) return;
    
    PCIDevice* dev = &devices[device_count++];
    dev->bus = bus;
    dev->device = device;
    dev->function = function;
    dev->vendor_id = vendor_id;
    dev->device_id = pci_config_read_word(bus, device, function, PCI_DEVICE_ID);
    dev->class_code = pci_config_read_byte(bus, device, function, PCI_CLASS);
    dev->subclass = pci_config_read_byte(bus, device, function, PCI_SUBCLASS);
    dev->prog_if = pci_config_read_byte(bus, device, function, PCI_PROG_IF);
    dev->revision = pci_config_read_byte(bus, device, function, PCI_REVISION_ID);
    dev->header_type = pci_config_read_byte(bus, device, function, PCI_HEADER_TYPE);
    dev->subsystem_vendor = pci_config_read_word(bus, device, function, PCI_SUBSYSTEM_VENDOR_ID);
    dev->subsystem_id = pci_config_read_word(bus, device, function, PCI_SUBSYSTEM_ID);
    dev->interrupt_line = pci_config_read_byte(bus, device, function, PCI_INTERRUPT_LINE);
    dev->interrupt_pin = pci_config_read_byte(bus, device, function, PCI_INTERRUPT_PIN);
    
    for (int i = 0; i < 6; i++) {
        dev->bar[i] = pci_config_read(bus, device, function, PCI_BAR0 + (i * 4));
    }
}

static void pci_check_device(uint8_t bus, uint8_t device) {
    uint16_t vendor_id = pci_config_read_word(bus, device, 0, PCI_VENDOR_ID);
    if (vendor_id == 0xFFFF) return;
    pci_check_function(bus, device, 0);
    uint8_t header_type = pci_config_read_byte(bus, device, 0, PCI_HEADER_TYPE);
    if (header_type & 0x80) {
        for (uint8_t function = 1; function < 8; function++) {
            vendor_id = pci_config_read_word(bus, device, function, PCI_VENDOR_ID);
            if (vendor_id != 0xFFFF) pci_check_function(bus, device, function);
        }
    }
}

void pci_scan_bus(void) {
    device_count = 0;
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check_device(bus, device);
        }
    }
}

void pci_init(void) {
    if (pci_initialized) return;
    pci_scan_bus();
    pci_initialized = 1;
}


int pci_get_device_count(void) { return device_count; }

PCIDevice* pci_get_device(int index) {
    if (index < 0 || index >= device_count) return NULL;
    return &devices[index];
}

PCIDevice* pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < device_count; i++) {
        if (devices[i].vendor_id == vendor_id && devices[i].device_id == device_id)
            return &devices[i];
    }
    return NULL;
}

PCIDevice* pci_find_class(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; i < device_count; i++) {
        if (devices[i].class_code == class_code && 
           (subclass == 0xFF || devices[i].subclass == subclass))
            return &devices[i];
    }
    return NULL;
}


const char* pci_get_vendor_name(uint16_t vendor_id) {
    for (int i = 0; known_vendors[i].name != NULL; i++) {
        if (known_vendors[i].id == vendor_id) return known_vendors[i].name;
    }
    return "Unknown Vendor";
}

const char* pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Device";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Communication Controller";
        case 0x08: return "System Peripheral";
        case 0x09: return "Input Device";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent I/O Controller";
        case 0x0F: return "Satellite Communication Controller";
        case 0x10: return "Encryption/Decryption Controller";
        case 0x11: return "Data Acquisition Controller";
        default: return "Unknown Class";
    }
}

const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; known_classes[i].class_code != 0xFF; i++) {
        if (known_classes[i].class_code == class_code && known_classes[i].subclass == subclass)
            return known_classes[i].description;
    }
    return "Unknown Subclass";
}

void pci_get_device_info(PCIDevice* dev, char* buffer) {
    const char* vendor = pci_get_vendor_name(dev->vendor_id);
    const char* subclass = pci_get_subclass_name(dev->class_code, dev->subclass);
    strcpy(buffer, vendor);
    strcat(buffer, " - ");
    strcat(buffer, subclass);
}


void pci_list_devices(void) {
    vga_set_auto_swap(0);
    vga_clear();
    
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("PCI DEVICE ENUMERATION", 0);
    
    char count_str[32];
    strcpy(count_str, "Found ");
    char num[16]; int pos = 0; int temp = device_count;
    if (temp == 0) num[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp > 0) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) num[pos++] = d[i];
    }
    num[pos] = 0;
    strcat(count_str, num);
    strcat(count_str, " PCI devices");
    
    vga_set_color(VGA_LCYAN, VGA_BLUE);
    vga_print_centered(count_str, 1);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(1, 3, '#');
    for (int i = 0; i < 4; i++) vga_putchr_at(4 + i, 3, "Bus "[i]);
    for (int i = 0; i < 6; i++) vga_putchr_at(10 + i, 3, "Vendor"[i]);
    for (int i = 0; i < 6; i++) vga_putchr_at(20 + i, 3, "Device"[i]);
    for (int i = 0; i < 11; i++) vga_putchr_at(30 + i, 3, "Description"[i]);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    for (int i = 0; i < 80; i++) vga_putchr_at(i, 4, 0xC4);

    int row = 5;
    for (int i = 0; i < device_count && row < 23; i++) {
        PCIDevice* dev = &devices[i];
        
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        if (i < 9) vga_putchr_at(1, row, '0' + (i + 1));
        else {
            vga_putchr_at(1, row, '0' + ((i + 1) / 10));
            vga_putchr_at(2, row, '0' + ((i + 1) % 10));
        }
        
        char bus_str[16]; pos = 0;
        bus_str[pos++] = '0' + (dev->bus / 100);
        bus_str[pos++] = '0' + ((dev->bus / 10) % 10);
        bus_str[pos++] = '0' + (dev->bus % 10);
        bus_str[pos++] = ':';
        bus_str[pos++] = '0' + (dev->device / 10);
        bus_str[pos++] = '0' + (dev->device % 10);
        bus_str[pos++] = '.';
        bus_str[pos++] = '0' + dev->function;
        bus_str[pos] = 0;
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        for (int j = 0; bus_str[j]; j++) vga_putchr_at(4 + j, row, bus_str[j]);
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        char hex[5];
        for (int j = 3; j >= 0; j--) {
            int nibble = (dev->vendor_id >> (j * 4)) & 0xF;
            hex[3 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
        }
        hex[4] = 0;
        for (int j = 0; j < 4; j++) vga_putchr_at(10 + j, row, hex[j]);
        
        for (int j = 3; j >= 0; j--) {
            int nibble = (dev->device_id >> (j * 4)) & 0xF;
            hex[3 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
        }
        for (int j = 0; j < 4; j++) vga_putchr_at(20 + j, row, hex[j]);
        
        const char* vendor_name = pci_get_vendor_name(dev->vendor_id);
        const char* subclass_name = pci_get_subclass_name(dev->class_code, dev->subclass);
        
        vga_set_color(VGA_LGREY, VGA_BLCK);
        int x = 30;
        for (int j = 0; vendor_name[j] && x < 50; j++) vga_putchr_at(x++, row, vendor_name[j]);
        if (x < 50) vga_putchr_at(x++, row, ' ');
        if (x < 50) vga_putchr_at(x++, row, '-');
        if (x < 50) vga_putchr_at(x++, row, ' ');
        for (int j = 0; subclass_name[j] && x < 79; j++) vga_putchr_at(x++, row, subclass_name[j]);
        
        row++;
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* footer = "Press any key to continue...";
    for (int i = 0; footer[i]; i++) vga_putchr_at(i, 24, footer[i]);
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void pci_show_device_details(int index) {
    if (index < 0 || index >= device_count) return;
    
    PCIDevice* dev = &devices[index];
    
    vga_set_auto_swap(0);
    vga_clear();
    
    vga_fill_rect(0, 0, 80, 3, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("PCI DEVICE DETAILS", 1);
    
    vga_draw_box_double(5, 4, 70, 17, VGA_CYAN, VGA_BLCK);
    
    int row = 6;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* loc_label = "Location: ";
    for (int i = 0; loc_label[i]; i++) vga_putchr_at(8 + i, row, loc_label[i]);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    char bus_str[32]; int pos = 0;
    bus_str[pos++] = 'B'; bus_str[pos++] = 'u'; bus_str[pos++] = 's'; bus_str[pos++] = ' ';
    bus_str[pos++] = '0' + (dev->bus / 100); bus_str[pos++] = '0' + ((dev->bus / 10) % 10); bus_str[pos++] = '0' + (dev->bus % 10);
    bus_str[pos++] = ','; bus_str[pos++] = ' ';
    bus_str[pos++] = 'D'; bus_str[pos++] = 'e'; bus_str[pos++] = 'v'; bus_str[pos++] = ' ';
    bus_str[pos++] = '0' + (dev->device / 10); bus_str[pos++] = '0' + (dev->device % 10);
    bus_str[pos++] = ','; bus_str[pos++] = ' ';
    bus_str[pos++] = 'F'; bus_str[pos++] = 'n'; bus_str[pos++] = ' ';
    bus_str[pos++] = '0' + dev->function; bus_str[pos] = 0;
    
    for (int i = 0; bus_str[i]; i++) vga_putchr_at(18 + i, row, bus_str[i]);
    row += 2;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* vendor_label = "Vendor: ";
    for (int i = 0; vendor_label[i]; i++) vga_putchr_at(8 + i, row, vendor_label[i]);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* vendor_name = pci_get_vendor_name(dev->vendor_id);
    for (int i = 0; vendor_name[i]; i++) vga_putchr_at(16 + i, row, vendor_name[i]);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    int vname_len = 0; while(vendor_name[vname_len]) vname_len++;
    
    vga_putchr_at(16 + vname_len + 1, row, '(');
    vga_putchr_at(16 + vname_len + 2, row, '0');
    vga_putchr_at(16 + vname_len + 3, row, 'x');
    
    char hex[9];
    for (int j = 3; j >= 0; j--) {
        int nibble = (dev->vendor_id >> (j * 4)) & 0xF;
        hex[3 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
    }
    for (int i = 0; i < 4; i++) vga_putchr_at(16 + vname_len + 4 + i, row, hex[i]);
    vga_putchr_at(16 + vname_len + 8, row, ')');
    row++;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* device_label = "Device: ";
    for (int i = 0; device_label[i]; i++) vga_putchr_at(8 + i, row, device_label[i]);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_putchr_at(16, row, '0'); vga_putchr_at(17, row, 'x');
    
    for (int j = 3; j >= 0; j--) {
        int nibble = (dev->device_id >> (j * 4)) & 0xF;
        hex[3 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
    }
    for (int i = 0; i < 4; i++) vga_putchr_at(18 + i, row, hex[i]);
    row += 2;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* class_label = "Class: ";
    for (int i = 0; class_label[i]; i++) vga_putchr_at(8 + i, row, class_label[i]);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* class_name = pci_get_class_name(dev->class_code);
    for (int i = 0; class_name[i]; i++) vga_putchr_at(15 + i, row, class_name[i]);
    row++;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* subclass_label = "Type: ";
    for (int i = 0; subclass_label[i]; i++) vga_putchr_at(8 + i, row, subclass_label[i]);
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* subclass_name = pci_get_subclass_name(dev->class_code, dev->subclass);
    for (int i = 0; subclass_name[i] && i < 50; i++) vga_putchr_at(14 + i, row, subclass_name[i]);
    row += 2;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* bar_label = "Base Address Registers:";
    for (int i = 0; bar_label[i]; i++) vga_putchr_at(5, row, bar_label[i]);
    row++;

    int bars_found = 0;
    for (int i = 0; i < 6; i++) {
        if (dev->bar[i] == 0) continue;
        bars_found++;
        
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        vga_putchr_at(8, row, 'B'); vga_putchr_at(9, row, 'A'); vga_putchr_at(10, row, 'R');
        vga_putchr_at(11, row, '0' + i); vga_putchr_at(12, row, ':');
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_putchr_at(15, row, '0'); vga_putchr_at(16, row, 'x');
        
        uint32_t val = dev->bar[i];
        for (int j = 7; j >= 0; j--) {
            int nibble = (val >> (j * 4)) & 0xF;
            hex[7 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
        }
        for (int k = 0; k < 8; k++) vga_putchr_at(17 + k, row, hex[k]);

        vga_set_color(VGA_DGREY, VGA_BLCK);
        const char* type = (val & 0x01) ? " (I/O Port)" : " (Memory)";
        for(int k=0; type[k]; k++) vga_putchr_at(26+k, row, type[k]);
        row++;
    }

    if (bars_found == 0) {
        vga_set_color(VGA_DGREY, VGA_BLCK);
        const char* none = "No BARs assigned";
        for(int k=0; none[k]; k++) vga_putchr_at(8+k, row, none[k]);
        row++;
    }
    row++;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* int_label = "Interrupts:";
    for(int k=0; int_label[k]; k++) vga_putchr_at(5, row, int_label[k]);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    char int_str[32]; pos = 0;
    const char* line_lbl = " Line ";
    for(int k=0; line_lbl[k]; k++) int_str[pos++] = line_lbl[k];
    
    if (dev->interrupt_line == 0xFF) int_str[pos++] = '-';
    else {
        if (dev->interrupt_line >= 10) int_str[pos++] = '0' + (dev->interrupt_line / 10);
        int_str[pos++] = '0' + (dev->interrupt_line % 10);
    }

    const char* pin_lbl = ", Pin ";
    for(int k=0; pin_lbl[k]; k++) int_str[pos++] = pin_lbl[k];
    
    if (dev->interrupt_pin == 0) int_str[pos++] = '-';
    else int_str[pos++] = 'A' + (dev->interrupt_pin - 1);
    
    int_str[pos] = 0;
    for(int k=0; int_str[k]; k++) vga_putchr_at(16+k, row, int_str[k]);

    vga_set_color(VGA_WHITE, VGA_BLUE);
    vga_fill_rect(0, 24, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
    vga_print_centered("Press ESC to return", 24);

    vga_set_auto_swap(1);
    vga_swap_buffers();

    clear_event_buffer();
    
    while (1) {
        KeyEvent k = wait_for_key_event();
        if (k.key_code == KEY_ESCAPE || k.character == 27) { 
            break;
        }
    }
}

void pci_interactive_browser(void) {
    int selected = 0;
    int offset = 0;
    int max_rows = 18; 
    int running = 1;
    
    clear_event_buffer();

    while (running) {
        vga_set_auto_swap(0);
        vga_clear();

        vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_MAGENTA);
        vga_set_color(VGA_WHITE, VGA_MAGENTA);
        vga_print_centered("PCI DEVICE BROWSER", 0);
        vga_print_centered("UP/DOWN: Navigate | ENTER: Details | ESC: Exit", 1);

        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_draw_line_horizontal(0, 2, 80, 0xC4, VGA_DGREY, VGA_BLCK);
        
        vga_putchr_at(2, 3, "Bus/Dev");
        vga_putchr_at(12, 3, "Vendor");
        vga_putchr_at(22, 3, "Device");
        vga_putchr_at(32, 3, "Class/Type");

        int y = 4;
        for (int i = 0; i < max_rows; i++) {
            int dev_idx = offset + i;
            if (dev_idx >= device_count) break;

            PCIDevice* dev = &devices[dev_idx];
            
            uint8_t fg = VGA_LGREY;
            uint8_t bg = VGA_BLCK;

            if (dev_idx == selected) {
                fg = VGA_BLCK;
                bg = VGA_LCYAN;
                vga_fill_rect(0, y, 80, 1, ' ', fg, bg);
            }

            vga_set_color(fg, bg);

            char buf[32];
            int b = dev->bus; int d = dev->device; int f = dev->function;
            char tmp[10]; int ti=0;
            tmp[ti++] = (b/100)?'0'+b/100:' '; 
            tmp[ti++] = '0'+(b/10)%10;
            tmp[ti++] = '0'+b%10;
            tmp[ti++] = ':';
            tmp[ti++] = '0'+d/10;
            tmp[ti++] = '0'+d%10;
            tmp[ti++] = '.';
            tmp[ti++] = '0'+f;
            tmp[ti] = 0;
            
            for(int k=0; tmp[k]; k++) vga_putchr_at(1+k, y, tmp[k]);

            char hex[5];
            for (int j = 3; j >= 0; j--) {
                int nibble = (dev->vendor_id >> (j * 4)) & 0xF;
                hex[3 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
            }
            hex[4] = 0;
            for(int k=0; k<4; k++) vga_putchr_at(12+k, y, hex[k]);

            for (int j = 3; j >= 0; j--) {
                int nibble = (dev->device_id >> (j * 4)) & 0xF;
                hex[3 - j] = (nibble < 10) ? '0' + nibble : 'A' + nibble - 10;
            }
            for(int k=0; k<4; k++) vga_putchr_at(22+k, y, hex[k]);

            const char* cls = pci_get_subclass_name(dev->class_code, dev->subclass);
            for(int k=0; cls[k] && k < 45; k++) vga_putchr_at(32+k, y, cls[k]);

            y++;
        }

        if (device_count > max_rows) {
            int bar_height = 18;
            int sb_y = 4 + ((offset * bar_height) / device_count);
            int sb_h = (max_rows * bar_height) / device_count;
            if (sb_h < 1) sb_h = 1;
            vga_draw_line_vertical(79, 4, bar_height, 0xB0, VGA_DGREY, VGA_BLCK);
            vga_draw_line_vertical(79, sb_y, sb_h, 0xDB, VGA_WHITE, VGA_BLCK);
        }

        vga_set_auto_swap(1);
        vga_swap_buffers();

        KeyEvent k = wait_for_key_event();
        if (k.key_code == KEY_ESCAPE || k.character == 27) {
            running = 0;
        } else if (k.key_code == KEY_ARROW_DOWN) {
            if (selected < device_count - 1) {
                selected++;
                if (selected >= offset + max_rows) offset++;
            }
        } else if (k.key_code == KEY_ARROW_UP) {
            if (selected > 0) {
                selected--;
                if (selected < offset) offset--;
            }
        } 
        else if (k.character == '\n' || k.key_code == KEY_ENTER) {
            pci_show_device_details(selected);
        }
    }
    vga_clear();
}

void cmd_lspci(void) {
    if (device_count == 0) {
        vga_println("No PCI devices found or PCI not initialized.");
        return;
    }
    
    pci_list_devices();
    clear_event_buffer();
    wait_for_key_event();
    vga_clear();
}

void cmd_pciinfo(void) {
    if (device_count == 0) {
        vga_println("No PCI devices found. Run 'pci scan' first?");
        return;
    }
    pci_interactive_browser();
}