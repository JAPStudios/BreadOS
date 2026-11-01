#include <assert.h>
#include <stdio.h>
#include "kernel/mouse.h"

int main(void) {
    struct mouse_event event;
    mouse_set_bounds(100,100);
    mouse_feed_byte(0x09); /* sync + left button */
    mouse_feed_byte(10);
    mouse_feed_byte(5);
    assert(mouse_read_event(&event)==1);
    assert(event.x==10 && event.y==0);
    assert(event.delta_x==10 && event.delta_y==-5);
    assert(event.buttons==1);

    mouse_feed_byte(0x08);
    mouse_feed_byte(0xFB); /* -5 */
    mouse_feed_byte(0xFF); /* -1 */
    assert(mouse_read_event(&event)==1);
    assert(event.x==5 && event.y==1);
    assert(event.buttons==0);

    mouse_set_bounds(4,4);
    mouse_feed_byte(0x08);
    mouse_feed_byte(127);
    mouse_feed_byte(127);
    assert(mouse_read_event(&event)==1);
    assert(event.x==3 && event.y==0);
    mouse_feed_byte(0xC8); /* overflow packet is discarded */
    mouse_feed_byte(1);
    mouse_feed_byte(1);
    assert(mouse_read_event(&event)==0);
    puts("Mouse host regression tests passed.");
    return 0;
}
