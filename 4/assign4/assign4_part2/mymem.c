/* References: https://github.com/Arctem/team_otw_os_labs/tree/master/totw_p5
https://github.com/BenEarle/SYSC4001-Assignments/tree/master/assignment4*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "mymem.h"
#include <time.h>

/* The main structure for implementing memory allocation.
 * You may change this to fit your implementation.
 */

struct memoryList
{
	// doubly-linked list
	struct memoryList *prev;
	struct memoryList *next;

	int size;            // How many bytes in this block?
	char alloc;          // 1 if this block is allocated,
	// 0 if this block is free.
	void *ptr;           // location of block in memory pool.
};

strategies myStrategy = NotSet;    // Current strategy


size_t mySize;
void *myMemory = NULL;

static struct memoryList *head;
static struct memoryList *next;

// This function releases memory in initddmem
void release() {
	struct memoryList *tmp = NULL;
	while(head != NULL) {
		tmp = head;
		head = head->next;
		free(tmp);
	}

	head = next = NULL;
}

// This function initializes memory for initmem
void initialize() {
	head = malloc(sizeof(struct memoryList));
	head->prev = head->next = NULL;
	head->alloc = 0;
	head->size = mySize;
	head->ptr = myMemory;

	next = head;
}

/* initmem must be called prior to mymalloc and myfree.
   initmem may be called more than once in a given exeuction;
   when this occurs, all memory you previously malloc'ed  *must* be freed,
   including any existing bookkeeping data.
   strategy must be one of the following:
		- "best" (best-fit)
		- "worst" (worst-fit)
		- "first" (first-fit)
		- "next" (next-fit)
   sz specifies the number of bytes that will be available, in total, for all mymalloc requests.
*/

void initmem(strategies strategy, size_t sz) {
	myStrategy = strategy;

	/* all implementations will need an actual block of memory to use */
	mySize = sz;

	if (myMemory != NULL)
		free(myMemory); /* in case this is not the first time initmem2 is called */

	/* release any other memory you were using for bookkeeping when doing a re-initialization! */
	release();

	myMemory = malloc(sz);

	/* Initialize memory management structure. */
	initialize();
}

/* Allocate a block of memory with the requested size.
 *  If the requested block is not available, mymalloc returns NULL.
 *  Otherwise, it returns a pointer to the newly allocated block.
 *  Restriction: requested >= 1
 */

void *mymalloc(size_t requested) {
	assert((int)myStrategy > 0);

	struct memoryList *to_use = NULL; /*Which chunk of memory are we going to use?*/
	struct memoryList *tmp = NULL; /* Some will use this */

	switch (myStrategy) {
		case NotSet:
			return NULL;
		case First:
			to_use = head;
			while(to_use != NULL) {
				if(to_use->size >= requested && !to_use->alloc) {
					break;
				} else {
					to_use = to_use->next;
				}
			}
			break;
		case Best:
			tmp = head;
			while(tmp != NULL) {
				if((!tmp->alloc && tmp->size >= requested) && (!to_use || tmp->size < to_use->size)) {
					to_use = tmp;
				}
				tmp = tmp->next;
			}
			break;
		case Worst:
			tmp = head;
			while(tmp != NULL) {
				if((!tmp->alloc && tmp->size >= requested) && (!to_use || tmp->size > to_use->size)) {
					to_use = tmp;
				}
				tmp = tmp->next;
			}
			break;
		case Next:
			while(next->alloc) {
				next = next->next;
			}
			to_use = next;
			break;
	}

	if(to_use) {
		if(to_use->size == requested) {
			/* simple case, no splitting needed */
			to_use->alloc = 1;
		} else {
			/* allocate what's needed, but create a new entry for the rest */
			int extra = to_use->size - requested;
			struct memoryList *new_list = malloc(sizeof(struct memoryList));
			new_list->next = to_use->next;
			if(new_list->next) {
				new_list->next->prev = new_list;
			}
			new_list->prev = to_use;
			to_use->next = new_list;

			new_list->alloc = 0;
			to_use->alloc = 1;
			new_list->size = extra;
			to_use->size = requested;
			new_list->ptr = to_use->ptr + requested;
		}

		/* Update next if needed */
		if(myStrategy == Next) {
			next = to_use->next;

			while(!next || (next->alloc && next != to_use)) {
				if(!next) {
					next = head;
				} else {
					next = next->next;
				}
			}
			if(next == to_use) {
				next = head;
			}
		}
		return to_use->ptr;
	} else {
		return NULL;
	}
}


/* Merges adjacent freed blocks into a single block */
void merge_blocks(struct memoryList *block) {
	struct memoryList *tmp;

	if(block->alloc) {
		return;
	}
	else {
		/* first move to the start of this block */
		while(block->prev && !block->prev->alloc) {
			block = block->prev;
		}

		/* next merge forward so it's all in one struct */
		while(block->next && !block->next->alloc) {
			tmp = block->next;
			block->size += tmp->size;
			block->next = tmp->next;
			if(tmp->next) {
				tmp->next->prev = block;
			}
			if(next == tmp) {
				next = block;
			}
			free(tmp);
		}
	}
}

