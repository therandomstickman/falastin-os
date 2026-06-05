#ifndef DISKFS_H
#define DISKFS_H

#include <stdint.h>

void diskfs_init(void);
int diskfs_create(const char* name);
int diskfs_write(const char* name, const char* data, int size);
int diskfs_read(const char* name, char* buffer, int size);
void diskfs_list(void);
int diskfs_delete(const char* name);
int diskfs_exists(const char* name);

#endif