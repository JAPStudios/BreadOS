#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "reminder.h"
#include "notification.h"

int main(void) {
    reminder_clear();notification_clear();
    assert(reminder_add(100,50,"Tea is ready")==0);
    assert(reminder_add(100,0,"invalid")==-1);
    assert(reminder_count()==1u);
    assert(strcmp(reminder_at(0)->text,"Tea is ready")==0);
    assert(reminder_poll(149)==0u);
    assert(reminder_poll(150)==1u);
    assert(reminder_count()==0u);
    assert(notification_count()==1u);
    assert(strcmp(notification_get(0)->body,"Tea is ready")==0);
    assert(reminder_add(200,10,"First")==0);
    assert(reminder_add(200,20,"Second")==0);
    assert(reminder_dismiss(0)==0);
    assert(reminder_count()==1u);
    assert(reminder_poll(220)==1u);
    assert(notification_count()==2u);
    puts("Reminder service regression tests passed.");
    return 0;
}
