#include "video.h"
#include "paging.h"

#define MB_FLAG_FRAMEBUFFER (1u << 12)
#define MB_FRAMEBUFFER_RGB 1u
#define FB_VIRTUAL_BASE 0xE0000000u

struct multiboot_info_video {
    uint32_t flags;
    uint8_t reserved[84];
    uint64_t framebuffer_address;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t color_info[6];
} __attribute__((packed));

static struct video_mode active_mode;
static int active;

static int mask_valid(uint8_t position, uint8_t size, uint8_t bpp) {
    return size && size <= 8u && position < bpp && (uint16_t)position + size <= bpp;
}

int video_bind(const struct video_mode *mode) {
    if (!mode || !mode->pixels || !mode->width || !mode->height ||
        (mode->bits_per_pixel != 24u && mode->bits_per_pixel != 32u))
        return -1;
    uint32_t bytes = (uint32_t)(mode->bits_per_pixel / 8u);
    if (mode->width > 8192u || mode->height > 8192u ||
        mode->width > 0xFFFFFFFFu / bytes ||
        mode->pitch < mode->width * bytes ||
        mode->pitch > 0x10000000u ||
        mode->height > 0xFFFFFFFFu / mode->pitch)
        return -1;
    if (!mask_valid(mode->red_position, mode->red_mask_size, mode->bits_per_pixel) ||
        !mask_valid(mode->green_position, mode->green_mask_size, mode->bits_per_pixel) ||
        !mask_valid(mode->blue_position, mode->blue_mask_size, mode->bits_per_pixel))
        return -1;
    active_mode = *mode;
    active = 1;
    return 0;
}

int video_init(uint32_t info_address) {
    if (!info_address) return -1;
    const struct multiboot_info_video *info =
        (const struct multiboot_info_video *)(uintptr_t)info_address;
    if (!(info->flags & MB_FLAG_FRAMEBUFFER) ||
        info->framebuffer_type != MB_FRAMEBUFFER_RGB ||
        (info->framebuffer_bpp != 24u && info->framebuffer_bpp != 32u))
        return -1;
    uint32_t bytes = info->framebuffer_bpp / 8u;
    if (!info->framebuffer_width || !info->framebuffer_height ||
        info->framebuffer_width > 8192u || info->framebuffer_height > 8192u ||
        info->framebuffer_width > 0xFFFFFFFFu / bytes ||
        info->framebuffer_pitch < info->framebuffer_width * bytes ||
        info->framebuffer_height > 0xFFFFFFFFu / info->framebuffer_pitch)
        return -1;
    uint32_t length = info->framebuffer_pitch * info->framebuffer_height;
    volatile uint8_t *mapped = (volatile uint8_t *)
        paging_map_framebuffer(info->framebuffer_address, length);
    if (!mapped) return -1;
    struct video_mode mode = {
        mapped, info->framebuffer_width, info->framebuffer_height,
        info->framebuffer_pitch, info->framebuffer_bpp,
        info->color_info[0], info->color_info[1],
        info->color_info[2], info->color_info[3],
        info->color_info[4], info->color_info[5]
    };
    return video_bind(&mode);
}

int video_available(void) { return active; }
uint32_t video_width(void) { return active ? active_mode.width : 0; }
uint32_t video_height(void) { return active ? active_mode.height : 0; }

static uint32_t scale_channel(uint8_t value, uint8_t bits) {
    uint32_t maximum = (1u << bits) - 1u;
    return ((uint32_t)value * maximum + 127u) / 255u;
}

void video_pixel(uint32_t x, uint32_t y, uint32_t rgb) {
    if (!active || x >= active_mode.width || y >= active_mode.height) return;
    uint32_t r = scale_channel((uint8_t)(rgb >> 16), active_mode.red_mask_size);
    uint32_t g = scale_channel((uint8_t)(rgb >> 8), active_mode.green_mask_size);
    uint32_t b = scale_channel((uint8_t)rgb, active_mode.blue_mask_size);
    uint32_t pixel = (r << active_mode.red_position) |
                     (g << active_mode.green_position) |
                     (b << active_mode.blue_position);
    volatile uint8_t *dst = active_mode.pixels +
        (size_t)y * active_mode.pitch +
        (size_t)x * (active_mode.bits_per_pixel / 8u);
    dst[0] = (uint8_t)pixel;
    dst[1] = (uint8_t)(pixel >> 8);
    dst[2] = (uint8_t)(pixel >> 16);
    if (active_mode.bits_per_pixel == 32u) dst[3] = (uint8_t)(pixel >> 24);
}

void video_clear(uint32_t rgb) {
    if (!active) return;
    video_fill_rect(0, 0, active_mode.width, active_mode.height, rgb);
}

void video_fill_rect(uint32_t x, uint32_t y, uint32_t width,
                     uint32_t height, uint32_t rgb) {
    if (!active || x >= active_mode.width || y >= active_mode.height) return;
    uint32_t max_width = active_mode.width - x;
    uint32_t max_height = active_mode.height - y;
    if (width > max_width) width = max_width;
    if (height > max_height) height = max_height;
    for (uint32_t row = 0; row < height; ++row)
        for (uint32_t col = 0; col < width; ++col)
            video_pixel(x + col, y + row, rgb);
}
