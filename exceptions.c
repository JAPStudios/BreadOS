#include <stdint.h>
#include <stddef.h>
typedef struct __attribute__((packed)) {
 uint16_t low,selector;uint8_t zero,flags;uint16_t high;
} Gate;
typedef struct __attribute__((packed)) {uint16_t limit;uint32_t base;} Idtr;
typedef struct {
 uint32_t edi,esi,ebp,old_esp,ebx,edx,ecx,eax;
 uint32_t vector,error,eip,cs,eflags;
} Frame;
_Static_assert(sizeof(Gate)==8,"IDT gate size");
_Static_assert(sizeof(Idtr)==6,"IDTR size");
_Static_assert(offsetof(Frame,vector)==32,"exception frame layout");
_Static_assert(offsetof(Frame,eip)==40,"exception frame layout");
extern void (*exception_handlers[32])(void);
extern void exception_unexpected(void);
static Gate idt[256];
static void gate(unsigned n,void (*handler)(void),uint16_t cs){
 uint32_t address=(uint32_t)(uintptr_t)handler;
 idt[n].low=address&65535;idt[n].high=address>>16;
 idt[n].selector=cs;idt[n].zero=0;idt[n].flags=0x8E;
}
void exceptions_init(void){
 uint16_t cs;__asm__ volatile("mov %%cs,%0":"=r"(cs));
 for(unsigned n=0;n<256;n++)gate(n,exception_unexpected,cs);
 for(unsigned n=0;n<32;n++)gate(n,exception_handlers[n],cs);
 Idtr descriptor={(uint16_t)(sizeof(idt)-1),(uint32_t)(uintptr_t)idt};
 __asm__ volatile("lidt %0"::"m"(descriptor):"memory");
}
static volatile uint16_t *const video=(volatile uint16_t *)0xB8000;
static unsigned cursor;
static void text(const char *s){while(*s&&cursor<2000)video[cursor++]=0x4F00|(unsigned char)*s++;}
static void hex(uint32_t value){
 const char *digits="0123456789ABCDEF";text("0x");
 for(int n=7;n>=0;n--){char s[2]={digits[(value>>(n*4))&15],0};text(s);}
}
__attribute__((noreturn)) void exceptions_panic(const Frame *f){
 __asm__ volatile("cli");
 for(unsigned i=0;i<2000;i++)video[i]=0x4F20;
 cursor=0;text("FATAL CPU EXCEPTION - kernel stopped");
 cursor=80;text("Vector: ");hex(f->vector);
 cursor=160;text("Error:  ");hex(f->error);
 cursor=240;text("EIP:    ");hex(f->eip);
 cursor=320;text("CS:     ");hex(f->cs);
 cursor=480;text("Unsaved RAM files may be lost. Close the emulator.");
 for(;;)__asm__ volatile("hlt");
}
/* Optional development-only test; execute only after exceptions_init(). */
void exceptions_test(void){__asm__ volatile("int3");}
