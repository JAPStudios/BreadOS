#include "pci.h"
#include <stdint.h>
#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA 0xCFCu
static struct pci_device devices[PCI_MAX_DEVICES];
static size_t count;
static pci_config_reader test_reader;
static inline void outl(uint16_t port,uint32_t value){__asm__ volatile("outl %0,%1"::"a"(value),"Nd"(port));}
static inline uint32_t inl(uint16_t port){uint32_t value;__asm__ volatile("inl %1,%0":"=a"(value):"Nd"(port));return value;}
static uint32_t config_read(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t offset){
    if(test_reader)return test_reader(bus,dev,fn,(uint8_t)(offset&0xFCu));
    uint32_t address=0x80000000u|((uint32_t)bus<<16)|((uint32_t)dev<<11)|((uint32_t)fn<<8)|(offset&0xFCu);
    outl(PCI_CONFIG_ADDRESS,address);return inl(PCI_CONFIG_DATA);
}
void pci_set_config_reader(pci_config_reader reader){test_reader=reader;}
static void scan_function(uint8_t bus,uint8_t dev,uint8_t fn){
    uint32_t id=config_read(bus,dev,fn,0);
    if((uint16_t)id==0xFFFFu||count>=PCI_MAX_DEVICES)return;
    uint32_t cls=config_read(bus,dev,fn,8),hdr=config_read(bus,dev,fn,12),irq=config_read(bus,dev,fn,0x3C);
    struct pci_device *d=&devices[count++];
    d->bus=bus;d->device=dev;d->function=fn;d->vendor_id=(uint16_t)id;d->device_id=(uint16_t)(id>>16);
    d->revision=(uint8_t)cls;d->programming_interface=(uint8_t)(cls>>8);d->subclass=(uint8_t)(cls>>16);d->class_code=(uint8_t)(cls>>24);
    d->header_type=(uint8_t)(hdr>>16);d->interrupt_line=(uint8_t)irq;d->interrupt_pin=(uint8_t)(irq>>8);
    unsigned bars=(d->header_type&0x7Fu)==0u?6u:((d->header_type&0x7Fu)==1u?2u:1u);
    for(unsigned i=0;i<6u;i++)d->bars[i]=i<bars?config_read(bus,dev,fn,(uint8_t)(0x10u+i*4u)):0u;
}
static uint8_t scanned_buses[256];

static void scan_bus(uint8_t bus) {
    if (scanned_buses[bus] || count >= PCI_MAX_DEVICES) return;
    scanned_buses[bus] = 1;

    for (uint32_t dev = 0; dev < 32u && count < PCI_MAX_DEVICES; ++dev) {
        uint32_t id = config_read(bus, (uint8_t)dev, 0, 0);
        if ((uint16_t)id == 0xFFFFu) continue;

        uint32_t header = config_read(bus, (uint8_t)dev, 0, 12);
        unsigned functions = ((header >> 16) & 0x80u) ? 8u : 1u;
        for (uint32_t fn = 0; fn < functions && count < PCI_MAX_DEVICES; ++fn) {
            size_t before = count;
            scan_function(bus, (uint8_t)dev, (uint8_t)fn);
            if (count == before) continue;

            const struct pci_device *device = &devices[count - 1u];
            if (device->class_code == 0x06u && device->subclass == 0x04u) {
                uint32_t buses = config_read(bus, (uint8_t)dev,
                                             (uint8_t)fn, 0x18u);
                uint8_t secondary = (uint8_t)(buses >> 8);
                if (secondary != 0u && secondary != bus)
                    scan_bus(secondary);
            }
        }
    }
}

void pci_init(void) {
    count = 0;
    for (size_t i = 0; i < sizeof(scanned_buses); ++i)
        scanned_buses[i] = 0;
    scan_bus(0);
}
size_t pci_device_count(void){return count;}
const struct pci_device *pci_device_at(size_t index){return index<count?&devices[index]:0;}
const struct pci_device *pci_find_class(uint8_t cls,uint8_t sub,size_t start){
    for(size_t i=start;i<count;i++)if(devices[i].class_code==cls&&devices[i].subclass==sub)return &devices[i];
    return 0;
}
