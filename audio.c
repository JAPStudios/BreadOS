#include "audio.h"
#include <stdint.h>
#define PIT_CHANNEL2 0x42u
#define PIT_COMMAND 0x43u
#define SPEAKER_PORT 0x61u
#define PIT_INPUT_HZ 1193180u
extern uint32_t timer_ticks(void);
static inline void outb(uint16_t port,uint8_t value){__asm__ volatile("outb %0,%1"::"a"(value),"Nd"(port));}
static inline uint8_t inb(uint16_t port){uint8_t value;__asm__ volatile("inb %1,%0":"=a"(value):"Nd"(port));return value;}
uint16_t audio_compute_divisor(uint32_t frequency_hz){if(frequency_hz<20u||frequency_hz>20000u)return 0;uint32_t divisor=PIT_INPUT_HZ/frequency_hz;if(!divisor||divisor>0xFFFFu)return 0;return (uint16_t)divisor;}
void audio_stop(void){uint8_t state=inb(SPEAKER_PORT);outb(SPEAKER_PORT,(uint8_t)(state&~3u));}
void audio_init(void){audio_stop();}
int audio_play_tone(uint32_t frequency_hz,uint32_t duration_ticks){uint16_t divisor=audio_compute_divisor(frequency_hz);if(!divisor||!duration_ticks||duration_ticks>1000u)return -1;outb(PIT_COMMAND,0xB6u);outb(PIT_CHANNEL2,(uint8_t)divisor);outb(PIT_CHANNEL2,(uint8_t)(divisor>>8));uint8_t state=inb(SPEAKER_PORT);outb(SPEAKER_PORT,(uint8_t)(state|3u));uint32_t deadline=timer_ticks()+duration_ticks;while((int32_t)(timer_ticks()-deadline)<0)__asm__ volatile("hlt":::"memory");audio_stop();return 0;}
