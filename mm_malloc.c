/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines. Remove this comment and provide
 * a summary of your allocator's design here.
 */

#include "mm_alloc.h"

#include <stdlib.h>

#include <sys/types.h>
#include<unistd.h>

//It is often required that pointers be aligned to the integer size(pointer size).
// A macro will be used for that.
#define align4(x) (((((x)-1)>>2)<<2)+4)

// Global variable base - is used by the function "find_block".
// So we define it.
void *base = NULL;

// Malloc function allocates(reserves) memory chunks.
void *mm_malloc(size_t size)
{
	s_block_ptr b, last;
	size_t s;
	s = align4(size);
	
	if(base)
	{ // First find a block.
		last = base;
		b = find_block(&last, s);
		
		if(b)
		{ // Can we split
			if((b->size - s) >= (BLOCK_SIZE + 4))
			{
				split_block(b, s);
			}
			b->free = 0;
		}
		else
		{
			// If no fitting block, extend the heap.
			b = extend_heap(last, s);
			
			if(!b)
			{
				return(NULL);
			}
		}
	}
	else
	{
		// And if its for the first time.
		b = extend_heap(NULL, s);
		if(!b)
		{
			return(NULL);
		}
		base = b;
	}
	return(b->data);
}

// Copies data from the old block to the new allocated block.
// These function will give help to the realloc function. 
void copy_block(s_block_ptr sd, s_block_ptr ot)
{
	int *sdata, *odata;
	size_t f;
	sdata = sd->ptr;
	odata = ot->ptr;
	
	for(f = 0; f * 4 < sd->size && f * 4 < ot->size; f++)
	{
		odata[f] = sdata[f];
	}
}

/* Realloc will:
	* Allocate a new bloc of the given size using malloc;
	* Copy data from the old one to the new one;
	* Free the old block;
	* Return the new pointer.
	* If the size doesn’t change, or the extra-available size (do to alignement constraint,
	  or if the ramainning size was to small to split) is sufficient, we do nothing;
	* If we shrink the block, we try a split;
	* If the next block is free and provide enough space, we fusion and try to split if necessary
*/
void* mm_realloc(void* ptr, size_t size)
{
	size_t s;
	s_block_ptr b, new;
	void *new_p;
	
	if(!ptr)
	{
		return(mm_malloc(size));
	}
	
	if(valid_addr(ptr))
	{
		s = align4(size);
		b = get_block(ptr);
		if(b->size >= s)
		{
			if(b->size - s >= (BLOCK_SIZE + 4))
			{
				split_block(b, s);
			}
		}
		else
		{
			// Try Fusion with the next.
			if((b->next && b->next->free && (b->size + BLOCK_SIZE + b->next->size) >= s)
			{
				fusion(b);
				if(b->size - s >= (BLOCK_SIZE + 4))
				{
					split_block(b, s);
				}
			}
			else
			{
				// Realloc with the new block.
				new_p = mm_malloc(s);
				if(!new_p)
				{
					return(NULL);
				}
				new = get_block(new_p);
				copy_block(b, new); // Copy data.
				free(ptr); // Free old.
				
				return (new_p);
			}
		}
		return(ptr);
	}
	return(NULL);
}

// Validate the address to make it simpler for freeing the space.
int valid_addr(void *p)
{
	if(base)
	{
		if(p > base && p  < sbrk(0))
		{
			return(p == (get_block(p)->ptr));
		}
	}
	return(0);
}

/* With the free() function we verify the pointer and get the corresponding chunk,
	we mark it free and fusion it if possible.
	We also try to release memory if we’re at the end of the heap.
	If we are at the end of the heap, we just have to put the break at the chunk
	position with a simple call to brk(2).
*/
void mm_free(void* ptr)
{
	s_block_ptr b;
	
	if(valid_addr(ptr))
	{
		b = get_block(ptr);
		b->free = 1;
		
		// If possible it will fusion with previous.
		if(b->prev && b->prev->free)
		{
			b = fusion(b->prev);
		}
		// Then fusion with next.
		if(b->next)
		{
			fusion(b);
		}
		else
		{
			//Frees the end of the heap.
			if(b->prev)
			{
				b->prev->next = NULL;
			}
			else
			{
				// If the is no block any more.
				base = NULL;
			}
			brk(b);
		}
	}
}
