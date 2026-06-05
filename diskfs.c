#include "diskfs.h"
#include "ata.h"
#include "screen.h"
#include "fs.h"  // Add this for print_int

#define BLOCK_SIZE 512
#define FS_START_LBA 100
#define MAX_FILES 64
#define MAX_FILENAME 32
#define MAX_FILE_SIZE (32 * BLOCK_SIZE)

typedef struct {
    char name[MAX_FILENAME];
    uint32_t size;
    uint32_t start_block;
    uint8_t used;
} __attribute__((packed)) FileEntry;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t file_count;
    FileEntry files[MAX_FILES];
} __attribute__((packed)) SuperBlock;

static SuperBlock sb;
static int diskfs_mounted = 0;

static void diskfs_save_superblock(void)
{
    uint8_t buffer[BLOCK_SIZE];
    
    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }
    
    SuperBlock* sb_ptr = (SuperBlock*)buffer;
    sb_ptr->magic = 0x464C5356;
    sb_ptr->version = 1;
    sb_ptr->file_count = sb.file_count;
    
    for (int i = 0; i < MAX_FILES; i++) {
        sb_ptr->files[i] = sb.files[i];
    }
    
    ata_write_sector(FS_START_LBA, buffer);
}

static void diskfs_load_superblock(void)
{
    uint8_t buffer[BLOCK_SIZE];
    
    if (ata_read_sector(FS_START_LBA, buffer) == 0) {
        SuperBlock* sb_ptr = (SuperBlock*)buffer;
        
        if (sb_ptr->magic == 0x464C5356) {
            sb.magic = sb_ptr->magic;
            sb.version = sb_ptr->version;
            sb.file_count = sb_ptr->file_count;
            
            for (int i = 0; i < MAX_FILES; i++) {
                sb.files[i] = sb_ptr->files[i];
            }
            print("Existing filesystem loaded\n");
        } else {
            sb.magic = 0x464C5356;
            sb.version = 1;
            sb.file_count = 0;
            
            for (int i = 0; i < MAX_FILES; i++) {
                sb.files[i].used = 0;
                sb.files[i].name[0] = '\0';
                sb.files[i].size = 0;
                sb.files[i].start_block = FS_START_LBA + 1 + i * (MAX_FILE_SIZE / BLOCK_SIZE);
            }
            
            diskfs_save_superblock();
            print("Created new filesystem\n");
        }
    } else {
        sb.magic = 0x464C5356;
        sb.version = 1;
        sb.file_count = 0;
        
        for (int i = 0; i < MAX_FILES; i++) {
            sb.files[i].used = 0;
            sb.files[i].name[0] = '\0';
            sb.files[i].size = 0;
            sb.files[i].start_block = FS_START_LBA + 1 + i * (MAX_FILE_SIZE / BLOCK_SIZE);
        }
        
        diskfs_save_superblock();
        print("Created new filesystem\n");
    }
}

void diskfs_init(void)
{
    print("Initializing Disk Filesystem...\n");
    
    if (!ata_drive_present()) {
        print("No ATA drive found! Cannot mount disk filesystem.\n");
        diskfs_mounted = 0;
        return;
    }
    
    diskfs_load_superblock();
    diskfs_mounted = 1;
    
    print("Filesystem ready, ");
    print_int(sb.file_count);
    print(" files present\n");
}

static int find_file(const char* name)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (sb.files[i].used) {
            int match = 1;
            for (int j = 0; j < MAX_FILENAME; j++) {
                if (sb.files[i].name[j] != name[j]) {
                    match = 0;
                    break;
                }
                if (name[j] == '\0' && sb.files[i].name[j] == '\0') break;
            }
            if (match) return i;
        }
    }
    return -1;
}

