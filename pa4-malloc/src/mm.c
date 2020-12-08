#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/* The standard allocator interface from stdlib.h.  These are the
 * functions you must implement, more information on each function is
 * found below. They are declared here in case you want to use one
 * function in the implementation of another. */
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

/* When requesting memory from the OS using sbrk(), request it in
 * increments of CHUNK_SIZE. */
#define CHUNK_SIZE (1<<12)
#define NUM_POOLS 12
#define MAX_NON_BULK_MEMORY_SIZE 4088
#define HEADER_SIZE 8
// Freelist node structure

typedef struct FreeListNode FreeListNode;
typedef struct TableNode TableNode;

struct FreeListNode{
    uint64_t header;
    FreeListNode *next;
};

struct TableNode{
    FreeListNode *start;
};

#define FREE_LIST_NODE_SIZE sizeof(FreeListNode)
#define TABLE_NODE_SIZE sizeof(TableNode)

TableNode *freeListTable;

void mm_init()
{
    // fprintf(stderr, "mm_init started\n");
    freeListTable = sbrk(CHUNK_SIZE);
    for(int i = 0; i < NUM_POOLS; i++)
    {
        freeListTable[i].start = NULL;
    }
    // fprintf(stderr, "mm_init ended\n");
}

uint64_t get_header( void *ptr)
{
    // fprintf(stderr, "get_header started with ptr: %p\n", ptr);
    // fprintf(stderr, "get_header ended, returning header: %lu\n", ((FreeListNode*)ptr)->header);
    return ((FreeListNode*)ptr)->header;
}

uint64_t get_size(void *ptr)
{
    // fprintf(stderr, "get_size started\n");
    uint64_t header = get_header(ptr);
    uint64_t size_mask = ~0x7ULL;
    // fprintf(stderr, "get_size ended\n");
    return header & size_mask;
}

int get_allocated_field(void *ptr)
{
    // fprintf(stderr, "get_allocated_field started\n");
    uint64_t header = get_header(ptr);
    uint64_t allocated_mask = 0x1ULL;
    // fprintf(stderr, "get_allocated_field ended\n");
    return header & allocated_mask;
}

void mark_free(void *ptr)
{
    // fprintf(stderr, "mark_free started with ptr: %p\n", ptr);
    uint64_t header = get_header(ptr);
    uint64_t allocated_mask = ~0x1ULL;
    ((FreeListNode*)ptr)->header = header & allocated_mask;
    // fprintf(stderr, "mark_free ended\n");
}


int get_list_block_size(int list_idx)
{
    // fprintf(stderr, "get_list_block_size started\n");
    // fprintf(stderr, "get_list_block_size ended\n");
    return 1 << list_idx;
}

void *get_list_next_node(int list_idx, void* curr_node)
{
    // fprintf(stderr, "get_list_next_node started\n");
    // fprintf(stderr, "get_list_next_node ended\n");
    return curr_node + get_list_block_size(list_idx);
}

int get_num_chunks(int list_idx)
{
    // fprintf(stderr, "get_num_chunks started\n");
    // fprintf(stderr, "get_num_chunks ended\n");
    return CHUNK_SIZE / get_list_block_size(list_idx);
}

void insert_in_list(int list_idx, void *ptr)
{
    // fprintf(stderr, "insert_in_list started\n");    
    
    if( freeListTable[list_idx].start == NULL)
    {
        freeListTable[list_idx].start = ptr;
        FreeListNode *listNode = (FreeListNode*)ptr;
        listNode->header = 0ULL;    // initialize header with allocated bit
        listNode->next = NULL;
        // fprintf(stderr, "insert_in_list ended\n");
        return;
    }
    else
    {
        void *currHead = freeListTable[list_idx].start;
        FreeListNode *listNode = (FreeListNode*)ptr;
        listNode->header = 0ULL;    // initialize header with allocated bit
        listNode->next = currHead;
        freeListTable[list_idx].start = listNode;
    }
    // fprintf(stderr, "insert_in_list ended\n");
}

int fill_list(int list_idx)
{
    // fprintf(stderr, "fill_list started\n");
    void *allocated_memory_ptr = sbrk(CHUNK_SIZE);
    // fprintf(stderr, "memory retrieved from OS, startaddr = %p\n", allocated_memory_ptr);
    
    if( allocated_memory_ptr == (void *)-1)
    {
        // fprintf(stderr, "Error occured in fill list..\n");
        return -1;
    }

    int num_chunks = get_num_chunks(list_idx);
    void *next_block_addr = allocated_memory_ptr;
    int list_block_size = get_list_block_size(list_idx);

    for(int i = 0; i < num_chunks; i++)
    {
        insert_in_list(list_idx, next_block_addr);
        next_block_addr = next_block_addr + list_block_size;
    }

    // fprintf(stderr, "fill_list ended\n");
    return 0;
}

void *retrieve_from_list(int list_idx, int reqd_size)
{
    // fprintf(stderr, "retrieve_from_list started, list_idx = %d, and reqd_size: %d\n", list_idx, reqd_size);
    if(freeListTable == NULL)
    {
        mm_init();
    }
    if(freeListTable[list_idx].start == NULL)
    {
        int status = fill_list(list_idx);
        if(status == -1)
        {
            // fprintf(stderr, "Error occured in retrieve from list, fill_list failed\n");
            return NULL;
        }
    }

    FreeListNode *head = freeListTable[list_idx].start;
    freeListTable[list_idx].start = freeListTable[list_idx].start->next;
    head->header = ((uint64_t)reqd_size) | 1ULL;
    // fprintf(stderr, "returning head : %p with header: %lu, actual_head: %p\n", (void*)head + HEADER_SIZE, head->header, head);
    // fprintf(stderr, "retrieve_from_list ended\n");
    return (void*)head + HEADER_SIZE;
}

