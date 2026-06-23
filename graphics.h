#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

void graphics_init(uint32_t addr, uint32_t pitch, uint32_t w, uint32_t h, uint8_t bpp);
int  graphics_ready(void);
void put_pixel(int x, int y, uint32_t color);
void fill_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_outline(int x, int y, int w, int h, uint32_t color);
void fill_screen(uint32_t color);
uint32_t fb_width_get(void);
uint32_t fb_height_get(void);
uint32_t* fb_get(void);

#endif