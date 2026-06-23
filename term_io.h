#ifndef TERM_IO_H
#define TERM_IO_H

extern char term_output[30][75];
extern int term_cursor_row;
extern int term_cursor_col;

void term_io_init(void);
void term_io_putchar(char c);
void term_io_print(const char* str);
void term_io_render(int x, int y);
int term_io_is_active(void);
void term_io_deactivate(void);
void term_io_set_needs_redraw(void);
void term_io_force_redraw(void);

#endif