/*
 * This function, defined in bulk.c, allocates a contiguous memory
 * region of at least size bytes.  It MAY NOT BE USED as the allocator
 * for pool-allocated regions.  Memory allocated using bulk_alloc()
 * must be freed by bulk_free().
 *
 * This function will return NULL on failure.
 */
extern void *bulk_alloc(size_t size);

/*
 * This function is also defined in bulk.c, and it frees an allocation
 * created with bulk_alloc().  Note that the pointer passed to this
 * function MUST have been returned by bulk_alloc(), and the size MUST
 * be the same as the size passed to bulk_alloc() when that memory was
 * allocated.  Any other usage is likely to fail, and may crash your
 * program.
 */
extern void bulk_free(void *ptr, size_t size);

/*
 * This function computes the log base 2 of the allocation block size
 * for a given allocation.  To find the allocation block size from the
 * result of this function, use 1 << block_size(x).
 *
 * Note that its results are NOT meaningful for any
 * size > 4088!
 *
 * You do NOT need to understand how this function works.  If you are
 * curious, see the gcc info page and search for __builtin_clz; it
 * basically counts the number of leading binary zeroes in the value
 * passed as its argument.
 */
static inline __attribute__((unused)) int block_index(size_t x) {
    if (x <= 8) {
        return 5;
    } else {
        return 32 - __builtin_clz((unsigned int)x + 7);
    }
}

/*
 * You must implement malloc().  Your implementation of malloc() must be
 * the multi-pool allocator described in the project handout.
 */
void *malloc(size_t size) {
    // fprintf(stderr, "malloc started, requested size = %lu\n", size);
    if(size < MAX_NON_BULK_MEMORY_SIZE)
    {
        int list_idx = block_index(size);        
        void *ret = retrieve_from_list(list_idx, size);
        // fprintf(stderr, "malloc ended\n");
        return ret;
    }

    // fprintf(stderr, "bulk malloc ended\n");
    void *bulk_ptr = bulk_alloc(size + HEADER_SIZE);
    *((uint64_t*)bulk_ptr) = size;
    return bulk_ptr + HEADER_SIZE;
}

/*
 * You must also implement calloc().  It should create allocations
 * compatible with those created by malloc().  In particular, any
 * allocations of a total size <= 4088 bytes must be pool allocated,
 * while larger allocations must use the bulk allocator.
 *
 * calloc() (see man 3 calloc) returns a cleared allocation large enough
 * to hold nmemb elements of size size.  It is cleared by setting every
 * byte of the allocation to 0.  You should use the function memset()
 * for this (see man 3 memset).
 */
void *calloc(size_t nmemb, size_t size) {
    // fprintf(stderr, "calloc started, req nmemb = %lu, of size: %lu each\n", nmemb, size);
    void *ptr;

    if(size < MAX_NON_BULK_MEMORY_SIZE)
    {
        ptr = malloc(nmemb * size);
    }
    else
    {
        ptr = bulk_alloc(nmemb * size);
    }
    
    memset(ptr, 0, nmemb * size);
    // fprintf(stderr, "calloc ended\n");
    return ptr;
}

/*
 * You must also implement realloc().  It should create allocations
 * compatible with those created by malloc(), honoring the pool
 * alocation and bulk allocation rules.  It must move data from the
 * previously-allocated block to the newly-allocated block if it cannot
 * resize the given block directly.  See man 3 realloc for more
 * information on what this means.
 *
 * It is not possible to implement realloc() using bulk_alloc() without
 * additional metadata, so the given code is NOT a working
 * implementation!
 */
void *realloc(void *ptr, size_t size)
{    
    // fprintf(stderr, "realloc started with ptr: %p and new_size: %lu\n", ptr, size);
    if(ptr == NULL)
    {
        return malloc(size);
    }
    void *ptr_with_header = ptr - HEADER_SIZE;
    // fprintf(stderr, "ptr_with_header: %p\n", ptr_with_header);
    
    uint64_t used_size = get_size(ptr_with_header);
    int list_idx = block_index(used_size);
    uint64_t avail_size = get_list_block_size(list_idx) - HEADER_SIZE;

    if(size < avail_size)
    {
        (*(uint64_t*)ptr_with_header) = size | 1ULL;
        return ptr;
    }

    void *new_ptr = malloc(size);
    memcpy(new_ptr, ptr, used_size);
    // ffprintf(stderr, stderr, "Realloc is not implemented!\n");
    // fprintf(stderr, "relloc ended\n");
    return new_ptr;
}

/*
 * You should implement a free() that can successfully free a region of
 * memory allocated by any of the above allocation routines, whether it
 * is a pool- or bulk-allocated region.
 *
 * The given implementation does nothing.
 */
void free(void *ptr)
{
    if(ptr == NULL)
    {
        return;
    }
    // fprintf(stderr, "free started with ptr: %p\n", ptr);
    void *ptr_with_header = ptr - HEADER_SIZE;
    uint64_t used_size = get_size(ptr_with_header);

    if(used_size < MAX_NON_BULK_MEMORY_SIZE)
    {
        int list_idx = block_index(used_size);
        mark_free(ptr_with_header);
        insert_in_list(list_idx, ptr_with_header);
        // fprintf(stderr, "free ended\n");
    }    
    else
    {
        bulk_free(ptr_with_header, used_size + HEADER_SIZE);
        // fprintf(stderr, "bulk free ended\n");
    }    
    return;
}
