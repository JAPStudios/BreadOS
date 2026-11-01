#include <assert.h>
#include <stdio.h>
#include "kernel/keyboard.h"

static char output;

static void expect_char(unsigned code, char expected) {
    assert(keyboard_process_scancode((uint8_t)code, &output) == 1);
    assert(output == expected);
}
static void expect_no_char(unsigned code) {
    assert(keyboard_process_scancode((uint8_t)code, &output) == 0);
}

int main(void) {
    expect_char(0x1E, 'a');
    expect_no_char(0xAA);
    expect_no_char(0x2A);
    expect_char(0x1E, 'A');
    expect_no_char(0x9E);
    expect_no_char(0xAA);
    expect_no_char(0x2A);
    expect_char(0x02, '!');
    expect_no_char(0xAA);
    expect_no_char(0x3A);
    expect_char(0x1E, 'A');
    expect_no_char(0xBA);
    expect_char(0x1E, 'A');
    expect_no_char(0x3A);
    expect_char(0x1E, 'a');
    expect_no_char(0xE0);
    expect_no_char(0x48);

    keyboard_queue_scancode(0x1E);
    keyboard_queue_scancode(0x30);
    assert(keyboard_read_char(&output)==1 && output=='a');
    assert(keyboard_read_char(&output)==1 && output=='b');
    puts("Keyboard host regression tests passed.");
    return 0;
}
