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
bool is_allocated(memory_block_t *block)
{
    assert(block != NULL);
    return block->block_metadata & 0x1;
}

/*
 * allocate - marks a block as allocated.
 */
void allocate(memory_block_t *block)
{
    assert(block != NULL);
    block->block_metadata |= 0x1;
}

/*
 * deallocate - marks a block as unallocated.
 */
void deallocate(memory_block_t *block)
{
    assert(block != NULL);
    block->block_metadata &= ~0x1;
}

/*
 * get_size - gets the size of the block.
 */
size_t get_size(memory_block_t *block)
{
    assert(block != NULL);
    return block->block_metadata & ~(ALIGNMENT - 1);
}

/*
 * get_next - gets the next block.
 */
memory_block_t *get_next(memory_block_t *block)
{
    assert(block != NULL);
    return block->next;
}

/*
 * put_block - puts a block struct into memory at the specified address.
 * Initializes the size and allocated fields, along with NUlling out the next
 * field.
 */
void put_block(memory_block_t *block, size_t size, bool alloc)
{
    assert(block != NULL);
    assert(size % ALIGNMENT == 0);
    assert(alloc >> 1 == 0);
    block->block_metadata = size | alloc;
    block->next = NULL;
}

/*
 * get_payload - gets the payload of the block.
 */
void *get_payload(memory_block_t *block)
{
    assert(block != NULL);
    return (void *)(block + 1);
}

/*
 * get_block - given a payload, returns the block.
 */
memory_block_t *get_block(void *payload)
{
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
memory_block_t *find(size_t size)
{
    // first fit search for a block satisfying the size in the list
    memory_block_t *find_block = free_head;

    while (find_block)
    {
        // checks if a block fits
        if (get_size(find_block) >= size)
        {
            return find_block;
        }
        find_block = get_next(find_block);
    }

    return find_block;
}

/*
 * extend - extends the heap if more memory is required.
 */
memory_block_t *extend(size_t size)
{
    memory_block_t *extra_block;

    // Allocate extra space every call
    extra_block = csbrk((size / PAGESIZE + 1) * PAGESIZE);
    put_block(extra_block, (size / PAGESIZE + 1) * PAGESIZE - ALIGNMENT, false);

    // if nothing given by csbrk return NULL
    if (extra_block == NULL)
    {
        return NULL;
    }

    return extra_block;
}

/*
 * perfectFit - returns the block after removing it from the free list
 */
memory_block_t *perfectFit(memory_block_t *block, size_t size)
{
    memory_block_t *block_before = free_head;
    // storing next block before it gets nulled
    memory_block_t *store_block = block->next;

    // if the block is the head and it perfectly fits move the head and allocates
    if (block == free_head)
    {
        free_head = free_head->next;
        put_block(block, size, true);
        return block;
    }

    // find the position is being removed and remove it from the list and allocates
    while (get_next(block_before) && get_next(block_before) != block)
    {
        block_before = get_next(block_before);
    }
    block_before->next = store_block;
    put_block(block, size, true);
    return block;
}

/*
 * split - splits a given block in parts, one allocated, one free.
 */
memory_block_t *split(memory_block_t *block, size_t size)
{
    memory_block_t *block_before = free_head;
    memory_block_t *store_block = block->next;
    // only occurs after malloc calls

    // allocates an free entire block if it fits perfectly
    if (size == get_size(block))
    {
        return perfectFit(block, size);
    }

    memory_block_t *allocated_block = NULL;
    size_t old_size = get_size(block);

    // finds where the block before the free block being allocated
    while (get_next(block_before) && get_next(block_before) != block)
    {
        block_before = get_next(block_before);
    }

    // if the block is the free head we have to move the free heads position
    bool changeHead = false;
    if (block == free_head)
    {
        changeHead = true;
    }
    allocated_block = block;
    put_block(allocated_block, size, true);

    // put the allocated block and move the free block to leftover bit of block
    block += (size / ALIGNMENT) + 1;
    put_block(block, old_size - (size + ALIGNMENT), false);
    block->next = store_block;

    // changes the head or the next value depending on position of block in list
    if (changeHead)
    {
        free_head = block;
    }
    else
    {
        block_before->next = block;
    }
    allocated_block->next = NULL;
    return allocated_block;
}

/*
 * coalesce - coalesces a free memory block with neighbors.
 */
memory_block_t *coalesce(memory_block_t *block)
{

    // add connect blocks together if and only if the addresses line up next to each other
    if (block->next && !is_allocated(block->next) &&
        block + (get_size(block) / ALIGNMENT) + 1 == block->next)
    {

        size_t old_size = get_size(block->next);

        // storing block before put_block NULLs it out
        memory_block_t *storage_block = block->next->next;
        put_block(block, ALIGNMENT + old_size + get_size(block), false);
        block->next = storage_block;
    }

    return block;
}

/*
 * uinit - Used initialize metadata required to manage the heap
 * along with allocating initial memory.
 */
int uinit()
{
    // getting the multiplier maximized for coalescing testing case
    free_head = csbrk((PAGESIZE * 3));
    if (free_head == NULL)
    {
        return -1;
    }

    put_block(free_head, ((PAGESIZE * 3)) - ALIGNMENT, false);
    return 0;
}

/*
 * umalloc -  allocates size bytes and returns a pointer to the allocated memory.
 */
void *umalloc(size_t size)
{
    // Aligning size
    size = (size % ALIGNMENT == 0) ? size : size + (ALIGNMENT - (size % ALIGNMENT));

    // find valid block
    memory_block_t *free_block = find(size);

    if (free_block)
    {
        // split block if we have one
        free_block = split(free_block, size);
    }
    else
    {
        // extend heap when out of space
        free_block = extend(size);

        if (free_head == NULL)
        {
            // if heap currently empty
            free_head = free_block;
        }
        else if (free_head > free_block)
        {
            // if block goes in front of head
            free_block->next = free_head;
            free_head = free_block;
        }
        else
        {
            // add block to right position in the free list
            memory_block_t *block_position = free_head;
            while (get_next(block_position) && get_next(block_position) < free_block)
            {
                block_position = get_next(block_position);
            }
            free_block->next = block_position->next;
            block_position->next = free_block;
        }
        // take the space needed to allocate block
        free_block = split(free_block, size);
    }
    // returns payload
    return free_block + 1;
}

/*
 * ufree -  frees the memory space pointed to by ptr, which must have been called
 * by a previous call to malloc.
 */
void ufree(void *ptr)
{
    memory_block_t *free_block = get_block(ptr);

    // only free if allocated already
    if (is_allocated(free_block))
    {
        deallocate(free_block);

        // add back to free list
        if (free_head == NULL)
        {
            // add to head if head empty
            free_head = free_block;
        }
        else if (free_head > free_block)
        {
            // add in front of head when earlier address and check to coalesce with old head
            free_block->next = free_head;
            free_head = free_block;
            coalesce(free_block);
        }
        else
        {
            // adding the block back into the middle of the list or end & coalesce adjacent blocks
            memory_block_t *block_position = free_head;
            while (get_next(block_position) && get_next(block_position) < free_block)
            {
                block_position = get_next(block_position);
            }
            free_block->next = block_position->next;
            block_position->next = free_block;
            coalesce(free_block);
            coalesce(block_position);
        }
    }
}
