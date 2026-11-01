#ifndef BREADOS_NOTIFICATION_H
#define BREADOS_NOTIFICATION_H
#include <stddef.h>
#include <stdint.h>

#define BREADOS_NOTIFICATION_CAPACITY 16u

struct bread_notification {
    char sender[16];
    char title[32];
    char body[96];
    uint32_t created_tick;
    uint32_t snoozed_until;
    uint8_t dismissed;
};

int notification_push(const char *sender,const char *title,const char *body,
                      uint32_t current_tick);
size_t notification_count(void);
/* Index zero is the newest retained notification. */
const struct bread_notification *notification_get(size_t index);
int notification_is_visible(size_t index,uint32_t current_tick);
int notification_dismiss(size_t index);
int notification_snooze(size_t index,uint32_t current_tick,uint32_t duration_ticks);
void notification_set_dnd(int enabled);
int notification_dnd(void);
void notification_clear(void);

#endif
