#ifndef BREADOS_KEYBOARD_H
#define BREADOS_KEYBOARD_H

/* Read one translated character from the legacy PS/2 keyboard.
 * Returns 1 and stores the character when input is available; otherwise 0.
 * Enter is returned as '\n'; Backspace is returned as '\b'.
 */
int keyboard_read_char(char *out);

#endif
