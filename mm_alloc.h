/*
 * mm_alloc.h
 *
 * Exports a clone of the interface documented in "man 3 malloc".
 */

#pragma once

#ifndef _malloc_H_
#define _malloc_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/types.h>
#include<unistd.h>

void* mm_malloc(size_t size);
void* mm_realloc(void* ptr, size_t size);
void mm_free(void* ptr);


/* block struct */
/*In memory, a struct is simply the concatenation of its fields.
	A struct s block is just 12 bytes(with 32 bit integer) the first four correspond
	to size, the next four to the pointer to next block, and finaly the last four
	are the integer free.
*/
struct s_block {
    size_t size;
    struct block_ptr *next;
    struct block_ptr *prev;
    int free;
    void *ptr;
    /* A pointer to the allocated block */
    char data [0];
 };
 
// Simplifies the use of type.
// We use the link list.
typedef struct s_block *s_block_ptr;
 
/* Define the block size since the sizeof will be wrong */
 //#define BLOCK_SIZE sizeof(struct s_block)
#define BLOCK_SIZE 40

// Finding a free sufficient chunk(space).
// The function returns a fitting chunk, or NULL if none where found.
// After the execution, the argument last points to the last visited chunk.
s_block_ptr find_block(s_block_ptr *last, size_t size)
{
	s_block_ptr b = base;
	while(b && !(b->free && b->size >= size))
	{
		*last = b;
		b = b->next;
	}
	return(b);
}


/* Split block according to size, b must exist */
// We use split to avoid loosing space.
// On these function we cut the block passed as an argument
// To make a data block of wanted size
void split_block (s_block_ptr b, size_t s)
{
	s_block_ptr new;
	new = (s_block_ptr)(b->data + s);
	new->size = b->size - s - BLOCK_SIZE;
	new->next = b->next;
	new->prev = b;
	new->free = 1;
	new->ptr = new->data;
	
	b->size = s;
	b->next = new;
	
	if(new->next)
	{
		new->next->prev = new;
	}
}

/* Try fusing block with neighbors */
// By fusing we eliminate the problem of free space being fragmented.
// So we fuse the free fragmented chunk together with these function.
s_block_ptr fusion(s_block_ptr b)
{
	if(b->next && b->next->free)
	{
		b->size += BLOCK_SIZE + b->next->size;
		b->next = b->next->next;
		
		if(b->next)
		{
			b->next->prev = b;
		}
	}
	return(b);
}

/* Get the block from addr */
// These function will verify and access the block corresponding to a given pointer.
s_block_ptr get_block (void *p)
{
	char *temp;
	temp = p;
	
	return(p = temp -=BLOCK_SIZE);
}

 
// As we won't always have a fitting space, we extend the heap.
s_block_ptr extend_heap (s_block_ptr last , size_t s)
{
	int sh;
	s_block_ptr h;
	h = sbrk(0);
	sh = (int)sbrk(BLOCK_SIZE + s);
	
	if(sh < 0)
	{
		return(NULL); // sbrk failed.
	}
	
	h->size = s;
	h->next = NULL;
	h->prev = last;
	h->ptr = h->data;
	
	if(last)
	{
		last->next = h;
	}
	h->free = 0;
	
	return(h);
}


#ifdef __cplusplus
}
#endif

#endif