int diskfs_create(const char* name)
{
    if (!diskfs_mounted) return -1;
    
    if (find_file(name) != -1) {
        print("File already exists\n");
        return -1;
    }
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!sb.files[i].used) {
            int j;
            for (j = 0; j < MAX_FILENAME - 1 && name[j]; j++) {
                sb.files[i].name[j] = name[j];
            }
            sb.files[i].name[j] = '\0';
            sb.files[i].used = 1;
            sb.files[i].size = 0;
            sb.file_count++;
            
            diskfs_save_superblock();
            print("Created: ");
            print(name);
            print("\n");
            return 0;
        }
    }
    
    print("No space for new file\n");
    return -1;
}

int diskfs_write(const char* name, const char* data, int size)
{
    if (!diskfs_mounted) return -1;
    
    int idx = find_file(name);
    if (idx == -1) {
        if (diskfs_create(name) != 0) return -1;
        idx = find_file(name);
    }
    
    if (size > MAX_FILE_SIZE) {
        print("File too large\n");
        return -1;
    }
    
    print("Writing ");
    print_int(size);
    print(" bytes to ");
    print(name);
    print("\n");
    
    uint32_t current_block = sb.files[idx].start_block;
    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    for (int block = 0; block < blocks_needed; block++) {
        uint8_t buffer[BLOCK_SIZE];
        int offset = block * BLOCK_SIZE;
        int bytes_to_copy = (size - offset > BLOCK_SIZE) ? BLOCK_SIZE : size - offset;
        
        for (int i = 0; i < BLOCK_SIZE; i++) {
            buffer[i] = 0;
        }
        
        for (int i = 0; i < bytes_to_copy; i++) {
            buffer[i] = data[offset + i];
        }
        
        if (ata_write_sector(current_block + block, buffer) != 0) {
            print("Write failed!\n");
            return -1;
        }
    }
    
    sb.files[idx].size = size;
    diskfs_save_superblock();
    
    print("Write successful\n");
    return 0;
}

int diskfs_read(const char* name, char* buffer, int size)
{
    if (!diskfs_mounted) return -1;
    
    int idx = find_file(name);
    if (idx == -1) {
        print("File not found\n");
        return -1;
    }
    
    int read_size = size;
    if ((unsigned int)read_size > sb.files[idx].size) {
        read_size = sb.files[idx].size;
    }
    
    uint32_t current_block = sb.files[idx].start_block;
    int blocks_needed = (sb.files[idx].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int bytes_copied = 0;
    
    for (int block = 0; block < blocks_needed && bytes_copied < read_size; block++) {
        uint8_t block_buffer[BLOCK_SIZE];
        if (ata_read_sector(current_block + block, block_buffer) != 0) {
            print("Read failed!\n");
            return -1;
        }
        
        int bytes_to_copy = read_size - bytes_copied;
        if (bytes_to_copy > BLOCK_SIZE) {
            bytes_to_copy = BLOCK_SIZE;
        }
        
        for (int i = 0; i < bytes_to_copy; i++) {
            buffer[bytes_copied + i] = block_buffer[i];
        }
        bytes_copied += bytes_to_copy;
    }
    
    return bytes_copied;
}

void diskfs_list(void)
{
    if (!diskfs_mounted) return;
    
    int found = 0;
    print("\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (sb.files[i].used) {
            print("  ");
            print(sb.files[i].name);
            print(" (");
            print_int(sb.files[i].size);
            print(" bytes)\n");
            found = 1;
        }
    }
    
    if (!found) {
        print("  No files\n");
    }
    print("\n");
}

int diskfs_delete(const char* name)
{
    if (!diskfs_mounted) return -1;
    
    int idx = find_file(name);
    if (idx == -1) {
        print("File not found\n");
        return -1;
    }
    
    sb.files[idx].used = 0;
    sb.files[idx].size = 0;
    sb.files[idx].name[0] = '\0';
    sb.file_count--;
    
    diskfs_save_superblock();
    print("Deleted: ");
    print(name);
    print("\n");
    return 0;
}

int diskfs_exists(const char* name)
{
    return find_file(name) != -1;
}