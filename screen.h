#ifndef SCREEN_H
#define SCREEN_H

void clear_screen(void);
void print(const char* str);
void put_char(char c);
void screen_put_char_at(int row, int col, char c, char color);
char screen_get_char_at(int row, int col);
void screen_set_terminal_mode(int enabled);
void screen_set_terminal_output(int enabled);
void print_fmt(const char* fmt, ...);

// New graphics functions
void init_graphics(void);

#endif