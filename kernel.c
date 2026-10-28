#include <stddef.h>
#include <stdint.h>
#include "interrupts.h"
#include "memory.h"
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
static void put(char c){
    if(c=='\n'){col=0;if(++row>=H)row=0;}
    else if(c=='\r'){col=0;}
    else if(c=='\b'){if(col)col--;}
    else {vga[row*W+col]=(uint16_t)c|0x0F00;if(++col>=W){col=0;if(++row>=H)row=0;}}
    for(unsigned i=0;i<100000;i++)if(inb(COM1+5)&0x20){outb(COM1,c);break;}
}
static void newline(void){put('\r');put('\n');}
static void scroll(void){
    for(size_t y=1;y<H;y++)for(size_t x=0;x<W;x++)vga[(y-1)*W+x]=vga[y*W+x];
    for(size_t x=0;x<W;x++)vga[(H-1)*W+x]=0x0F20;
    row=H-1;
}
static void terminal_put(char c){
    if(c=='\n'){col=0;if(++row>=H)scroll();return;}
    if(c=='\r'){col=0;return;}
    if(c=='\b'){if(col)col--;return;}
    vga[row*W+col]=(uint16_t)c|0x0F00;
    if(++col>=W){col=0;if(++row>=H)scroll();}
}
static void puts(const char*s){while(*s)put(*s++);}
static void clear(void){for(size_t i=0;i<W*H;i++)vga[i]=0x0F20;row=col=0;}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static void hex32(uint32_t value){static const char d[]="0123456789ABCDEF";for(int s=28;s>=0;s-=4)put(d[(value>>s)&15u]);}
static void prompt(void){puts("bread> ");}
static void command(void){line[n]=0;puts("\n");if(eq(line,"help"))puts("Commands: help, about, memory, clear, reboot, halt\n");else if(eq(line,"about"))puts("BreadOS: freestanding 32-bit x86 kernel; Multiboot v1 boot.\n");else if(eq(line,"memory")){puts("Free 4 KiB frames: 0x");hex32(memory_free_frames());puts("\n");}else if(eq(line,"clear"))clear();else if(eq(line,"reboot")){puts("Rebooting...\n");__asm__ volatile("cli");uint8_t s=0x02;while(s&2)s=inb(0x64);outb(0x64,0xFE);for(;;)__asm__ volatile("hlt");}else if(eq(line,"halt")){puts("CPU halted. Reset the VM to continue.\n");__asm__ volatile("cli");for(;;)__asm__ volatile("hlt");}else if(n)puts("Unknown command. Type help.\n");n=0;prompt();}
static const char *keys[128]={[0x02]="1",[0x03]="2",[0x04]="3",[0x05]="4",[0x06]="5",[0x07]="6",[0x08]="7",[0x09]="8",[0x0A]="9",[0x0B]="0",[0x0C]="-",[0x0D]="=",[0x0E]="\b",[0x10]="q",[0x11]="w",[0x12]="e",[0x13]="r",[0x14]="t",[0x15]="y",[0x16]="u",[0x17]="i",[0x18]="o",[0x19]="p",[0x1A]="[",[0x1B]="]",[0x1C]="\n",[0x1E]="a",[0x1F]="s",[0x20]="d",[0x21]="f",[0x22]="g",[0x23]="h",[0x24]="j",[0x25]="k",[0x26]="l",[0x27]=";",[0x28]="'",[0x2B]="\\",[0x2C]="z",[0x2D]="x",[0x2E]="c",[0x2F]="v",[0x30]="b",[0x31]="n",[0x32]="m",[0x33]=",",[0x34]=".",[0x35]="/",[0x39]=" "};
void kernel_main(uint32_t magic,uint32_t mbi){serial_init();clear();puts("BreadOS kernel starting...\n");if(magic!=MB_MAGIC){puts("ERROR: invalid Multiboot magic; halted.\n");for(;;)__asm__ volatile("cli; hlt");}puts("Boot OK. VGA console and PS/2 keyboard shell ready.\n");memory_init(mbi);puts("Physical frame allocator initialized. Type help for commands.\n");interrupts_init();prompt();for(;;){if(!(inb(0x64)&1)){__asm__ volatile("pause");continue;}uint8_t sc=inb(0x60);if(sc&0x80)continue;if(sc==0x1C){command();continue;}if(sc==0x0E){if(n){n--;put('\b');put(' ');put('\b');}continue;}if(sc<128&&keys[sc]&&keys[sc][0]&&n<sizeof(line)-1){char c=keys[sc][0];line[n++]=c;put(c);}}}
