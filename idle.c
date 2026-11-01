#include "idle.h"

#define DEFAULT_IDLE_TIMEOUT 30000u

static uint32_t timeout_ticks=DEFAULT_IDLE_TIMEOUT;
static uint32_t last_activity;
static int active;

void idle_set_timeout(uint32_t value) {
    timeout_ticks=value;
    active=0;
}
uint32_t idle_timeout(void) { return timeout_ticks; }
void idle_init(uint32_t current_tick) {
    last_activity=current_tick;
    active=0;
}
void idle_activity(uint32_t current_tick) {
    last_activity=current_tick;
    active=0;
}
int idle_poll(uint32_t current_tick) {
    if(!timeout_ticks) {active=0;return 0;}
    if(!active && (uint32_t)(current_tick-last_activity)>=timeout_ticks) active=1;
    return active;
}
int idle_is_active(void) { return active; }
