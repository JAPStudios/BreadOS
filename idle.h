#ifndef BREADOS_IDLE_H
#define BREADOS_IDLE_H
#include <stdint.h>

/* Timeout is measured in PIT ticks (100 ticks per second); zero disables idle. */
void idle_set_timeout(uint32_t timeout_ticks);
uint32_t idle_timeout(void);
void idle_init(uint32_t current_tick);
void idle_activity(uint32_t current_tick);
int idle_poll(uint32_t current_tick);
int idle_is_active(void);

#endif
