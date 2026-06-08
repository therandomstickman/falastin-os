#include "fs.h"
#include "diskfs.h"
#include "screen.h"

void print_int(int num)
{
    char buffer[32];
    int i = 0;
    if (num == 0) {
        put_char('0');
        return;
    }
    if (num < 0) {
        put_char('-');
        num = -num;
    }
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        put_char(buffer[j]);
    }
}

int strcmp_fs(const char* a, const char* b)
{
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return *a - *b;
}

void fs_init(void)
{
    diskfs_init();
}

int fs_create(const char* name)
{
    return diskfs_create(name);
}

int fs_mkdir(const char* name)
{
    return diskfs_mkdir(name);
}

int fs_write(const char* name, const char* data, int size)
{
    return diskfs_write(name, data, size);
}

int fs_read(const char* name, char* buffer, int size)
{
    return diskfs_read(name, buffer, size);
}

void fs_list(const char* path)
{
    diskfs_list(path);
}

int fs_delete(const char* name)
{
    return diskfs_delete(name);
}

int fs_cd(const char* path)
{
    return diskfs_cd(path);
}

void fs_pwd(void)
{
    diskfs_pwd();
}

int fs_exists(const char* name)
{
    return diskfs_exists(name);
}

int fs_load_program(const char* name, uint8_t* buffer, int max_size)
{
    return diskfs_load_program(name, buffer, max_size);
}

void fs_debug(void)
{
    diskfs_debug();
}