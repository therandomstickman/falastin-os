#include "diskfs.h"
#include "screen.h"

#define MAX_FILES 32
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 4096

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
    int is_dir;
    int parent;
} File;

static File files[MAX_FILES];
static int current_dir = 0;

static void print_int(int num)
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

static int find_file(const char* name, int parent)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && files[i].parent == parent) {
            int match = 1;
            for (int j = 0; j < MAX_FILENAME; j++) {
                if (name[j] != files[i].name[j]) {
                    match = 0;
                    break;
                }
                if (name[j] == '\0' && files[i].name[j] == '\0') break;
            }
            if (match) return i;
        }
    }
    return -1;
}

static int find_free(void)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) return i;
    }
    return -1;
}

void diskfs_init(void)
{
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used = 0;
        files[i].size = 0;
        files[i].name[0] = '\0';
        files[i].is_dir = 0;
        files[i].parent = -1;
    }
    
    // Root directory
    files[0].used = 1;
    files[0].is_dir = 1;
    files[0].parent = -1;
    files[0].name[0] = '/';
    files[0].name[1] = '\0';
    current_dir = 0;
    
    print("RAM filesystem initialized\n");
    print("Use 'save' and 'load' commands for persistence via QEMU monitor\n");
}

int diskfs_create(const char* name)
{
    if (find_file(name, current_dir) != -1) {
        print("File already exists\n");
        return -1;
    }
    
    int slot = find_free();
    if (slot == -1) {
        print("No free slots\n");
        return -1;
    }
    
    for (int i = 0; i < MAX_FILENAME - 1 && name[i]; i++) {
        files[slot].name[i] = name[i];
    }
    files[slot].used = 1;
    files[slot].is_dir = 0;
    files[slot].parent = current_dir;
    files[slot].size = 0;
    
    print("Created: ");
    print(name);
    print("\n");
    return 0;
}

int diskfs_mkdir(const char* name)
{
    if (find_file(name, current_dir) != -1) {
        print("Directory already exists\n");
        return -1;
    }
    
    int slot = find_free();
    if (slot == -1) {
        print("No free slots\n");
        return -1;
    }
    
    for (int i = 0; i < MAX_FILENAME - 1 && name[i]; i++) {
        files[slot].name[i] = name[i];
    }
    files[slot].used = 1;
    files[slot].is_dir = 1;
    files[slot].parent = current_dir;
    files[slot].size = 0;
    
    print("Directory created: ");
    print(name);
    print("\n");
    return 0;
}

int diskfs_write(const char* name, const char* data, int size)
{
    int idx = find_file(name, current_dir);
    if (idx == -1) {
        if (diskfs_create(name) != 0) return -1;
        idx = find_file(name, current_dir);
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

int diskfs_read(const char* name, char* buffer, int size)
{
    int idx = find_file(name, current_dir);
    if (idx == -1) {
        print("File not found\n");
        return -1;
    }
    
    int read_size = size;
    if (read_size > files[idx].size) read_size = files[idx].size;
    
    for (int i = 0; i < read_size; i++) {
        buffer[i] = files[idx].data[i];
    }
    return read_size;
}

void diskfs_list(const char* path)
{
    (void)path;
    int found = 0;
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && files[i].parent == current_dir) {
            found = 1;
            if (files[i].is_dir) {
                print("  [DIR]  ");
            } else {
                print("  [FILE] ");
            }
            print(files[i].name);
            if (!files[i].is_dir) {
                print(" (");
                print_int(files[i].size);
                print(" bytes)");
            }
            print("\n");
        }
    }
    if (!found) {
        print("  (empty)\n");
    }
}

int diskfs_delete(const char* name)
{
    int idx = find_file(name, current_dir);
    if (idx == -1) {
        print("File not found\n");
        return -1;
    }
    
    files[idx].used = 0;
    print("Deleted: ");
    print(name);
    print("\n");
    return 0;
}

int diskfs_cd(const char* path)
{
    if (path[0] == '/' && path[1] == '\0') {
        current_dir = 0;
        return 0;
    }
    
    int idx = find_file(path, current_dir);
    if (idx != -1 && files[idx].is_dir) {
        current_dir = idx;
        return 0;
    }
    
    print("Directory not found\n");
    return -1;
}

void diskfs_pwd(void)
{
    print("/\n");
}

int diskfs_exists(const char* name)
{
    return find_file(name, current_dir) != -1;
}

int diskfs_load_program(const char* name, uint8_t* buffer, int max_size)
{
    return diskfs_read(name, (char*)buffer, max_size);
}

void diskfs_debug(void)
{
    print("\n=== RAM Filesystem Debug ===\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            print("Slot ");
            print_int(i);
            print(": ");
            if (files[i].is_dir) print("[DIR] ");
            else print("[FILE] ");
            print(files[i].name);
            print(" parent=");
            print_int(files[i].parent);
            if (!files[i].is_dir) {
                print(" size=");
                print_int(files[i].size);
            }
            print("\n");
        }
    }
}