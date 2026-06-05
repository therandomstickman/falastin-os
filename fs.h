#ifndef FS_H
#define FS_H

#include <stdint.h>

void fs_init(void);
int fs_create(const char* name);
int fs_write(const char* name, const char* data, int size);
int fs_read(const char* name, char* buffer, int size);
void fs_list(void);
int fs_delete(const char* name);
int strcmp_fs(const char* a, const char* b);
void print_int(int num);
void fs_debug(void);
int fs_load_program(const char* name, uint8_t* buffer, int max_size);

#endif