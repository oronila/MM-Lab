#include "umalloc.h"
#include "csbrk.h"
#include <stdio.h>
#include <assert.h>
#include "ansicolors.h"

const char author[] = ANSI_BOLD ANSI_COLOR_RED "Noor Ali na27858" ANSI_RESET;

/*
 * The following helpers can be used to interact with the memory_block_t
 * struct, they can be adjusted as necessary.
 */

// A sample pointer to the start of the free list.
memory_block_t *free_head;

/*
 * block_metadata - returns true if a block is marked as allocated.
 */
bool is_allocated(memory_block_t *block) {
    assert(block != NULL);
    return block->block_metadata & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_metadata |= 0x1;
}


/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block) {
    assert(block != NULL);
    block->block_metadata &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block) {
    assert(block != NULL);
    return block->block_metadata & ~(ALIGNMENT-1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block) {
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next 
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc) {
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_metadata = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block) {
    assert(block != NULL);
    return (void*)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload) {
    assert(payload != NULL);
    return ((memory_block_t *)payload) - 1;
}

/*
 * The following are helper functions that can be implemented to assist in your
 * design, but they are not required. 
 */

/*
 * find - finds a free block that can satisfy the umalloc request.
 */
memory_block_t *find(size_t size) {
    // STUDENT TODO (maybe coalesce blocks here if seen an opprotunity)
    memory_block_t* find_block = free_head;
    while (find_block && free_head->block_metadata < size + ALIGNMENT) {
        find_block = get_next(find_block);
    }

    return find_block;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size) {
    memory_block_t* extra_block = csbrk(size + ALIGNMENT);
    if (extra_block == NULL) {
        return NULL;
    }
    put_block(extra_block, size, false);
    //* STUDENT TODO
    return extra_block;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size) {
    memory_block_t* block_before = free_head;
    memory_block_t* store_block = block->next;
    // only occurs after malloc calls
    if (size >= block->block_metadata + ALIGNMENT) {
        while (get_next(block_before) && get_next(block_before) != block) {
            block_before = get_next(block_before);
        }
        block_before->next = store_block;
        put_block(block, size, true);
        return block;
    }
    memory_block_t* allocated_block = NULL;
    size_t old_size = block->block_metadata;
    if (block == free_head) {
        allocated_block = free_head;
        put_block(allocated_block, size, true);
        free_head += (size / ALIGNMENT) + 1;
        put_block(free_head, old_size, false);
        free_head->next = store_block;
        return allocated_block;
    } 
    
    
    while (get_next(block_before) && get_next(block_before) != block) {
        block_before = get_next(block_before);
    }

    // can probably remove the entire if statement
    allocated_block = block;
    put_block(allocated_block, size, true);
    block += (size / ALIGNMENT) + 1;
    put_block(block, old_size, false);
    block->next = store_block;
    block_before->next = block; // this is only extra line needed
    return allocated_block;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block) {
    // only occurs after free calls
    //* STUDENT TODO
    return NULL;
}



/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit() {
    free_head = csbrk(PAGESIZE * 10);
    if(free_head == NULL) {
        return -1;
    }

    put_block(free_head, PAGESIZE * 10, false);
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size) {
    size = (size % ALIGNMENT == 0) ? size: size + (ALIGNMENT - (size % ALIGNMENT));
    memory_block_t* free_block = find(size);
    if (free_block) {
        free_block = split(free_block, size);
    } else {
        free_block = extend(size);
    }
    
    return free_block + 1;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr) {
    memory_block_t* free_block = get_block(ptr);
    if (is_allocated(free_block)) {
        deallocate(free_block);
        // don't worry about coalescening right now
        
        if (free_head > free_block) {
            memory_block_t* temp_block = free_head;
            free_head = free_block;
            free_head->next = temp_block;
        } else {
            memory_block_t* block_position_before = free_head;
            memory_block_t* block_position = free_head->next;
            while (block_position && block_position < free_block) {
                block_position_before = block_position; // could simplify this using block_position->next later
                block_position = get_next(block_position);
                
            }
            if (block_position == NULL) {
                block_position_before->next = free_block;
            } else {
                memory_block_t* temp_block = block_position_before;
                block_position_before->next = free_block;
                free_block->next = temp_block;
            }
        }
    } else {
        // throw a double free error here
    }
    //* STUDENT TODO
}
