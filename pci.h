#ifndef BREADOS_PCI_H
#define BREADOS_PCI_H
#include <stdint.h>
#include <stddef.h>
#define PCI_MAX_DEVICES 256u
struct pci_device {
    uint8_t bus,device,function;
    uint16_t vendor_id,device_id;
    uint8_t class_code,subclass,programming_interface,revision;
    uint8_t header_type,interrupt_line,interrupt_pin;
    uint32_t bars[6];
};
typedef uint32_t (*pci_config_reader)(uint8_t,uint8_t,uint8_t,uint8_t);
void pci_set_config_reader(pci_config_reader reader);
void pci_init(void);
size_t pci_device_count(void);
const struct pci_device *pci_device_at(size_t index);
const struct pci_device *pci_find_class(uint8_t class_code,uint8_t subclass,size_t start_index);
#endif
