#include "ata.h"
#define LIMIT 1000000
static inline unsigned char input8(unsigned short p) {
 unsigned char v; __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p)); return v;
}
static inline unsigned short input16(unsigned short p) {
 unsigned short v; __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p)); return v;
}
static inline void output8(unsigned short p,unsigned char v) {
 __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static inline void output16(unsigned short p,unsigned short v) {
 __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));
}
static void delay(void) { for(int i=0;i<4;i++)(void)input8(0x3F6); }
static int wait_ready(int data) {
 for(int i=0;i<LIMIT;i++) {
  unsigned char s=input8(0x1F7);
  if(s==0||s==255)return -1;
  if(s&0x80)continue;
  if(s&0x21)return -1;
  if(!data||(s&8))return 0;
 }
 return -1;
}
static int select_sector(uint32_t lba) {
 if(lba>=0x10000000U)return -1;
 output8(0x3F6,2);
 output8(0x1F6,0xE0|((lba>>24)&15));delay();
 if(wait_ready(0)<0)return -1;
 output8(0x1F1,0);output8(0x1F2,1);
 output8(0x1F3,lba&255);output8(0x1F4,(lba>>8)&255);output8(0x1F5,(lba>>16)&255);
 return 0;
}
int ata_read_sector(uint32_t lba,void *buffer) {
 if(select_sector(lba)<0)return -1;
 output8(0x1F7,0x20);delay();if(wait_ready(1)<0)return -1;
 unsigned char *b=buffer;
 for(int i=0;i<256;i++){unsigned short v=input16(0x1F0);b[i*2]=v&255;b[i*2+1]=v>>8;}
 delay();return wait_ready(0);
}
int ata_write_sector(uint32_t lba,const void *buffer) {
 if(select_sector(lba)<0)return -1;
 output8(0x1F7,0x30);delay();if(wait_ready(1)<0)return -1;
 const unsigned char *b=buffer;
 for(int i=0;i<256;i++)output16(0x1F0,(unsigned short)b[i*2]|((unsigned short)b[i*2+1]<<8));
 delay();return wait_ready(0);
}
int ata_flush(void) {
 output8(0x1F6,0xE0);delay();if(wait_ready(0)<0)return -1;
 output8(0x1F7,0xE7);delay();return wait_ready(0);
}
