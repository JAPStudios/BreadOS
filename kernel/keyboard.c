#include "keyboard.h"
#include <stdint.h>

#define KEY_QUEUE_SIZE 64u
#define KEY_QUEUE_MASK (KEY_QUEUE_SIZE - 1u)

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static int shift_down;
static int caps_lock;
static int extended_scancode;
static int irq_enabled;
static volatile uint8_t queue_head, queue_tail;
static char input_queue[KEY_QUEUE_SIZE];

static const uint8_t keymap[128] = {
    [0x02]=49,[0x03]=50,[0x04]=51,[0x05]=52,[0x06]=53,[0x07]=54,
    [0x08]=55,[0x09]=56,[0x0A]=57,[0x0B]=48,[0x0C]=45,[0x0D]=61,
    [0x0E]=8,[0x10]=113,[0x11]=119,[0x12]=101,[0x13]=114,[0x14]=116,
    [0x15]=121,[0x16]=117,[0x17]=105,[0x18]=111,[0x19]=112,
    [0x1A]=91,[0x1B]=93,[0x1C]=10,[0x1E]=97,[0x1F]=115,[0x20]=100,
    [0x21]=102,[0x22]=103,[0x23]=104,[0x24]=106,[0x25]=107,[0x26]=108,
    [0x27]=59,[0x28]=39,[0x29]=96,[0x2B]=92,[0x2C]=122,[0x2D]=120,
    [0x2E]=99,[0x2F]=118,[0x30]=98,[0x31]=110,[0x32]=109,[0x33]=44,
    [0x34]=46,[0x35]=47,[0x39]=32
};
static const uint8_t shifted_keymap[128] = {
    [0x02]=33,[0x03]=64,[0x04]=35,[0x05]=36,[0x06]=37,[0x07]=94,
    [0x08]=38,[0x09]=42,[0x0A]=40,[0x0B]=41,[0x0C]=95,[0x0D]=43,
    [0x1A]=123,[0x1B]=125,[0x27]=58,[0x28]=34,[0x29]=126,[0x2B]=124,
    [0x33]=60,[0x34]=62,[0x35]=63,[0x39]=32,
    [0x10]=81,[0x11]=87,[0x12]=69,[0x13]=82,[0x14]=84,[0x15]=89,
    [0x16]=85,[0x17]=73,[0x18]=79,[0x19]=80,[0x1E]=65,[0x1F]=83,
    [0x20]=68,[0x21]=70,[0x22]=71,[0x23]=72,[0x24]=74,[0x25]=75,
    [0x26]=76,[0x2C]=90,[0x2D]=88,[0x2E]=67,[0x2F]=86,[0x30]=66,
    [0x31]=78,[0x32]=77
};

static int wait_input_empty(void) {
    for (uint32_t i=0;i<100000u;i++) if (!(inb(0x64u)&2u)) return 1;
    return 0;
}
static int wait_output_full(void) {
    for (uint32_t i=0;i<100000u;i++) if (inb(0x64u)&1u) return 1;
    return 0;
}

int keyboard_init(void) {
    irq_enabled=0;
    if (!wait_input_empty()) return -1;
    outb(0x64u,0x20u);
    if (!wait_output_full()) return -1;
    uint8_t config=inb(0x60u);
    config=(uint8_t)((config|1u)&~0x10u);
    if (!wait_input_empty()) return -1;
    outb(0x64u,0x60u);
    if (!wait_input_empty()) return -1;
    outb(0x60u,config);
    irq_enabled=1;
    return 0;
}

int keyboard_process_scancode(uint8_t scancode, char *out) {
    if (!out) return 0;
    if (scancode == 0xE0u || scancode == 0xE1u) {
        extended_scancode = 1;
        return 0;
    }
    if (extended_scancode) {
        extended_scancode = 0;
        return 0;
    }
    uint8_t key = (uint8_t)(scancode & 0x7Fu);
    int released = (scancode & 0x80u) != 0;
    if (key == 0x2Au || key == 0x36u) {
        shift_down = !released;
        return 0;
    }
    if (key == 0x3Au) {
        if (!released) caps_lock = !caps_lock;
        return 0;
    }
    if (released) return 0;
    uint8_t normal = keymap[key];
    uint8_t shifted = shifted_keymap[key];
    if (normal >= 'a' && normal <= 'z') {
        int uppercase = shift_down ^ caps_lock;
        *out = uppercase ? (char)(normal - 'a' + 'A') : (char)normal;
        return 1;
    }
    if (shift_down && shifted) {
        *out = (char)shifted;
        return 1;
    }
    if (!normal) return 0;
    *out = (char)normal;
    return 1;
}

void keyboard_queue_scancode(uint8_t scancode) {
    char character;
    if (!keyboard_process_scancode(scancode,&character)) return;
    uint8_t next=(uint8_t)((queue_head+1u)&KEY_QUEUE_MASK);
    if (next==queue_tail) return;
    input_queue[queue_head]=character;
    queue_head=next;
}

void keyboard_irq_handler(void) {
    if (inb(0x64u)&1u) keyboard_queue_scancode(inb(0x60u));
}

int keyboard_read_char(char *out) {
    if (!out) return 0;
    if (queue_tail==queue_head && !irq_enabled && (inb(0x64u)&1u))
        keyboard_queue_scancode(inb(0x60u));
    if (queue_tail==queue_head) return 0;
    *out=input_queue[queue_tail];
    queue_tail=(uint8_t)((queue_tail+1u)&KEY_QUEUE_MASK);
    return 1;
}
