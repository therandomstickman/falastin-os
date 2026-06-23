#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

void mouse_init(void);
void irq12_c(void);
int  mouse_x(void);
int  mouse_y(void);
int  mouse_buttons(void);  // bit 0 = left, bit 1 = right, bit 2 = middle

#endif