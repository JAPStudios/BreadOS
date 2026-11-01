#ifndef BREADOS_AUDIO_H
#define BREADOS_AUDIO_H
#include <stdint.h>
/* PIT channel-2 PC-speaker output. Duration is measured in 100 Hz timer ticks. */
void audio_init(void);
void audio_stop(void);
uint16_t audio_compute_divisor(uint32_t frequency_hz);
int audio_play_tone(uint32_t frequency_hz,uint32_t duration_ticks);
#endif
