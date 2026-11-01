#ifndef BREADOS_KEYBOARD_H
#define BREADOS_KEYBOARD_H
#include <stdint.h>

/* Initialize the PS/2 controller for IRQ1 delivery. */
int keyboard_init(void);

/* Translate a Set 1 scancode into a character; returns 1 when produced. */
int keyboard_process_scancode(uint8_t scancode, char *out);

/* IRQ1 entry point: consume the controller byte and queue translated input. */
void keyboard_irq_handler(void);

/* Testable scancode-to-input-queue path. */
void keyboard_queue_scancode(uint8_t scancode);

/* Non-blocking character read from the IRQ-fed input queue. */
int keyboard_read_char(char *out);

#endif
