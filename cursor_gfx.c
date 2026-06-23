#include "cursor_gfx.h"
#include "graphics.h"

static uint32_t saved[16 * 16];
static int saved_x = -1, saved_y = -1;

static const uint8_t cursor_shape[16][16] = {
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,1,1,1,0,0,0,0,0,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,2,2,1,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,1,2,2,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0},
};

void cursor_gfx_erase(void) {
    if (saved_x < 0) return;
    uint32_t* fb = fb_get();
    uint32_t w = fb_width_get();
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            int px = saved_x + x;
            int py = saved_y + y;
            if (px >= 0 && px < (int)w && py >= 0 && py < (int)fb_height_get()) {
                fb[py * w + px] = saved[y * 16 + x];
            }
        }
    }
    saved_x = -1;
}

void cursor_gfx_draw(int x, int y) {
    uint32_t* fb = fb_get();
    uint32_t w = fb_width_get();
    uint32_t h = fb_height_get();

    saved_x = x;
    saved_y = y;
    
    // Save background
    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16; cx++) {
            int px = x + cx;
            int py = y + cy;
            if (px >= 0 && px < (int)w && py >= 0 && py < (int)h) {
                saved[cy * 16 + cx] = fb[py * w + px];
            } else {
                saved[cy * 16 + cx] = 0;
            }
        }
    }

    // Draw cursor
    for (int cy = 0; cy < 16; cy++) {
        for (int cx = 0; cx < 16; cx++) {
            int px = x + cx;
            int py = y + cy;
            if (px < 0 || px >= (int)w || py < 0 || py >= (int)h) continue;
            uint8_t p = cursor_shape[cy][cx];
            if (p == 1) {
                put_pixel(px, py, 0x000000);
            } else if (p == 2) {
                put_pixel(px, py, 0xFFFFFF);
            }
        }
    }
}
