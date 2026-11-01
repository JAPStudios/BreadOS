#ifndef BREADOS_MOUSE_H
#define BREADOS_MOUSE_H
#include <stdint.h>

struct mouse_event {
    uint32_t x, y;
    int16_t delta_x, delta_y;
    uint8_t buttons;
};

/* Enable the standard three-byte PS/2 mouse packet stream and IRQ12. */
int mouse_init(uint32_t screen_width, uint32_t screen_height);

/* IRQ12 entry point and host-testable packet decoder. */
void mouse_irq_handler(void);
void mouse_set_bounds(uint32_t width, uint32_t height);
void mouse_feed_byte(uint8_t value);

/* Non-blocking read from the bounded mouse-event queue. */
int mouse_read_event(struct mouse_event *event);

#endif