/* Frees a block of memory previously allocated by mymalloc. */
void myfree(void* block) {
	struct memoryList *tmp = head;

	// iterate through memory until item to free
	while(tmp != NULL) {
		if(tmp->ptr == block) {
			tmp->alloc = 0;
			merge_blocks(tmp);
			return;
		}
		tmp = tmp->next;
	}
}

/****** Memory status/property functions ******
 * Implement these functions.
 * Note that when we refer to "memory" here, we mean the
 * memory pool this module manages via initmem/mymalloc/myfree.
 */

/* Get the number of contiguous areas of free space in memory. */
int mem_holes() {
	struct memoryList *tmp = NULL;
	int holes = 0;

	// iterate through the memory
	tmp = head;
	while(tmp != NULL) {
		if(!tmp->alloc) {
			holes++;
		}
		tmp = tmp->next;
	}
	return holes;
}

/* Get the number of bytes allocated */
int mem_allocated() {
	struct memoryList *tmp = NULL;
	int allocated_bytes = 0;

	// iterate through the memory
	tmp = head;
	while(tmp != NULL) {
		if(tmp->alloc) {
			allocated_bytes += tmp->size;
		}
		tmp = tmp->next;
	}
	return allocated_bytes;
}

/* Number of non-allocated (a.k.a. free) bytes */
int mem_free() {
	struct memoryList *tmp = NULL;
	int non_bytes = 0;

	// iterate through the memory
	tmp = head;
	while(tmp != NULL) {
		if(tmp->alloc == 0) {
			non_bytes += tmp->size;
		}
		tmp = tmp->next;
	}
	return non_bytes;
}

/* Number of bytes in the largest contiguous area of unallocated memory */
int mem_largest_free() {
	struct memoryList *tmp = NULL;
	int free_bytes = 0;

	// iterate through the memory
	tmp = head;
	while(tmp != NULL) {
		if(tmp->alloc == 0 && tmp->size > free_bytes) {
			free_bytes = tmp->size;
		}
		tmp = tmp->next;
	}
	return free_bytes;
}

/* Number of free blocks smaller than "size" bytes. */
/* NOTE: test case seems to be assuming <= rather than < */
int mem_small_free(int size) {
	struct memoryList *tmp = NULL;
	int free_blocks = 0;

	// iterate through the memory
	tmp = head;
	while(tmp != NULL) {
		if(!tmp->alloc && tmp->size <= size) {
			free_blocks++;
		}
		tmp = tmp->next;
	}
	return free_blocks;
}

char mem_is_alloc(void *ptr) {
	struct memoryList *tmp = head;

	while(tmp != NULL) {
		if(tmp->ptr == ptr) {
			return tmp->alloc;
		}
		tmp = tmp->next;
	}
	return 0;
}

/*
 * Feel free to use these functions, but do not modify them.
 * The test code uses them, but you may ind them useful.
 */


//Returns a pointer to the memory pool.
void *mem_pool() {
	return myMemory;
}

// Returns the total number of bytes in the memory pool. */
int mem_total() {
	return mySize;
}


// Get string name for a strategy.
char *strategy_name(strategies strategy) {
	switch (strategy) {
		case Best:
			return "best";
		case Worst:
			return "worst";
		case First:
			return "first";
		case Next:
			return "next";
		default:
			return "unknown";
	}
}

// Get strategy from name.
strategies strategyFromString(char * strategy) {
	if (!strcmp(strategy,"best")) {
		return Best;
	} else if (!strcmp(strategy,"worst")) {
		return Worst;
	} else if (!strcmp(strategy,"first")) {
		return First;
	} else if (!strcmp(strategy,"next")) {
		return Next;
	} else {
		return 0;
	}
}


/*
 * These functions are for you to modify however you see fit.  These will not
 * be used in tests, but you may find them useful for debugging.
 */

/* Use this function to print out the current contents of memory. */
void print_memory() {
	return;
}

/* Use this function to track memory allocation performance.
 * This function does not depend on your implementation,
 * but on the functions you wrote above.
 */
void print_memory_status() {
	printf("%d out of %d bytes allocated.\n",mem_allocated(),mem_total());
	printf("%d bytes are free in %d holes; maximum allocatable block is %d bytes.\n",mem_free(),mem_holes(),mem_largest_free());
	printf("Average hole size is %f.\n\n",((float)mem_free())/mem_holes());
}

/* Use this function to see what happens when your malloc and free
 * implementations are called.  Run "mem -try <args>" to call this function.
 * We have given you a simple example to start.
 */
void try_mymem(int argc, char **argv) {
	strategies strat;
	void *a, *b, *c, *d, *e;
	if(argc > 1)
		strat = strategyFromString(argv[1]);
	else
		strat = First;


	/* A simple example.
       Each algorithm should produce a different layout. */

	initmem(strat,500);

	a = mymalloc(100);
	b = mymalloc(100);
	c = mymalloc(100);
	myfree(b);
	d = mymalloc(50);
	myfree(a);
	e = mymalloc(25);

	print_memory();
	print_memory_status();
}