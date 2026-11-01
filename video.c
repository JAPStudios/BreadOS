#include "video.h"
#include "paging.h"

#define MB_FLAG_FRAMEBUFFER (1u << 12)
#define MB_FRAMEBUFFER_RGB 1u
#define CELL_WIDTH 8u
#define CELL_HEIGHT 16u

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
static uint32_t cursor_x, cursor_y, console_bg, console_fg;

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
    cursor_x = cursor_y = 0;
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
    if (active) video_fill_rect(0, 0, active_mode.width, active_mode.height, rgb);
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

/* Compact 5x7 uppercase glyphs. Lowercase input is folded to uppercase. */
static const uint8_t alpha_numeric[36][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14},{7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14},{17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31},{30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14},{14,17,17,15,1,1,14}
};

static void glyph_rows(char ch, uint8_t rows[7]) {
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
    if (ch >= 'A' && ch <= 'Z') {
        for (uint32_t i=0;i<7;i++) rows[i]=alpha_numeric[(uint32_t)(ch-'A')][i];
        return;
    }
    if (ch >= '0' && ch <= '9') {
        for (uint32_t i=0;i<7;i++) rows[i]=alpha_numeric[26u+(uint32_t)(ch-'0')][i];
        return;
    }
    static const struct { char ch; uint8_t row[7]; } punctuation[] = {
        {' ',{0,0,0,0,0,0,0}}, {'.',{0,0,0,0,0,12,12}},
        {',',{0,0,0,0,0,12,8}}, {':',{0,12,12,0,12,12,0}},
        {';',{0,12,12,0,12,8,0}}, {'-',{0,0,0,31,0,0,0}},
        {'_',{0,0,0,0,0,0,31}}, {'/',{1,2,2,4,8,8,16}},
        {'\\',{16,8,8,4,2,2,1}}, {'+',{0,4,4,31,4,4,0}},
        {'=',{0,31,0,31,0,0,0}}, {'(',{2,4,8,8,8,4,2}},
        {')',{8,4,2,2,2,4,8}}, {'!',{4,4,4,4,4,0,4}},
        {'?',{14,17,1,2,4,0,4}}, {'\'',{4,4,8,0,0,0,0}},
        {'"',{10,10,20,0,0,0,0}}, {'>',{16,8,4,2,4,8,16}},
        {'<',{1,2,4,8,4,2,1}}, {'#',{10,10,31,10,31,10,10}},
        {'*',{0,21,14,31,14,21,0}}, {'[',{14,8,8,8,8,8,14}},
        {']',{14,2,2,2,2,2,14}}, {'@',{14,17,23,21,23,16,15}},
        {'$',{4,15,20,14,5,30,4}}, {'%',{25,25,2,4,8,19,19}},
        {'&',{12,18,20,8,21,18,13}}, {'|',{4,4,4,4,4,4,4}},
        {'~',{0,0,9,22,0,0,0}}
    };
    for (uint32_t i=0;i<sizeof(punctuation)/sizeof(punctuation[0]);i++) {
        if (punctuation[i].ch == ch) {
            for (uint32_t row=0;row<7;row++) rows[row]=punctuation[i].row[row];
            return;
        }
    }
    for (uint32_t i=0;i<7;i++) rows[i]=0;
}

static void console_scroll(void) {
    uint32_t bytes_per_pixel = active_mode.bits_per_pixel / 8u;
    uint32_t scroll_bytes = (active_mode.height > CELL_HEIGHT)
        ? (active_mode.height - CELL_HEIGHT) * active_mode.pitch : 0;
    volatile uint8_t *pixels = active_mode.pixels;
    for (uint32_t i=0;i<scroll_bytes;i++) pixels[i]=pixels[i+CELL_HEIGHT*active_mode.pitch];
    (void)bytes_per_pixel;
    video_fill_rect(0, active_mode.height-CELL_HEIGHT, active_mode.width,
                    CELL_HEIGHT, console_bg);
    uint32_t rows = active_mode.height / CELL_HEIGHT;
    cursor_y = rows ? rows-1u : 0;
}

static void console_draw(char ch) {
    uint8_t rows[7];
    glyph_rows(ch, rows);
    uint32_t x = cursor_x * CELL_WIDTH;
    uint32_t y = cursor_y * CELL_HEIGHT;
    video_fill_rect(x,y,CELL_WIDTH,CELL_HEIGHT,console_bg);
    for (uint32_t gy=0;gy<7;gy++) {
        for (uint32_t gx=0;gx<5;gx++) {
            if (rows[gy] & (1u << (4u-gx))) {
                video_pixel(x+1u+gx,y+1u+gy*2u,console_fg);
                video_pixel(x+1u+gx,y+2u+gy*2u,console_fg);
            }
        }
    }
}

void video_console_clear(uint32_t background, uint32_t foreground) {
    if (!active) return;
    console_bg=background;
    console_fg=foreground;
    cursor_x=cursor_y=0;
    video_clear(background);
}

void video_console_put(char ch) {
    if (!active) return;
    uint32_t columns=active_mode.width/CELL_WIDTH;
    uint32_t lines=active_mode.height/CELL_HEIGHT;
    if (!columns || !lines) return;
    if (ch=='\r') { cursor_x=0; return; }
    if (ch=='\n') {
        cursor_x=0;
        if (++cursor_y>=lines) console_scroll();
        return;
    }
    if (ch=='\b') {
        if (cursor_x) cursor_x--;
        else if (cursor_y) { cursor_y--; cursor_x=columns-1u; }
        console_draw(' ');
        return;
    }
    if (ch=='\t') {
        uint32_t target=(cursor_x+4u)&~3u;
        do { video_console_put(' '); } while (cursor_x && cursor_x<target);
        return;
    }
    console_draw(ch);
    if (++cursor_x>=columns) {
        cursor_x=0;
        if (++cursor_y>=lines) console_scroll();
    }
}
