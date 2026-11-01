#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "audio.h"
uint32_t timer_ticks(void){return 0u;}
int main(void){assert(audio_compute_divisor(440u)==2711u);assert(audio_compute_divisor(1000u)==1193u);assert(audio_compute_divisor(20u)==59659u);assert(audio_compute_divisor(20000u)==59u);assert(audio_compute_divisor(0u)==0u);assert(audio_compute_divisor(19u)==0u);assert(audio_compute_divisor(20001u)==0u);assert(audio_play_tone(0u,10u)==-1);assert(audio_play_tone(440u,0u)==-1);assert(audio_play_tone(440u,1001u)==-1);puts("PC-speaker audio validation tests passed.");return 0;}
