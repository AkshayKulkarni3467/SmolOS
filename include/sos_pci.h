#ifndef INCLUDE_SMOLOS_PCI_H
#define INCLUDE_SMOLOS_PCI_H

#include "sos_stdint.h"


#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_VENDOR_ID      0x00
#define PCI_DEVICE_ID      0x02
#define PCI_COMMAND        0x04
#define PCI_STATUS         0x06
#define PCI_REVISION_ID    0x08
#define PCI_PROG_IF        0x09
#define PCI_SUBCLASS       0x0A
#define PCI_CLASS          0x0B
#define PCI_CACHE_LINE     0x0C
#define PCI_LATENCY_TIMER  0x0D
#define PCI_HEADER_TYPE    0x0E
#define PCI_BIST           0x0F
#define PCI_BAR0           0x10
#define PCI_BAR1           0x14
#define PCI_BAR2           0x18
#define PCI_BAR3           0x1C
#define PCI_BAR4           0x20
#define PCI_BAR5           0x24
#define PCI_CARDBUS_CIS    0x28
#define PCI_SUBSYSTEM_VENDOR_ID 0x2C
#define PCI_SUBSYSTEM_ID   0x2E
#define PCI_EXPANSION_ROM  0x30
#define PCI_CAPABILITIES   0x34
#define PCI_INTERRUPT_LINE 0x3C
#define PCI_INTERRUPT_PIN  0x3D
#define PCI_MIN_GRANT      0x3E
#define PCI_MAX_LATENCY    0x3F


#define PCI_CLASS_UNCLASSIFIED     0x00
#define PCI_CLASS_STORAGE          0x01
#define PCI_CLASS_NETWORK          0x02
#define PCI_CLASS_DISPLAY          0x03
#define PCI_CLASS_MULTIMEDIA       0x04
#define PCI_CLASS_MEMORY           0x05
#define PCI_CLASS_BRIDGE           0x06
#define PCI_CLASS_SIMPLE_COMM      0x07
#define PCI_CLASS_BASE_SYSTEM      0x08
#define PCI_CLASS_INPUT            0x09
#define PCI_CLASS_DOCKING          0x0A
#define PCI_CLASS_PROCESSOR        0x0B
#define PCI_CLASS_SERIAL_BUS       0x0C
#define PCI_CLASS_WIRELESS         0x0D
#define PCI_CLASS_INTELLIGENT      0x0E
#define PCI_CLASS_SATELLITE        0x0F
#define PCI_CLASS_ENCRYPTION       0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11
#define PCI_CLASS_UNKNOWN          0xFF


#define MAX_PCI_DEVICES    64


typedef struct {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
    uint8_t  header_type;
    uint16_t subsystem_vendor;
    uint16_t subsystem_id;
    uint32_t bar[6];
    uint8_t  interrupt_line;
    uint8_t  interrupt_pin;
} PCIDevice;


typedef struct {
    uint16_t id;
    const char* name;
} PCIVendor;


typedef struct {
    uint8_t class_code;
    uint8_t subclass;
    const char* description;
} PCIClass;


void pci_init(void);
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);


void pci_scan_bus(void);
int pci_get_device_count(void);
PCIDevice* pci_get_device(int index);
PCIDevice* pci_find_device(uint16_t vendor_id, uint16_t device_id);
PCIDevice* pci_find_class(uint8_t class_code, uint8_t subclass);


const char* pci_get_vendor_name(uint16_t vendor_id);
const char* pci_get_class_name(uint8_t class_code);
const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass);
void pci_get_device_info(PCIDevice* dev, char* buffer);


void pci_list_devices(void);
void pci_show_device_details(int index);
void pci_interactive_browser(void);


void cmd_lspci(void);
void cmd_pciinfo(void);

#endif // INCLUDE_SMOLOS_PCI_H