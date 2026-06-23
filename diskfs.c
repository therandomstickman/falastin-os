#include "diskfs.h"
#include "screen.h"
#include "ata.h"

#define MAX_FILES     64
#define MAX_FILE_SIZE 8192   // 8KB per file is plenty for now
#define MAX_FILENAME 32
#define FS_MAGIC   0x464C5354
#define FS_VERSION 1
#define FS_START_LBA 200

typedef struct {
    char name[MAX_FILENAME];
    char data[MAX_FILE_SIZE];
    int size;
    int used;
    int is_dir;
    int parent;
} File;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t reserved[30];
} __attribute__((packed)) Superblock;

static File files[MAX_FILES];
static int current_dir = 0;

// Forward declarations
void diskfs_save(void);
static int  diskfs_load(void);

static void print_int(int num) {
    char buffer[32];
    int i = 0;
    if (num == 0) { put_char('0'); return; }
    while (num > 0) { buffer[i++] = '0' + (num % 10); num /= 10; }
    for (int j = i - 1; j >= 0; j--) put_char(buffer[j]);
}

static int find_file(const char* name, int parent) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && files[i].parent == parent) {
            int match = 1;
            for (int j = 0; j < MAX_FILENAME; j++) {
                if (name[j] != files[i].name[j]) { match = 0; break; }
                if (name[j] == '\0' && files[i].name[j] == '\0') break;
            }
            if (match) return i;
        }
    }
    return -1;
}

static int find_free(void) {
    for (int i = 0; i < MAX_FILES; i++)
        if (!files[i].used) return i;
    return -1;
}

void diskfs_save(void) {
    if (!ata_drive_present()) return;

    uint8_t sector[512];
    int i, j;

    // Write superblock
    for (i = 0; i < 512; i++) sector[i] = 0;
    Superblock* sb = (Superblock*)sector;
    sb->magic   = FS_MAGIC;
    sb->version = FS_VERSION;
    if (ata_write_sector(FS_START_LBA, sector) != 0) {
        print("FS save: superblock write failed\n");
        return;
    }

    // Write file array as raw sectors
    uint8_t* raw = (uint8_t*)files;
    int total_bytes   = sizeof(files);
    int total_sectors = (total_bytes + 511) / 512;

    for (i = 0; i < total_sectors; i++) {
        for (j = 0; j < 512; j++) {
            int idx = i * 512 + j;
            sector[j] = (idx < total_bytes) ? raw[idx] : 0;
        }
        if (ata_write_sector(FS_START_LBA + 1 + i, sector) != 0) {
            print("FS save: data write failed\n");
            return;
        }
    }

    print("Filesystem saved.\n");
}

static int diskfs_load(void) {
    uint8_t sector[512];

    // Check superblock
    if (ata_read_sector(FS_START_LBA, sector) != 0) return 0;
    Superblock* sb = (Superblock*)sector;
    if (sb->magic != FS_MAGIC || sb->version != FS_VERSION) return 0;

    // Load file array
    uint8_t* raw = (uint8_t*)files;
    int total_bytes   = sizeof(files);
    int total_sectors = (total_bytes + 511) / 512;

    for (int i = 0; i < total_sectors; i++) {
        if (ata_read_sector(FS_START_LBA + 1 + i, sector) != 0) return 0;
        for (int j = 0; j < 512; j++) {
            int idx = i * 512 + j;
            if (idx < total_bytes) raw[idx] = sector[j];
        }
    }

    return 1;
}

void diskfs_init(void) {
    print("Initializing filesystem...\n");

    if (ata_drive_present() && diskfs_load()) {
        print("Loaded filesystem from disk.\n");
        return;
    }

    // Fresh init
    for (int i = 0; i < MAX_FILES; i++) {
        files[i].used    = 0;
        files[i].size    = 0;
        files[i].name[0] = '\0';
        files[i].is_dir  = 0;
        files[i].parent  = -1;
    }
    files[0].used    = 1;
    files[0].is_dir  = 1;
    files[0].parent  = -1;
    files[0].name[0] = '/';
    files[0].name[1] = '\0';
    current_dir = 0;

    if (ata_drive_present()) {
        print("Fresh filesystem, saving to disk.\n");
        diskfs_save();
    } else {
        print("No disk, using RAM only.\n");
    }
}

