#include <stddef.h>
#include <stdint.h>
#include "interrupts.h"
#include "memory.h"
#include "heap.h"
#include "fs.h"
#include "paging.h"
#include "video.h"
#include "pfs.h"
#include "gdt.h"
#include "task.h"
#include "kernel/keyboard.h"
#define W 80
#define H 25
#define MB_MAGIC 0x2BADB002u
#define COM1 0x3F8
extern uint32_t timer_ticks(void);
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
    if(video_available()) video_console_put(c);
    else {
        if(c=='\n'){col=0;if(++row>=H)scroll();}
        else if(c=='\r'){col=0;}
        else if(c=='\b'){if(col)col--;}
        else {vga[row*W+col]=(uint16_t)c|0x0F00;if(++col>=W){col=0;if(++row>=H)scroll();}}
    }
    for(unsigned i=0;i<100000;i++)if(inb(COM1+5)&0x20){outb(COM1,c);break;}
}
static void puts(const char*s){while(*s)put(*s++);}
static void clear(void){for(size_t i=0;i<W*H;i++)vga[i]=0x0F20;row=col=0;if(video_available())video_console_clear(0x101820u,0xE6E6E6u);}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static void hex32(uint32_t value){static const char d[]="0123456789ABCDEF";for(int s=28;s>=0;s-=4)put(d[(value>>s)&15u]);}
static void prompt(void){puts("bread> ");}
static size_t file_count(void){return pfs_ready()?pfs_count():fs_count();}
static const char *file_name(size_t i){return pfs_ready()?pfs_name(i):fs_name(i);}
static int file_read(const char *name,char *out,size_t capacity){return pfs_ready()?pfs_read(name,out,capacity):fs_read(name,out,capacity);}
static int file_write(const char *name,const char *data){return pfs_ready()?pfs_write(name,data):fs_write(name,data);}
static int file_remove(const char *name){return pfs_ready()?pfs_remove(name):fs_remove(name);}
static int file_mkdir(const char *path){return pfs_ready()?pfs_mkdir(path):fs_mkdir(path);}
static int file_rmdir(const char *path){return pfs_ready()?pfs_rmdir(path):fs_rmdir(path);}
static int file_move(const char *source,const char *destination){return pfs_ready()?pfs_move(source,destination):fs_move(source,destination);}
static void command(void){
    line[n]=0;
    puts("\n");
    char *arg=line;
    while(*arg==' ')++arg;
    char *end=arg;
    while(*end && *end!=' ')++end;
    char *rest=end;
    if(*end){*end++=0;while(*end==' ')++end;rest=end;}else rest=end;
    if(eq(arg,"help"))puts("Commands: help, about, video, memory, storage, uptime, tasks, ls, mkdir PATH, rmdir PATH, cat PATH, write PATH TEXT, mv SOURCE DEST, rm PATH, clear, reboot, halt\n");
    else if(eq(arg,"about"))puts("BreadOS: freestanding 32-bit x86 kernel; Multiboot v1 boot.\n");
    else if(eq(arg,"video")){if(video_available()){puts("Framebuffer: ");hex32(video_width());puts(" x ");hex32(video_height());puts(" RGB\n");}else puts("No supported RGB framebuffer; VGA text console remains active.\n");}
    else if(eq(arg,"memory")){puts("Free 4 KiB frames: 0x");hex32(memory_free_frames());puts("\n");}
    else if(eq(arg,"storage")){if(pfs_ready())puts("Persistent BreadFS on ATA primary-master disk.\n");else puts("Volatile RAM filesystem; no usable BreadFS volume.\n");}
    else if(eq(arg,"uptime")){puts("PIT ticks at 100 Hz: ");hex32(timer_ticks());puts("\n");}
    else if(eq(arg,"tasks")){puts("Timer-driven kernel task switches: ");hex32(task_switch_count());puts("\n");}
    else if(eq(arg,"ls")){for(size_t i=0;i<file_count();++i){const char *name=file_name(i);if(name){puts(name);puts("\n");}}}
    else if(eq(arg,"cat")){char content[FS_DATA_MAX+1u];if(!*rest)puts("Usage: cat NAME\n");else if(file_read(rest,content,sizeof(content))<0)puts("File not found.\n");else {puts(content);if(content[0]){size_t k=0;while(content[k])++k;if(content[k-1]!='\n')puts("\n");}}}
    else if(eq(arg,"write")){char *name=rest;while(*rest && *rest!=' ')++rest;if(*rest){*rest++=0;while(*rest==' ')++rest;if(!*rest)puts("Usage: write NAME TEXT\n");else {int result=file_write(name,rest);if(result>=0){if(pfs_ready())puts("File written to disk.\n");else puts("File written to RAM.\n");}else if(result==-2)puts("Error: file content limit is 255 bytes.\n");else if(result==-3)puts("Error: RAM filesystem is full.\n");else puts("Invalid filename. Use letters, numbers, dot, dash, underscore.\n");}}else puts("Usage: write NAME TEXT\n");}
    else if(eq(arg,"mkdir")){if(!*rest)puts("Usage: mkdir PATH\n");else if(file_mkdir(rest)==0)puts("Directory created.\n");else puts("Cannot create directory (invalid path, existing path, missing parent, or full filesystem).\n");}
    else if(eq(arg,"rmdir")){if(!*rest)puts("Usage: rmdir PATH\n");else {int result=file_rmdir(rest);if(result==0)puts("Directory removed.\n");else if(result==-2)puts("Directory is not empty.\n");else puts("Directory not found or storage error.\n");}}
    else if(eq(arg,"mv")){char *source=rest;while(*rest && *rest!=' ')++rest;if(*rest){*rest++=0;while(*rest==' ')++rest;if(!*source||!*rest)puts("Usage: mv SOURCE DEST\n");else if(file_move(source,rest)==0)puts("File moved.\n");else puts("Cannot move file (source missing, destination exists, or invalid path).\n");}else puts("Usage: mv SOURCE DEST\n");}
    else if(eq(arg,"rm")){if(!*rest)puts("Usage: rm PATH\n");else if(file_remove(rest)==0){if(pfs_ready())puts("File removed from disk.\n");else puts("File removed from RAM.\n");}else puts("File not found.\n");}
    else if(eq(arg,"clear"))clear();
    else if(eq(arg,"reboot")){puts("Rebooting...\n");__asm__ volatile("cli");uint8_t s=0x02;while(s&2)s=inb(0x64);outb(0x64,0xFE);for(;;)__asm__ volatile("hlt");}
    else if(eq(arg,"halt")){puts("CPU halted. Reset the VM to continue.\n");__asm__ volatile("cli");for(;;)__asm__ volatile("hlt");}
    else if(*arg)puts("Unknown command. Type help.\n");
    n=0;prompt();
}
void kernel_main(uint32_t magic,uint32_t mbi){
    serial_init();clear();puts("BreadOS kernel starting...\n");
    if(magic!=MB_MAGIC){puts("ERROR: invalid Multiboot magic; halted.\n");for(;;)__asm__ volatile("cli; hlt");}
    puts("Boot OK. VGA console and PS/2 keyboard shell ready.\n");
    gdt_init();
    puts("GDT installed; TSS loaded for future ring-3 execution.\n");
    memory_init(mbi);puts("Physical frame allocator initialized.\n");
    heap_init();puts("Kernel heap initialized (256 KiB).\n");
    paging_init();puts("Paging enabled: first 16 MiB identity-mapped.\n");
    if(video_init(mbi)==0){video_console_clear(0x101820u,0xE6E6E6u);puts("GRUB RGB framebuffer mapped; native graphical text console initialized.\n");}else puts("No supported GRUB RGB framebuffer; VGA text fallback active.\n");
    int storage=pfs_init();if(storage==2)puts("Persistent BreadFS formatted and mounted.\n");else if(storage==1)puts("Persistent BreadFS mounted.\n");else{fs_init();if(storage==0)puts("RAM filesystem initialized (no usable ATA volume).\n");else puts("RAM filesystem initialized (ATA volume not recognized; disk left unchanged).\n");}
    tasking_init();puts("Round-robin kernel scheduler initialized (3 kernel threads).\n");
    puts("Type help for commands.\n");interrupts_init();
    while(task_switch_count()<6u){__asm__ volatile("hlt");}
    puts("Scheduler smoke test passed: round-robin task switching verified.\n");prompt();
    for(;;){char c;if(!keyboard_read_char(&c)){__asm__ volatile("pause");continue;}if(c=='\n'){command();continue;}if(c=='\b'){if(n){n--;put('\b');put(' ');put('\b');}continue;}if(n<sizeof(line)-1){line[n++]=c;put(c);}}
}
