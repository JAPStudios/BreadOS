#include "shell.h"
#include "keyboard.h"
#include "terminal.h"

#include <stddef.h>
#include <string.h>

#define SHELL_INPUT_SIZE 128

static char input[SHELL_INPUT_SIZE];

static int strings_equal(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

static void shell_print_prompt(void)
{
    terminal_set_color(VGA_COLOR_LIGHT_GREEN,
                       VGA_COLOR_BLACK);
    terminal_writestring("BreadOS");

    terminal_set_color(VGA_COLOR_LIGHT_GREY,
                       VGA_COLOR_BLACK);
    terminal_writestring("> ");
}

static void shell_execute(const char *command)
{
    if (strings_equal(command, "help")) {
        terminal_writestring(
            "Commands:\n"
            "  help  - List commands\n"
            "  about - Show OS information\n"
            "  clear - Clear the terminal\n"
            "  halt  - Stop the CPU\n"
        );
    } else if (strings_equal(command, "about")) {
        terminal_writestring(
            "BreadOS\n"
            "A minimal x86 operating-system kernel.\n"
        );
    } else if (strings_equal(command, "clear")) {
        terminal_initialize();
    } else if (strings_equal(command, "halt")) {
        terminal_writestring("BreadOS halted.\n");

        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    } else if (command[0] != '\0') {
        terminal_writestring("Unknown command: ");
        terminal_writestring(command);
        terminal_writestring("\nType 'help' for commands.\n");
    }
}

void shell_run(void)
{
    keyboard_initialize();

    terminal_writestring(
        "\nBreadOS interactive shell\n"
        "Type 'help' to list commands.\n\n"
    );

    for (;;) {
        size_t length = 0;

        input[0] = '\0';
        shell_print_prompt();

        for (;;) {
            char character = keyboard_getchar();

            if (character == '\n') {
                terminal_putchar('\n');
                input[length] = '\0';
                break;
            }

            if (character == '\b') {
                if (length > 0) {
                    length--;
                    input[length] = '\0';
                    terminal_putchar('\b');
                }
                continue;
            }

            if (character >= ' ' &&
                character <= '~' &&
                length < SHELL_INPUT_SIZE - 1) {
                input[length++] = character;
                input[length] = '\0';
                terminal_putchar(character);
            }
        }

        shell_execute(input);
    }
}