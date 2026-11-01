#ifndef BREADOS_REMINDER_H
#define BREADOS_REMINDER_H
#include <stddef.h>
#include <stdint.h>

#define BREADOS_REMINDER_CAPACITY 16u

struct bread_reminder {
    uint32_t due_tick;
    char text[96];
    uint8_t active;
};

int reminder_add(uint32_t current_tick,uint32_t delay_ticks,const char *text);
size_t reminder_count(void);
/* Returns the nth active reminder in creation order. */
const struct bread_reminder *reminder_at(size_t index);
int reminder_dismiss(size_t index);
/* Converts due reminders into native notifications; returns the number fired. */
size_t reminder_poll(uint32_t current_tick);
void reminder_clear(void);

#endif
