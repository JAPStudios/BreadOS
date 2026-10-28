#include "interrupts.h"
#include <stddef.h>
#include <stdint.h>

#define IDT_ENTRIES 256
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t attributes;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static volatile uint32_t ticks;
extern void isr0(void); extern void isr1(void); extern void isr2(void); extern void isr3(void);
extern void isr4(void); extern void isr5(void); extern void isr6(void); extern void isr7(void);
extern void isr8(void); extern void isr9(void); extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
extern void irq0(void);

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline void lidt(const struct idt_pointer *p) {
    __asm__ volatile ("lidt (%0)" : : "r"(p) : "memory");
}
static void set_gate(unsigned n, void (*handler)(void)) {
    uint32_t address = (uint32_t)handler;
    idt[n].offset_low = (uint16_t)(address & 0xFFFF);
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].attributes = 0x8E;
    idt[n].offset_high = (uint16_t)(address >> 16);
}
static void pic_remap(void) {
    outb(PIC1_COMMAND, 0x11); outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20); outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04); outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01); outb(PIC2_DATA, 0x01);
    /* Enable only IRQ0 (PIT); mask all other hardware IRQs for now. */
    outb(PIC1_DATA, 0xFE); outb(PIC2_DATA, 0xFF);
}
static void pit_init(void) {
    /* PIT input clock ~1,193,182 Hz; divisor 11932 gives ~100 Hz. */
    uint16_t divisor = 11932;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}
void interrupts_init(void) {
    void (*const exceptions[32])(void) = {
        isr0,isr1,isr2,isr3,isr4,isr5,isr6,isr7,isr8,isr9,isr10,isr11,isr12,isr13,isr14,isr15,
        isr16,isr17,isr18,isr19,isr20,isr21,isr22,isr23,isr24,isr25,isr26,isr27,isr28,isr29,isr30,isr31
    };
    for (size_t i=0;i<IDT_ENTRIES;i++) {
        idt[i].offset_low=0; idt[i].selector=0; idt[i].zero=0; idt[i].attributes=0; idt[i].offset_high=0;
    }
    for (unsigned i=0;i<32;i++) set_gate(i, exceptions[i]);
    set_gate(32, irq0);
    struct idt_pointer p = { (uint16_t)(sizeof(idt)-1), (uint32_t)(uintptr_t)idt };
    pic_remap();
    lidt(&p);
    pit_init();
    __asm__ volatile ("sti" : : : "memory");
}
uint32_t timer_ticks(void) { return ticks; }
static void serial_char(char c) {
    /* COM1 is initialized by kernel_main before interrupts are enabled. */
    for (unsigned wait=0; wait<1000000; ++wait) {
        uint8_t status;
        __asm__ volatile ("inb %1, %0" : "=a"(status) : "Nd"((uint16_t)0x3FD));
        if (status & 0x20) {
            __asm__ volatile ("outb %0, %1" : : "a"((uint8_t)c), "Nd"((uint16_t)0x3F8));
            return;
        }
    }
}
static void serial_text(const char *s) { while (*s) serial_char(*s++); }
static void serial_hex(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    for (int shift=28; shift>=0; shift-=4) serial_char(digits[(value >> shift) & 0xF]);
}
static void fatal_exception(uint32_t vector, uint32_t error_code) {
    volatile uint16_t *vga=(volatile uint16_t *)0xB8000;
    const char *msg="BreadOS fatal CPU exception; halted.";
    for (unsigned i=0;msg[i] && i<80;i++) vga[i]=(uint16_t)(uint8_t)msg[i] | 0x4F00;
    serial_text("FATAL CPU exception vector=0x");
    serial_hex(vector);
    serial_text(" error=0x");
    serial_hex(error_code);
    serial_text("\r\n");
    __asm__ volatile ("cli");
    for (;;) __asm__ volatile ("hlt");
}
void interrupt_handler(uint32_t vector, uint32_t error_code) {
    if (vector < 32) fatal_exception(vector, error_code);
    if (vector == 32) {
        ticks++;
        outb(PIC1_COMMAND, 0x20);
    }
}
