#include "screen.h"
#include "cursor.h"

static unsigned short* video = (unsigned short*)0xB8000;
static int current_row = 0;
static int current_col = 0;

void screen_put_char_at(int row, int col, char ch, char color)
{
    if (row >= 0 && row < 25 && col >= 0 && col < 80) {
        video[row * 80 + col] = (color << 8) | ch;
    }
}

char screen_get_char_at(int row, int col)
{
    if (row >= 0 && row < 25 && col >= 0 && col < 80) {
        return video[row * 80 + col] & 0xFF;
    }
    return 0;
}

void get_cursor_position(int* row, int* col)
{
    *row = current_row;
    *col = current_col;
}

static void scroll(void)
{
    if (current_row < 25)
        return;
    
    for (int i = 0; i < 24 * 80; i++)
        video[i] = video[i + 80];
    
    for (int i = 24 * 80; i < 25 * 80; i++)
        video[i] = 0x0720;
    
    current_row = 24;
}

void put_char(char c)
{
    if (c == '\b') {
        if (current_col > 0) {
            current_col--;
            video[current_row * 80 + current_col] = 0x0720;
            update_cursor(current_row, current_col);
        }
        return;
    }
    
    if (c == '\n') {
        current_row++;
        current_col = 0;
        scroll();
        update_cursor(current_row, current_col);
        return;
    }
    
    video[current_row * 80 + current_col] = (0x07 << 8) | c;
    current_col++;
    
    if (current_col >= 80) {
        current_col = 0;
        current_row++;
        scroll();
    }
    
    update_cursor(current_row, current_col);
}

void print(const char* str)
{
    while (*str)
        put_char(*str++);
}

void clear_screen(void)
{
    for (int i = 0; i < 80 * 25; i++)
        video[i] = 0x0720;
    current_row = 0;
    current_col = 0;
    update_cursor(current_row, current_col);
}