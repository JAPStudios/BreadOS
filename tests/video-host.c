#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../video.h"

static uint8_t pixels[4 * 4 * 4];
void *paging_map_framebuffer(uint64_t address, uint32_t length) {
    (void)address;
    return length <= sizeof(pixels) ? pixels : 0;
}

int main(void) {
    memset(pixels, 0, sizeof(pixels));
    struct video_mode mode = {
        pixels, 4, 4, 16, 32,
        16, 8, 8, 8, 0, 8
    };
    assert(video_bind(&mode) == 0);
    assert(video_available());
    assert(video_width() == 4 && video_height() == 4);
    video_pixel(1, 2, 0x123456u);
    uint8_t *p = pixels + 2 * 16 + 4;
    assert(p[0] == 0x56 && p[1] == 0x34 && p[2] == 0x12);
    assert(p[3] == 0);
    video_fill_rect(3, 3, 4, 4, 0xFF0000u);
    p = pixels + 3 * 16 + 12;
    assert(p[0] == 0 && p[1] == 0 && p[2] == 0xFF);
    video_pixel(4, 0, 0xFFFFFFu);
    struct video_mode invalid = mode;
    invalid.pitch = 1;
    assert(video_bind(&invalid) == -1);
    video_clear(0x0000FFu);
    assert(pixels[0] == 0xFF && pixels[1] == 0 && pixels[2] == 0);
    return 0;
}
