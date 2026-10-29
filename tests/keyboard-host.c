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
    expect_no_char(0xAA);         /* left shift release must not stick */
    expect_no_char(0x2A);         /* left shift down */
    expect_char(0x1E, 'A');
    expect_no_char(0x9E);         /* a release */
    expect_no_char(0xAA);         /* left shift up */
    expect_no_char(0x2A);         /* shift down again */
    expect_char(0x02, '!');       /* shifted number-row punctuation */
    expect_no_char(0xAA);         /* shift up */
    expect_no_char(0x3A);         /* Caps Lock on */
    expect_char(0x1E, 'A');
    expect_no_char(0xBA);         /* Caps Lock release does not toggle */
    expect_char(0x1E, 'A');
    expect_no_char(0x3A);         /* Caps Lock off */
    expect_char(0x1E, 'a');
    expect_no_char(0xE0);         /* extended key prefix */
    expect_no_char(0x48);         /* extended arrow is ignored */
    puts("Keyboard host regression tests passed.");
    return 0;
}
