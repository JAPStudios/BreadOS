#include "syscall.h"
#include <stdint.h>

/* These numbers are an ABI: do not renumber once user programs depend on them. */
enum {
    BREADOS_SYS_VERSION = 0,
    BREADOS_SYS_WRITE_CHAR = 1,
    BREADOS_SYS_EXIT = 2
};

#define BREADOS_ABI_VERSION 1u
#define VGA_WIDTH 80u
#define VGA_HEIGHT 25u

static uint32_t cursor;

static void console_write_char(char ch) {
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    if (ch == '\r') {
        cursor -= cursor % VGA_WIDTH;
        return;
    }
    if (ch == '\n') {
        cursor = ((cursor / VGA_WIDTH) + 1u) * VGA_WIDTH;
    } else {
        if (cursor >= VGA_WIDTH * VGA_HEIGHT)
            cursor = 0;
        vga[cursor++] = (uint16_t)(uint8_t)ch | 0x0F00u;
    }
    if (cursor >= VGA_WIDTH * VGA_HEIGHT)
        cursor = 0;
}

void syscall_init(void) {
    cursor = 0;
}

void syscall_dispatch(struct syscall_frame *frame) {
    if (frame == 0)
        return;

    switch (frame->eax) {
    case BREADOS_SYS_VERSION:
        frame->eax = BREADOS_ABI_VERSION;
        break;
    case BREADOS_SYS_WRITE_CHAR:
        /* One character only: no unchecked user pointer is dereferenced. */
        console_write_char((char)(frame->ebx & 0xFFu));
        frame->eax = 0;
        break;
    case BREADOS_SYS_EXIT:
        /* Process teardown is deliberately not claimed until process management exists. */
        frame->eax = (uint32_t)-1;
        break;
    default:
        frame->eax = (uint32_t)-1;
        break;
    }
}
