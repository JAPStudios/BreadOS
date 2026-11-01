#include "notification.h"

static struct bread_notification history[BREADOS_NOTIFICATION_CAPACITY];
static uint32_t next_slot,used_count;
static int dnd_enabled;

static void copy_text(char *destination,size_t capacity,const char *source) {
    if(!capacity) return;
    size_t i=0;
    if(source) while(source[i] && i+1u<capacity){destination[i]=source[i];i++;}
    destination[i]=0;
}

static struct bread_notification *get_mutable(size_t index) {
    if(index>=used_count) return 0;
    uint32_t slot=(next_slot+BREADOS_NOTIFICATION_CAPACITY-1u-(uint32_t)index)
                  %BREADOS_NOTIFICATION_CAPACITY;
    return &history[slot];
}

int notification_push(const char *sender,const char *title,const char *body,
                      uint32_t current_tick) {
    struct bread_notification *entry=&history[next_slot];
    copy_text(entry->sender,sizeof(entry->sender),sender);
    copy_text(entry->title,sizeof(entry->title),title);
    copy_text(entry->body,sizeof(entry->body),body);
    entry->created_tick=current_tick;
    entry->snoozed_until=0;
    entry->dismissed=0;
    next_slot=(next_slot+1u)%BREADOS_NOTIFICATION_CAPACITY;
    if(used_count<BREADOS_NOTIFICATION_CAPACITY) used_count++;
    return dnd_enabled?1:0;
}

size_t notification_count(void) { return used_count; }
const struct bread_notification *notification_get(size_t index) {
    return get_mutable(index);
}
int notification_is_visible(size_t index,uint32_t current_tick) {
    const struct bread_notification *entry=get_mutable(index);
    if(!entry || dnd_enabled || entry->dismissed) return 0;
    if(entry->snoozed_until && (int32_t)(current_tick-entry->snoozed_until)<0) return 0;
    return 1;
}
int notification_dismiss(size_t index) {
    struct bread_notification *entry=get_mutable(index);
    if(!entry) return -1;
    entry->dismissed=1;
    return 0;
}
int notification_snooze(size_t index,uint32_t current_tick,uint32_t duration_ticks) {
    struct bread_notification *entry=get_mutable(index);
    if(!entry || !duration_ticks) return -1;
    entry->snoozed_until=current_tick+duration_ticks;
    return 0;
}
void notification_set_dnd(int enabled) { dnd_enabled=enabled?1:0; }
int notification_dnd(void) { return dnd_enabled; }
void notification_clear(void) {
    next_slot=used_count=0;
    dnd_enabled=0;
}
