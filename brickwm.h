#ifndef BRICKWM_H
#define BRICKWM_H

typedef struct {
    int x, y, w, h;
    char title[32];
    int active;
    void (*draw)(int x, int y, int w, int h);
    void (*keypress)(char c);
    int use_count;
} Window;

void brickwm_init(void);
int brickwm_create_window(const char* title, int x, int y, int w, int h, 
                          void (*draw)(int, int, int, int),
                          void (*keypress)(char));
void brickwm_run(void);
void brickwm_close_window(int handle);
int brickwm_window_count(void);

#endif