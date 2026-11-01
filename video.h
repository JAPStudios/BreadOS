#ifndef BREADOS_VIDEO_H
#define BREADOS_VIDEO_H
#include <stdint.h>
#include <stddef.h>

struct video_mode {
    volatile uint8_t *pixels;
    uint32_t width, height, pitch;
    uint8_t bits_per_pixel;
    uint8_t red_position, red_mask_size;
    uint8_t green_position, green_mask_size;
    uint8_t blue_position, blue_mask_size;
};

int video_init(uint32_t multiboot_info_address);
int video_bind(const struct video_mode *mode);
int video_available(void);
uint32_t video_width(void);
uint32_t video_height(void);
void video_clear(uint32_t rgb);
void video_pixel(uint32_t x, uint32_t y, uint32_t rgb);
void video_fill_rect(uint32_t x, uint32_t y, uint32_t width,
                     uint32_t height, uint32_t rgb);
void video_console_clear(uint32_t background, uint32_t foreground);
void video_console_put(char character);

#endif
