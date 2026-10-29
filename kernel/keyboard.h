#ifndef BREADOS_KEYBOARD_H
#define BREADOS_KEYBOARD_H
#include <stdint.h>

/* Poll one translated character from the legacy PS/2 keyboard. Supports
 * US-layout Shift and Caps Lock. Extended keys and key releases are ignored.
 */
int keyboard_read_char(char *out);

/* Exposed for deterministic host-side scancode regression tests. */
int keyboard_process_scancode(uint8_t scancode, char *out);

#endif
