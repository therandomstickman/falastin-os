#include "fs.h"
#include "screen.h"

#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
} File;

static File files[MAX_FILES];

void print_int(int num)
{
    char buffer[32];
    int i = 0;
    if (num == 0) {
        put_char('0');
        return;
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
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = '\0';
    }
    print("Filesystem initialized\n");
}

static int find_file(const char* name)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp_fs(name, files[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

int fs_create(const char* name)
{
    if (find_file(name) != -1) {
        print("File already exists\n");
        return -1;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            int j;
            for (j = 0; j < MAX_FILENAME - 1 && name[j]; j++) {
                files[i].name[j] = name[j];
            }
            files[i].name[j] = '\0';
            files[i].used = 1;
            files[i].size = 0;
            print("Created: ");
            print(name);
            print("\n");
            return 0;
        }
    }
    print("No space for new file\n");
    return -1;
}

int fs_write(const char* name, const char* data, int size)
{
    int idx = find_file(name);
    if (idx == -1) {
        if (fs_create(name) != 0) return -1;
        idx = find_file(name);
    }
    
    if (size > MAX_FILE_SIZE) {
        print("File too large\n");
        return -1;
    }
    
    for (int i = 0; i < size; i++) {
        files[idx].data[i] = data[i];
    }
    files[idx].size = size;
    
    print("Written ");
    print_int(size);
    print(" bytes\n");
    return 0;
}

int fs_read(const char* name, char* buffer, int size)
{
    int idx = find_file(name);
    if (idx == -1) {
        print("File not found\n");
        return -1;
    }
    
    int read_size = size;
    if (read_size > files[idx].size) {
        read_size = files[idx].size;
    }
    
    for (int i = 0; i < read_size; i++) {
        buffer[i] = files[idx].data[i];
    }
    return read_size;
}

void fs_list(void)
{
    int found = 0;
    print("\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            print("  ");
            print(files[i].name);
            print(" (");
            print_int(files[i].size);
            print(" bytes)\n");
            found = 1;
        }
    }
    if (!found) {
        print("  No files\n");
    }
    print("\n");
}

int fs_delete(const char* name)
{
    int idx = find_file(name);
    if (idx == -1) {
        print("File not found\n");
        return -1;
    }
    
    files[idx].used = 0;
    files[idx].size = 0;
    files[idx].name[0] = '\0';
    print("Deleted: ");
    print(name);
    print("\n");
    return 0;
}

void fs_debug(void)
{
    print("\n=== Debug ===\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            print("File: ");
            print(files[i].name);
            print(" size=");
            print_int(files[i].size);
            print("\n");
        }
    }
}

int fs_load_program(const char* name, uint8_t* buffer, int max_size)
{
    int idx = find_file(name);
    if (idx == -1) return -1;
    if (files[idx].size > max_size) return -1;
    
    for (int i = 0; i < files[idx].size; i++) {
        buffer[i] = files[idx].data[i];
    }
    return files[idx].size;
}