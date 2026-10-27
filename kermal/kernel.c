#include "terminal.h"

void kernel_main(void)
{
    terminal_initialize();

    terminal_set_color(VGA_COLOR_LIGHT_GREEN,
                       VGA_COLOR_BLACK);

    terminal_writestring("Ultimate OS\n");

    terminal_set_color(VGA_COLOR_LIGHT_GREY,
                       VGA_COLOR_BLACK);

    terminal_writestring("Kernel initialized successfully.\n");
    terminal_writestring("Architecture: x86 protected mode\n");
    terminal_writestring("Boot protocol: Multiboot\n");
    terminal_writestring("\n");
    terminal_writestring("System is running.\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}