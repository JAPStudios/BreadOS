#include <assert.h>
#include <stdint.h>
#include "pci.h"
static uint32_t mock(uint8_t bus,uint8_t dev,uint8_t fn,uint8_t off){
 if(bus)return 0xFFFFFFFFu;
 if(dev==0&&fn==0){if(off==0)return 0x12348086u;if(off==8)return 0x06000001u;if(off==12)return 0;if(off==0x3C)return 0x100u;return 0;}
 if(dev==1&&fn==0){if(off==0)return 0x100E8086u;if(off==8)return 0x02000002u;if(off==12)return 0x00800000u;if(off==0x3C)return 0x10Bu;if(off==0x10)return 0xFEBF0000u;return 0;}
 if(dev==1&&fn==1){if(off==0)return 0x29228086u;if(off==8)return 0x01060101u;return 0;}
 return 0xFFFFFFFFu;
}
int main(void){
 pci_set_config_reader(mock);pci_init();assert(pci_device_count()==3u);
 const struct pci_device *h=pci_device_at(0);assert(h&&h->vendor_id==0x8086u&&h->device_id==0x1234u&&h->class_code==6u);
 const struct pci_device *n=pci_find_class(2u,0u,0);assert(n&&n->device==1u&&n->bars[0]==0xFEBF0000u);
 const struct pci_device *s=pci_find_class(1u,6u,0);assert(s&&s->function==1u);
 assert(pci_device_at(3)==0);pci_set_config_reader(0);return 0;
}
