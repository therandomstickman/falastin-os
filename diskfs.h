#ifndef DISKFS_H
#define DISKFS_H

#include <stdint.h>

void diskfs_init(void);
int diskfs_create(const char* name);
int diskfs_write(const char* name, const char* data, int size);
int diskfs_read(const char* name, char* buffer, int size);
void diskfs_list(const char* path);
int diskfs_delete(const char* name);
int diskfs_mkdir(const char* name);
int diskfs_cd(const char* path);
void diskfs_pwd(void);
int diskfs_exists(const char* name);
int diskfs_load_program(const char* name, uint8_t* buffer, int max_size);
void diskfs_debug(void);

#endif