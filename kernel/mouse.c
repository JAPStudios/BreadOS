#include "mouse.h"
#include <stdint.h>

#define MOUSE_QUEUE_SIZE 32u
#define MOUSE_QUEUE_MASK (MOUSE_QUEUE_SIZE - 1u)

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static int enabled;
static uint8_t packet[3], packet_index;
static uint32_t bound_width=640u, bound_height=480u;
static uint32_t pointer_x, pointer_y;
static volatile uint8_t queue_head, queue_tail;
static struct mouse_event event_queue[MOUSE_QUEUE_SIZE];

static int wait_input_empty(void) {
    for (uint32_t i=0;i<100000u;i++) if (!(inb(0x64u)&2u)) return 1;
    return 0;
}
static int wait_output_full(void) {
    for (uint32_t i=0;i<100000u;i++) if (inb(0x64u)&1u) return 1;
    return 0;
}
static int send_aux_command(uint8_t command) {
    if (!wait_input_empty()) return 0;
    outb(0x64u,0xD4u);
    if (!wait_input_empty()) return 0;
    outb(0x60u,command);
    if (!wait_output_full()) return 0;
    return inb(0x60u)==0xFAu;
}

void mouse_set_bounds(uint32_t width, uint32_t height) {
    bound_width=width ? width : 1u;
    bound_height=height ? height : 1u;
    if (pointer_x>=bound_width) pointer_x=bound_width-1u;
    if (pointer_y>=bound_height) pointer_y=bound_height-1u;
}

int mouse_init(uint32_t width, uint32_t height) {
    enabled=0;
    packet_index=0;
    queue_head=queue_tail=0;
    mouse_set_bounds(width,height);
    pointer_x=bound_width/2u;
    pointer_y=bound_height/2u;

    if (!wait_input_empty()) return -1;
    outb(0x64u,0xA8u); /* Enable the auxiliary PS/2 port. */
    if (!wait_input_empty()) return -1;
    outb(0x64u,0x20u); /* Read controller configuration byte. */
    if (!wait_output_full()) return -1;
    uint8_t config=inb(0x60u);
    config=(uint8_t)((config|0x02u)&~0x20u); /* IRQ12 on; aux clock enabled. */
    if (!wait_input_empty()) return -1;
    outb(0x64u,0x60u);
    if (!wait_input_empty()) return -1;
    outb(0x60u,config);
    if (!send_aux_command(0xF4u)) return -1; /* Enable mouse data reporting. */
    enabled=1;
    return 0;
}

void mouse_feed_byte(uint8_t value) {
    if (packet_index==0u) {
        if (!(value&0x08u)) return; /* Packet synchronization bit. */
        packet[0]=value;
        packet_index=1u;
        return;
    }
    packet[packet_index++]=value;
    if (packet_index<3u) return;
    packet_index=0u;

    uint8_t flags=packet[0];
    if (flags&0xC0u) return; /* Ignore X/Y overflow packets. */
    int16_t dx=(int8_t)packet[1];
    int16_t dy=(int8_t)packet[2];
    int32_t next_x=(int32_t)pointer_x+dx;
    int32_t next_y=(int32_t)pointer_y-dy;
    if (next_x<0) next_x=0;
    if (next_y<0) next_y=0;
    if ((uint32_t)next_x>=bound_width) next_x=(int32_t)(bound_width-1u);
    if ((uint32_t)next_y>=bound_height) next_y=(int32_t)(bound_height-1u);
    pointer_x=(uint32_t)next_x;
    pointer_y=(uint32_t)next_y;

    uint8_t next=(uint8_t)((queue_head+1u)&MOUSE_QUEUE_MASK);
    if (next==queue_tail) return;
    event_queue[queue_head].x=pointer_x;
    event_queue[queue_head].y=pointer_y;
    event_queue[queue_head].delta_x=dx;
    event_queue[queue_head].delta_y=(int16_t)-dy;
    event_queue[queue_head].buttons=(uint8_t)(flags&0x07u);
    queue_head=next;
}

void mouse_irq_handler(void) {
    if (enabled && (inb(0x64u)&1u)) mouse_feed_byte(inb(0x60u));
}

int mouse_read_event(struct mouse_event *event) {
    if (!event || queue_tail==queue_head) return 0;
    *event=event_queue[queue_tail];
    queue_tail=(uint8_t)((queue_tail+1u)&MOUSE_QUEUE_MASK);
    return 1;
}
