#include "reminder.h"
#include "notification.h"

static struct bread_reminder reminders[BREADOS_REMINDER_CAPACITY];

static void copy_text(char *destination,const char *source) {
    size_t i=0;
    if(source) while(source[i] && i<95u){destination[i]=source[i];i++;}
    destination[i]=0;
}

int reminder_add(uint32_t current_tick,uint32_t delay_ticks,const char *text) {
    if(!delay_ticks || !text || !*text) return -1;
    for(size_t i=0;i<BREADOS_REMINDER_CAPACITY;i++) {
        if(!reminders[i].active) {
            reminders[i].due_tick=current_tick+delay_ticks;
            copy_text(reminders[i].text,text);
            reminders[i].active=1;
            return 0;
        }
    }
    return -1;
}

size_t reminder_count(void) {
    size_t count=0;
    for(size_t i=0;i<BREADOS_REMINDER_CAPACITY;i++) if(reminders[i].active) count++;
    return count;
}

const struct bread_reminder *reminder_at(size_t index) {
    for(size_t i=0;i<BREADOS_REMINDER_CAPACITY;i++) {
        if(reminders[i].active) {
            if(!index) return &reminders[i];
            index--;
        }
    }
    return 0;
}

int reminder_dismiss(size_t index) {
    for(size_t i=0;i<BREADOS_REMINDER_CAPACITY;i++) {
        if(reminders[i].active) {
            if(!index) {reminders[i].active=0;return 0;}
            index--;
        }
    }
    return -1;
}

size_t reminder_poll(uint32_t current_tick) {
    size_t fired=0;
    for(size_t i=0;i<BREADOS_REMINDER_CAPACITY;i++) {
        struct bread_reminder *entry=&reminders[i];
        if(entry->active && (int32_t)(current_tick-entry->due_tick)>=0) {
            notification_push("Reminder","Reminder",entry->text,current_tick);
            entry->active=0;
            fired++;
        }
    }
    return fired;
}

void reminder_clear(void) {
    for(size_t i=0;i<BREADOS_REMINDER_CAPACITY;i++) reminders[i].active=0;
}
