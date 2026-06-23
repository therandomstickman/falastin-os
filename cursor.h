#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

void update_cursor(int row, int col);
void enable_cursor(void);
void disable_cursor(void);

#endif