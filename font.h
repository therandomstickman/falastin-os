#ifndef FONT_H
#define FONT_H

#include <stdint.h>

void font_init(void);
void font_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void font_draw_string(int x, int y, const char* str, uint32_t fg, uint32_t bg);
int  font_char_width(void);
int  font_char_height(void);

#endif