int diskfs_create(const char* name) {
    if (find_file(name, current_dir) != -1) { print("File already exists\n"); return -1; }
    int slot = find_free();
    if (slot == -1) { print("No free slots\n"); return -1; }
    for (int i = 0; i < MAX_FILENAME - 1 && name[i]; i++)
        files[slot].name[i] = name[i];
    files[slot].name[MAX_FILENAME-1] = '\0';
    files[slot].used    = 1;
    files[slot].is_dir  = 0;
    files[slot].parent  = current_dir;
    files[slot].size    = 0;
    print("Created: "); print(name); print("\n");
    diskfs_save();
    return 0;
}

int diskfs_write(const char* name, const char* data, int size) {
    int idx = find_file(name, current_dir);
    if (idx == -1) {
        if (diskfs_create(name) != 0) return -1;
        idx = find_file(name, current_dir);
    }
    if (size > MAX_FILE_SIZE) { print("File too large\n"); return -1; }
    for (int i = 0; i < size; i++) files[idx].data[i] = data[i];
    files[idx].size = size;
    print("Written "); print_int(size); print(" bytes\n");
    diskfs_save();
    return 0;
}

int diskfs_read(const char* name, char* buffer, int size) {
    int idx = find_file(name, current_dir);
    if (idx == -1) { print("File not found\n"); return -1; }
    int read_size = (size < files[idx].size) ? size : files[idx].size;
    for (int i = 0; i < read_size; i++) buffer[i] = files[idx].data[i];
    return read_size;
}

void diskfs_list(const char* path) {
    (void)path;
    int found = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && files[i].parent == current_dir) {
            found = 1;
            print(files[i].is_dir ? "  [DIR]  " : "  [FILE] ");
            print(files[i].name);
            if (!files[i].is_dir) {
                print(" ("); print_int(files[i].size); print(" bytes)");
            }
            print("\n");
        }
    }
    if (!found) print("  (empty)\n");
}

int diskfs_delete(const char* name) {
    int idx = find_file(name, current_dir);
    if (idx == -1) { print("File not found\n"); return -1; }
    files[idx].used = 0;
    print("Deleted: "); print(name); print("\n");
    diskfs_save();
    return 0;
}

int diskfs_mkdir(const char* name) {
    if (find_file(name, current_dir) != -1) { print("Directory already exists\n"); return -1; }
    int slot = find_free();
    if (slot == -1) return -1;
    for (int i = 0; i < MAX_FILENAME - 1 && name[i]; i++)
        files[slot].name[i] = name[i];
    files[slot].name[MAX_FILENAME-1] = '\0';
    files[slot].used    = 1;
    files[slot].is_dir  = 1;
    files[slot].parent  = current_dir;
    files[slot].size    = 0;
    print("Directory created: "); print(name); print("\n");
    diskfs_save();
    return 0;
}

int diskfs_cd(const char* path) {
    if (path[0] == '/' && path[1] == '\0') {
        current_dir = 0;
        return 0;
    }

    // Handle ".." - go to parent
    if (path[0] == '.' && path[1] == '.' && path[2] == '\0') {
        if (current_dir == 0) return 0;  // already at root
        int parent = files[current_dir].parent;
        if (parent >= 0) {
            current_dir = parent;
            return 0;
        }
        return -1;
    }

    int idx = find_file(path, current_dir);
    if (idx != -1 && files[idx].is_dir) {
        current_dir = idx;
        return 0;
    }
    print("Directory not found\n");
    return -1;
}

void diskfs_pwd(void)  { print("/\n"); }

int diskfs_exists(const char* name) {
    return find_file(name, current_dir) != -1;
}

int diskfs_load_program(const char* name, uint8_t* buffer, int max_size) {
    return diskfs_read(name, (char*)buffer, max_size);
}

int diskfs_get_entries(Entry* out, int max) {
    int count = 0;
    for (int i = 0; i < MAX_FILES && count < max; i++) {
        if (files[i].used && files[i].parent == current_dir) {
            for (int j = 0; j < 32; j++)
                out[count].name[j] = files[i].name[j];
            out[count].is_dir = files[i].is_dir;
            out[count].size   = files[i].size;
            count++;
        }
    }
    return count;
}

void diskfs_debug(void) {
    print("\n=== Filesystem Debug ===\n");
    print("ATA present: "); print(ata_drive_present() ? "yes" : "no"); print("\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            print("Slot "); print_int(i); print(": ");
            print(files[i].is_dir ? "[DIR] " : "[FILE] ");
            print(files[i].name);
            print(" parent="); print_int(files[i].parent);
            if (!files[i].is_dir) { print(" size="); print_int(files[i].size); }
            print("\n");
        }
    }
}

