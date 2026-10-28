#include <stddef.h>
#include <stdint.h>
#include "interrupts.h"
#include "memory.h"
#include "fs.h"
#include "paging.h"
#include "pfs.h"
#define W 80
#define H 25
#define MB_MAGIC 0x2BADB002u
#define COM1 0x3F8
static volatile uint16_t *const vga=(volatile uint16_t*)0xB8000;
static size_t row,col,n;
static char line[128];
static inline void outb(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline uint8_t inb(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static void serial_init(void){outb(COM1+1,0);outb(COM1+3,0x80);outb(COM1,3);outb(COM1+1,0);outb(COM1+3,3);outb(COM1+2,0xC7);outb(COM1+4,0x0B);}
static void scroll(void){
    for(size_t y=1;y<H;y++)for(size_t x=0;x<W;x++)vga[(y-1)*W+x]=vga[y*W+x];
    for(size_t x=0;x<W;x++)vga[(H-1)*W+x]=0x0F20;
    row=H-1;
}
static void put(char c){
    if(c=='\n'){col=0;if(++row>=H)scroll();}
    else if(c=='\r'){col=0;}
    else if(c=='\b'){if(col)col--;}
    else {vga[row*W+col]=(uint16_t)c|0x0F00;if(++col>=W){col=0;if(++row>=H)scroll();}}
    for(unsigned i=0;i<100000;i++)if(inb(COM1+5)&0x20){outb(COM1,c);break;}
}
static void puts(const char*s){while(*s)put(*s++);}
static void clear(void){for(size_t i=0;i<W*H;i++)vga[i]=0x0F20;row=col=0;}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static void hex32(uint32_t value){static const char d[]="0123456789ABCDEF";for(int s=28;s>=0;s-=4)put(d[(value>>s)&15u]);}
static void prompt(void){puts("bread> ");}
static size_t file_count(void){return pfs_ready()?pfs_count():fs_count();}
static const char *file_name(size_t i){return pfs_ready()?pfs_name(i):fs_name(i);}
static int file_read(const char *name,char *out,size_t capacity){return pfs_ready()?pfs_read(name,out,capacity):fs_read(name,out,capacity);}
static int file_write(const char *name,const char *data){return pfs_ready()?pfs_write(name,data):fs_write(name,data);}
static int file_remove(const char *name){return pfs_ready()?pfs_remove(name):fs_remove(name);}
static void command(void){
    line[n]=0;
    puts("\n");
    char *arg=line;
    while(*arg==' ')++arg;
    char *end=arg;
    while(*end && *end!=' ')++end;
    char *rest=end;
    if(*end){*end++=0;while(*end==' ')++end;rest=end;}else rest=end;
    if(eq(arg,"help"))puts("Commands: help, about, memory, storage, ls, cat NAME, write NAME TEXT, rm NAME, clear, reboot, halt\n");
    else if(eq(arg,"about"))puts("BreadOS: freestanding 32-bit x86 kernel; Multiboot v1 boot.\n");
    else if(eq(arg,"memory")){puts("Free 4 KiB frames: 0x");hex32(memory_free_frames());puts("\n");}
    else if(eq(arg,"storage")){if(pfs_ready())puts("Persistent BreadFS on ATA primary-master disk.\n");else puts("Volatile RAM filesystem; no usable BreadFS volume.\n");}
    else if(eq(arg,"ls")){for(size_t i=0;i<file_count();++i){const char *name=file_name(i);if(name){puts(name);puts("\n");}}}
    else if(eq(arg,"cat")){char content[FS_DATA_MAX+1u];if(!*rest)puts("Usage: cat NAME\n");else if(file_read(rest,content,sizeof(content))<0)puts("File not found.\n");else {puts(content);if(content[0]){size_t k=0;while(content[k])++k;if(content[k-1]!='\n')puts("\n");}}}
    else if(eq(arg,"write")){char *name=rest;while(*rest && *rest!=' ')++rest;if(*rest){*rest++=0;while(*rest==' ')++rest;if(!*rest)puts("Usage: write NAME TEXT\n");else {int result=fs_write(name,rest);if(result>=0)puts("File written to RAM.\n");else if(result==-2)puts("Error: file content limit is 255 bytes.\n");else if(result==-3)puts("Error: RAM filesystem is full.\n");else puts("Invalid filename. Use letters, numbers, dot, dash, underscore.\n");}}else puts("Usage: write NAME TEXT\n");}
    else if(eq(arg,"rm")){if(!*rest)puts("Usage: rm NAME\n");else if(fs_remove(rest)==0)puts("File removed from RAM.\n");else puts("File not found.\n");}
    else if(eq(arg,"clear"))clear();
    else if(eq(arg,"reboot")){puts("Rebooting...\n");__asm__ volatile("cli");uint8_t s=0x02;while(s&2)s=inb(0x64);outb(0x64,0xFE);for(;;)__asm__ volatile("hlt");}
    else if(eq(arg,"halt")){puts("CPU halted. Reset the VM to continue.\n");__asm__ volatile("cli");for(;;)__asm__ volatile("hlt");}
    else if(*arg)puts("Unknown command. Type help.\n");
    n=0;prompt();
}
static const char *keys[128]={[0x02]="1",[0x03]="2",[0x04]="3",[0x05]="4",[0x06]="5",[0x07]="6",[0x08]="7",[0x09]="8",[0x0A]="9",[0x0B]="0",[0x0C]="-",[0x0D]="=",[0x0E]="\b",[0x10]="q",[0x11]="w",[0x12]="e",[0x13]="r",[0x14]="t",[0x15]="y",[0x16]="u",[0x17]="i",[0x18]="o",[0x19]="p",[0x1A]="[",[0x1B]="]",[0x1C]="\n",[0x1E]="a",[0x1F]="s",[0x20]="d",[0x21]="f",[0x22]="g",[0x23]="h",[0x24]="j",[0x25]="k",[0x26]="l",[0x27]=";",[0x28]="'",[0x2B]="\\",[0x2C]="z",[0x2D]="x",[0x2E]="c",[0x2F]="v",[0x30]="b",[0x31]="n",[0x32]="m",[0x33]=",",[0x34]=".",[0x35]="/",[0x39]=" "};
void kernel_main(uint32_t magic,uint32_t mbi){serial_init();clear();puts("BreadOS kernel starting...\n");if(magic!=MB_MAGIC){puts("ERROR: invalid Multiboot magic; halted.\n");for(;;)__asm__ volatile("cli; hlt");}puts("Boot OK. VGA console and PS/2 keyboard shell ready.\n");memory_init(mbi);puts("Physical frame allocator initialized.\n");paging_init();puts("Paging enabled: first 16 MiB identity-mapped.\n");fs_init();puts("RAM filesystem initialized. Type help for commands.\n");interrupts_init();prompt();for(;;){if(!(inb(0x64)&1)){__asm__ volatile("pause");continue;}uint8_t sc=inb(0x60);if(sc&0x80)continue;if(sc==0x1C){command();continue;}if(sc==0x0E){if(n){n--;put('\b');put(' ');put('\b');}continue;}if(sc<128&&keys[sc]&&keys[sc][0]&&n<sizeof(line)-1){char c=keys[sc][0];line[n++]=c;put(c);}}}
