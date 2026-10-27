#include "terminal.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static volatile uint16_t *terminal_buffer;

static uint8_t make_color(enum vga_color foreground,
                          enum vga_color background)
{
    return (uint8_t)(foreground | (background << 4));
}

static uint16_t make_entry(char character, uint8_t color)
{
    return (uint16_t)(uint8_t)character
         | ((uint16_t)color << 8);
}

void terminal_set_color(enum vga_color foreground,
                        enum vga_color background)
{
    terminal_color = make_color(foreground, background);
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = make_color(VGA_COLOR_LIGHT_GREY,
                                VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] =
                make_entry(' ', terminal_color);
        }
    }
}

static void terminal_scroll(void)
{
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[(y - 1) * VGA_WIDTH + x] =
                terminal_buffer[y * VGA_WIDTH + x];
        }
    }

    const size_t last_row = (VGA_HEIGHT - 1) * VGA_WIDTH;
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        terminal_buffer[last_row + x] =
            make_entry(' ', terminal_color);
    }

    terminal_row = VGA_HEIGHT - 1;
}

static void terminal_newline(void)
{
    terminal_column = 0;
    terminal_row++;

    if (terminal_row >= VGA_HEIGHT) {
        terminal_scroll();
    }
}

static void terminal_backspace(void)
{
    if (terminal_column == 0) {
        if (terminal_row == 0) {
            return;
        }
        terminal_row--;
        terminal_column = VGA_WIDTH;
    }

    terminal_column--;
    terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] =
        make_entry(' ', terminal_color);
}

void terminal_putchar(char character)
{
    if (character == '\n') {
        terminal_newline();
        return;
    }

    if (character == '\r') {
        terminal_column = 0;
        return;
    }

    if (character == '\b') {
        terminal_backspace();
        return;
    }

    const size_t index =
        terminal_row * VGA_WIDTH + terminal_column;

    terminal_buffer[index] =
        make_entry(character, terminal_color);

    terminal_column++;

    if (terminal_column >= VGA_WIDTH) {
        terminal_newline();
    }
}

void terminal_writestring(const char *string)
{
    while (*string != '\0') {
        terminal_putchar(*string++);
    }
}
