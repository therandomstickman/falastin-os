// graphics.c
#include "graphics.h"
#include <stdint.h>

static uint32_t* fb = 0;
static uint32_t  fb_pitch_px = 0;  // pitch in pixels, NOT bytes
static uint32_t  fb_width = 0;
static uint32_t  fb_height = 0;
uint32_t* fb_get(void) { return fb; }

void graphics_init(uint32_t addr, uint32_t pitch, uint32_t w, uint32_t h, uint8_t bpp) {
    (void)bpp;
    fb          = (uint32_t*)addr;
    fb_pitch_px = pitch / 4;     // bytes -> pixels (32bpp assumed)
    fb_width    = w;
    fb_height   = h;
}

int graphics_ready(void) {
    return fb != 0;
}

void put_pixel(int x, int y, uint32_t color) {
    if ((uint32_t)x < fb_width && (uint32_t)y < fb_height)
        fb[y * fb_pitch_px + x] = color;
}

void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h; row++)
        for (int col = x; col < x + w; col++)
            put_pixel(col, row, color);
}

void draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    for (int i = x; i < x + w; i++) {
        put_pixel(i, y,         color);
        put_pixel(i, y + h - 1, color);
    }
    for (int i = y; i < y + h; i++) {
        put_pixel(x,         i, color);
        put_pixel(x + w - 1, i, color);
    }
}

void fill_screen(uint32_t color) {
    fill_rect(0, 0, fb_width, fb_height, color);
}

uint32_t fb_width_get(void)  { return fb_width; }
uint32_t fb_height_get(void) { return fb_height; }