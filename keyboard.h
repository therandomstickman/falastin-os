#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_UP     1
#define KEY_DOWN   2
#define KEY_LEFT   3
#define KEY_RIGHT  4
#define KEY_DELETE 127

char keyboard_getchar(void);
void irq1_c(void);

#endif