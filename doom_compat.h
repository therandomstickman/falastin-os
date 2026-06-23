#ifndef DOOM_COMPAT_H
#define DOOM_COMPAT_H

#include <stdint.h>

// File I/O functions DOOM expects
int doom_open(const char* path, int flags);
int doom_read(int fd, void* buf, int count);
int doom_write(int fd, const void* buf, int count);
int doom_close(int fd);
int doom_lseek(int fd, int offset, int whence);

// Video functions
void doom_video_init(void);
void doom_draw_pixel(int x, int y, uint32_t color);
void doom_draw_rect(int x, int y, int w, int h, uint32_t color);
void doom_flush(void);
void doom_draw_8bit_buffer(uint8_t* buffer, int width, int height);

// Input functions
int doom_get_key(void);
void doom_get_mouse(int* dx, int* dy, int* buttons);

// Timer functions
uint32_t doom_get_ticks(void);
void doom_sleep(uint32_t ms);

// Entry point for DOOM
void doom_run(void);

#endif