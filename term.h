#ifndef TERM_H
#define TERM_H

void terminal_run(void);
void term_draw(int x, int y, int w, int h);
void term_handle_key(char c);
void term_set_window_handle(int handle);

#endif