#include "terminal.h"
#include "shell.h"

void kernel_main(void)
{
    terminal_initialize();

    terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_writestring("BreadOS\n");

    terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_writestring("Minimal x86 kernel prototype\n");
    terminal_writestring("Boot protocol: GRUB Multiboot v1\n");
    terminal_writestring("Type 'help' for shell commands.\n\n");

    shell_run();

    /* shell_run is expected not to return. */
    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
