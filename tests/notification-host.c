#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "notification.h"

int main(void) {
    notification_clear();
    assert(notification_push("System","Boot","BreadOS started",10)==0);
    assert(notification_count()==1u);
    assert(notification_is_visible(0,10));
    assert(strcmp(notification_get(0)->sender,"System")==0);
    assert(notification_snooze(0,10,50)==0);
    assert(!notification_is_visible(0,20));
    assert(notification_is_visible(0,60));
    assert(notification_dismiss(0)==0);
    assert(!notification_is_visible(0,100));
    notification_set_dnd(1);
    assert(notification_push("Network","Wi-Fi","Connected",100)==1);
    assert(!notification_is_visible(0,100));
    notification_set_dnd(0);
    assert(notification_is_visible(0,100));
    for(uint32_t i=0;i<20;i++) notification_push("Test","Entry","Ring buffer",200+i);
    assert(notification_count()==BREADOS_NOTIFICATION_CAPACITY);
    assert(strcmp(notification_get(0)->title,"Entry")==0);
    puts("Notification service regression tests passed.");
    return 0;
}
