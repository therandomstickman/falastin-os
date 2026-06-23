#include "malloc.h"
#include "screen.h"
#include "libc.h"
#include "fs.h"

// Heap sits at 4MB, grows upward, 1MB total
// (well below kernel at 1MB, above any BIOS stuff)
#define HEAP_START  0x800000   // 8MB, safely past everything
#define HEAP_SIZE   0x400000   // 4MB heap // 1MB

// Block header — sits immediately before every allocation
typedef struct Block {
    size_t       size;   // size of data region (not including header)
    int          free;   // 1 = free, 0 = used
    struct Block* next;  // next block in list
    uint32_t     magic;  // 0xDEADBEEF sanity check
} Block;

#define BLOCK_MAGIC  0xDEADBEEF
#define HEADER_SIZE  sizeof(Block)
#define MIN_SPLIT    32  // don't split if remainder would be smaller than this

static Block* heap_start_ptr = 0;
static int    heap_ready     = 0;

void heap_init(void) {
    heap_start_ptr = (Block*)HEAP_START;
    heap_start_ptr->size  = HEAP_SIZE - HEADER_SIZE;
    heap_start_ptr->free  = 1;
    heap_start_ptr->next  = 0;
    heap_start_ptr->magic = BLOCK_MAGIC;
    heap_ready = 1;
}

// Merge adjacent free blocks to prevent fragmentation
static void merge_free(void) {
    Block* cur = heap_start_ptr;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += HEADER_SIZE + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

void* malloc(size_t size) {
    if (!heap_ready || size == 0) return 0;

    // Align to 8 bytes
    size = (size + 7) & ~7;

    Block* cur = heap_start_ptr;
    while (cur) {
        if (cur->magic != BLOCK_MAGIC) {
            // Heap corruption
            return 0;
        }

        if (cur->free && cur->size >= size) {
            // Split if there's enough room for another block
            if (cur->size >= size + HEADER_SIZE + MIN_SPLIT) {
                Block* newb = (Block*)((uint8_t*)cur + HEADER_SIZE + size);
                newb->size  = cur->size - size - HEADER_SIZE;
                newb->free  = 1;
                newb->next  = cur->next;
                newb->magic = BLOCK_MAGIC;
                cur->next   = newb;
                cur->size   = size;
            }
            cur->free = 0;
            return (void*)((uint8_t*)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }

    // Out of memory
    return 0;
}

void* calloc(size_t count, size_t size) {
    size_t total = count * size;
    void*  ptr   = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return 0; }

    Block* block = (Block*)((uint8_t*)ptr - HEADER_SIZE);
    if (block->size >= size) return ptr;  // already big enough

    void* newptr = malloc(size);
    if (!newptr) return 0;

    memcpy(newptr, ptr, block->size);
    free(ptr);
    return newptr;
}

void free(void* ptr) {
    if (!ptr) return;

    Block* block = (Block*)((uint8_t*)ptr - HEADER_SIZE);
    if (block->magic != BLOCK_MAGIC) return;  // bad pointer, ignore

    block->free = 1;
    merge_free();
}

void malloc_debug(void) {
    Block* cur   = heap_start_ptr;
    int    total = 0, used = 0, free_b = 0, blocks = 0;

    while (cur) {
        blocks++;
        total += cur->size;
        if (cur->free) free_b += cur->size;
        else           used   += cur->size;
        cur = cur->next;
    }

    print("=== Heap ===\n");
    print("Blocks: "); print_int(blocks); print("\n");
    print("Used:   "); print_int(used);   print(" bytes\n");
    print("Free:   "); print_int(free_b); print(" bytes\n");
}