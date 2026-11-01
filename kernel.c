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
#include "kernel/mouse.h"
#include "process.h"
#include "console.h"
#include "theme.h"
#include "menu.h"
#include "desktop.h"
#include "notification.h"
#include "clipboard.h"
#include "reminder.h"
#define W 80
#define H 25
#define MB_MAGIC 0x2BADB002u
#define COM1 0x3F8
extern uint32_t timer_ticks(void);
static volatile uint16_t *const vga=(volatile uint16_t*)0xB8000;
static size_t row,col,n;
static char line[128];
#define HISTORY_CAPACITY 16u
static char command_history[HISTORY_CAPACITY][128];
static uint32_t history_head,history_count;
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
        else {const struct bread_theme *t=theme_current();uint32_t light=((t->background>>16&255u)+(t->background>>8&255u)+(t->background&255u))/3u;uint16_t attr=light>128u?0x7000u:0x0F00u;vga[row*W+col]=(uint16_t)c|attr;if(++col>=W){col=0;if(++row>=H)scroll();}}
    }
    for(unsigned i=0;i<100000;i++)if(inb(COM1+5)&0x20){outb(COM1,c);break;}
}
static void puts(const char*s){while(*s)put(*s++);}
void console_put_char(char c){put(c);}
static void clear(void){const struct bread_theme *t=theme_current();uint32_t light=((t->background>>16&255u)+(t->background>>8&255u)+(t->background&255u))/3u;uint16_t attr=light>128u?0x7000u:0x0F00u;for(size_t i=0;i<W*H;i++)vga[i]=(uint16_t)' '|attr;row=col=0;if(video_available())video_console_clear(t->background,t->foreground);}
static int eq(const char*a,const char*b){while(*a&&*b&&*a==*b){a++;b++;}return *a==*b;}
static int parse_u32(const char *text,uint32_t *out) {
    if(!text || !*text || !out) return 0;
    uint32_t value=0;
    while(*text) {
        if(*text<'0' || *text>'9') return 0;
        uint32_t digit=(uint32_t)(*text-'0');
        if(value>(0xFFFFFFFFu-digit)/10u) return 0;
        value=value*10u+digit;
        text++;
    }
    *out=value;
    return 1;
}
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
size_t bread_file_count(void){return file_count();}
const char *bread_file_name(size_t index){return file_name(index);}
int bread_file_read(const char *name,char *out,size_t capacity){return file_read(name,out,capacity);}
static void command(void){
    line[n]=0;
    if(n){uint32_t slot=history_head;for(size_t i=0;i<n && i<sizeof(command_history[slot])-1u;i++)command_history[slot][i]=line[i];command_history[slot][n<sizeof(command_history[slot])?n:sizeof(command_history[slot])-1u]=0;history_head=(history_head+1u)%HISTORY_CAPACITY;if(history_count<HISTORY_CAPACITY)history_count++;}
    puts("\n");
    char *arg=line;
    while(*arg==' ')++arg;
    char *end=arg;
    while(*end && *end!=' ')++end;
    char *rest=end;
    if(*end){*end++=0;while(*end==' ')++end;rest=end;}else rest=end;
    if(eq(arg,"help"))puts("Commands: help, menu [SEARCH], history, theme [NAME], desktop, notify TEXT, notifications, snooze INDEX TICKS, dismiss INDEX, dnd [on|off], clipboard, remind TICKS TEXT, reminders, about, video, userdemo, memory, storage, uptime, tasks, ls, mkdir PATH, rmdir PATH, cat PATH, write PATH TEXT, mv SOURCE DEST, rm PATH, clear, reboot, halt\n");
    else if(eq(arg,"menu")){size_t shown=0;puts("BreadOS command palette\n");for(size_t i=0;i<shell_menu_count();i++){const struct shell_menu_entry *entry=shell_menu_at(i);if(shell_menu_matches(entry,rest)){puts("[");puts(entry->category);puts("] ");puts(entry->command);puts(" - ");puts(entry->description);puts("\n");shown++;}}if(!shown)puts("No matching commands.\n");}
    else if(eq(arg,"history")){uint32_t start=(history_head+HISTORY_CAPACITY-history_count)%HISTORY_CAPACITY;for(uint32_t i=0;i<history_count;i++){uint32_t slot=(start+i)%HISTORY_CAPACITY;hex32(i+1u);puts("  ");puts(command_history[slot]);puts("\n");}}
    else if(eq(arg,"theme")){if(!*rest){puts("Current theme: ");puts(theme_current()->name);puts("\nAvailable themes: ");for(size_t i=0;i<theme_count();i++){if(i)puts(", ");puts(theme_at(i)->name);}puts("\n");}else if(theme_apply(rest)==0){clear();if(file_write("theme",theme_current()->name)>=0){puts("Theme applied and saved: ");puts(theme_current()->name);puts("\n");}else puts("Theme applied, but could not be saved to storage.\n");}else puts("Unknown theme. Run theme to list available themes.\n");}
    else if(eq(arg,"desktop")){int result=desktop_run();clear();if(result==0)puts("Returned to BreadOS shell.\n");else puts("Graphical desktop unavailable; an RGB framebuffer of at least 320x240 is required.\n");}
    else if(eq(arg,"clipboard")) {
        char *mode=rest;
        while(*mode==' ')mode++;
        char *payload=mode;
        while(*payload && *payload!=' ')payload++;
        if(*payload){*payload++=0;while(*payload==' ')payload++;}
        if(eq(mode,"copy")) {
            if(!*payload) puts("Usage: clipboard copy TEXT\n");
            else {clipboard_set(payload,0);puts("Copied to BreadOS clipboard.\n");}
        } else if(eq(mode,"sensitive")) {
            if(!*payload) puts("Usage: clipboard sensitive TEXT\n");
            else {clipboard_set(payload,1);puts("Copied; sensitive text excluded from history.\n");}
        } else if(eq(mode,"paste")) {
            puts(clipboard_current());puts("\n");
        } else if(eq(mode,"history")) {
            size_t count=clipboard_history_count();
            if(!count) puts("Clipboard history is empty.\n");
            for(size_t i=0;i<count;i++){const char *entry=clipboard_history_at(i);if(entry){hex32(i);puts("  ");puts(entry);puts("\n");}}
        } else if(eq(mode,"clear")) {
            clipboard_clear();puts("Clipboard and history cleared.\n");
        } else puts("Usage: clipboard copy|sensitive|paste|history|clear [TEXT]\n");
    }
    else if(eq(arg,"remind")) {
        if(eq(rest,"clear")) {reminder_clear();puts("All reminders cleared.\n");}
        else {
            char *p=rest;while(*p==' ')p++;
            char *q=p;while(*q>='0' && *q<='9')q++;
            if(*q){*q++=0;while(*q==' ')q++;}
            uint32_t delay=0;
            if(!*p || !*q || !parse_u32(p,&delay) || reminder_add(timer_ticks(),delay,q)<0)
                puts("Usage: remind TICKS TEXT (100 ticks = 1 second), or remind clear.\n");
            else puts("Reminder scheduled.\n");
        }
    }
    else if(eq(arg,"reminders")) {
        size_t count=reminder_count();
        if(!count) puts("No pending reminders.\n");
        for(size_t i=0;i<count;i++) {
            const struct bread_reminder *entry=reminder_at(i);
            if(entry){hex32(i);puts("  due tick ");hex32(entry->due_tick);puts(": ");puts(entry->text);puts("\n");}
        }
    }
    else if(eq(arg,"notify")){if(!*rest)puts("Usage: notify TEXT\n");else if(notification_push("BreadOS","Notice",rest,timer_ticks()))puts("Notification stored; Do Not Disturb is suppressing alerts.\n");else puts("Notification queued.\n");}
    else if(eq(arg,"notifications")){size_t count=notification_count();if(!count)puts("No notifications.\n");for(size_t i=0;i<count;i++){const struct bread_notification *entry=notification_get(i);if(entry){puts(entry->sender);puts(": ");puts(entry->title);puts(" - ");puts(entry->body);if(entry->dismissed)puts(" [dismissed]");else if(entry->snoozed_until && (int32_t)(timer_ticks()-entry->snoozed_until)<0)puts(" [snoozed]");puts("\n");}}}
    else if(eq(arg,"snooze")) {
        char *p=rest;while(*p==' ')p++;
        char *q=p;while(*q>='0' && *q<='9')q++;
        if(*q){*q++=0;while(*q==' ')q++;}
        uint32_t index=0,duration=0;
        if(!*p || !*q || !parse_u32(p,&index) || !parse_u32(q,&duration) ||
           notification_snooze(index,timer_ticks(),duration)<0)
            puts("Usage: snooze INDEX TICKS (100 ticks = 1 second).\n");
        else puts("Notification snoozed.\n");
    }
    else if(eq(arg,"dismiss")) {
        uint32_t index=0;
        if(!parse_u32(rest,&index) || notification_dismiss(index)<0)
            puts("Usage: dismiss INDEX.\n");
        else puts("Notification dismissed.\n");
    }
    else if(eq(arg,"dnd")){if(!*rest)notification_set_dnd(!notification_dnd());else if(eq(rest,"on"))notification_set_dnd(1);else if(eq(rest,"off"))notification_set_dnd(0);else puts("Usage: dnd [on|off]\n");if(!*rest||eq(rest,"on")||eq(rest,"off"))puts(notification_dnd()?"Do Not Disturb enabled.\n":"Do Not Disturb disabled.\n");}
    else if(eq(arg,"about"))puts("BreadOS: freestanding 32-bit x86 kernel; Multiboot v1 boot.\n");
    else if(eq(arg,"video")){if(video_available()){puts("Framebuffer: ");hex32(video_width());puts(" x ");hex32(video_height());puts(" RGB\n");}else puts("No supported RGB framebuffer; VGA text console remains active.\n");}
    else if(eq(arg,"userdemo")){int result=process_run_user_demo();if(result==0)puts("Ring-3 process exited successfully.\n");else puts("Ring-3 process failed.\n");}
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
    if(video_init(mbi)==0){theme_apply(theme_current()->name);puts("GRUB RGB framebuffer mapped; native graphical text console initialized.\n");}else puts("No supported GRUB RGB framebuffer; VGA text fallback active.\n");
    int storage=pfs_init();if(storage==2)puts("Persistent BreadFS formatted and mounted.\n");else if(storage==1)puts("Persistent BreadFS mounted.\n");else{fs_init();if(storage==0)puts("RAM filesystem initialized (no usable ATA volume).\n");else puts("RAM filesystem initialized (ATA volume not recognized; disk left unchanged).\n");}
    tasking_init();puts("Round-robin kernel scheduler initialized (3 kernel threads).\n");
    if(keyboard_init()==0)puts("PS/2 keyboard IRQ1 configured.\n");else puts("PS/2 keyboard IRQ1 unavailable; polling fallback enabled.\n");
    if(mouse_init(video_available()?video_width():640u,video_available()?video_height():480u)==0)puts("PS/2 mouse IRQ12 configured.\n");else puts("PS/2 mouse unavailable.\n");
    char saved_theme[FS_DATA_MAX+1u];
    if(file_read("theme",saved_theme,sizeof(saved_theme))>=0){size_t i=0;while(saved_theme[i] && saved_theme[i]!='\n' && saved_theme[i]!='\r')i++;saved_theme[i]=0;if(theme_apply(saved_theme)<0)theme_apply("nord");}
    else theme_apply("nord");
    clear();
    puts("Type help for commands.\n");interrupts_init();
    while(task_switch_count()<6u){__asm__ volatile("hlt");}
    puts("Scheduler smoke test passed: round-robin task switching verified.\n");
    int user_result=process_run_user_demo();
    if(user_result==0)puts("Ring-3 process and syscall smoke test passed.\n");else puts("Ring-3 process and syscall smoke test failed.\n");
    prompt();
    for(;;){reminder_poll(timer_ticks());char c;if(!keyboard_read_char(&c)){__asm__ volatile("hlt");continue;}if(c=='\n'){command();continue;}if(c=='\b'){if(n){n--;put('\b');put(' ');put('\b');}continue;}if(n<sizeof(line)-1){line[n++]=c;put(c);}}
}
