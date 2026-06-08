#ifndef SCREEN_H
#define SCREEN_H

void clear_screen(void);
void print(const char* str);
void put_char(char c);
void screen_put_char_at(int row, int col, char c, char color);
char screen_get_char_at(int row, int col);

#endif