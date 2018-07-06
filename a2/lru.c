#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"


extern unsigned int memsize;

extern int debug;

extern struct frame *coremap;

struct frame * stack_head = NULL;
struct frame * stack_tail = NULL;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	struct frame * evict_ptr = stack_tail;
	if (stack_tail->sp_prev != NULL) {
		stack_tail->sp_prev->sp_next = NULL;
	} else {
		// prev is NULL indicates this is the last node in list
		stack_head = NULL;
	}
	stack_tail = stack_tail->sp_prev;

	evict_ptr->sp_next = NULL;
	evict_ptr->sp_prev = NULL;

	if (debug == 1) {
		printf("evicting %lx\n", *(addr_t *)(&physmem[(evict_ptr->pte->frame >> PAGE_SHIFT)*SIMPAGESIZE] + sizeof(int)));
	}

	
	return (evict_ptr->pte->frame) >> PAGE_SHIFT;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	struct frame * target_node = &(coremap[(p->frame) >> PAGE_SHIFT]);
	if (stack_head == NULL && stack_tail == NULL) {
		// The initial item
		stack_head = target_node;
		stack_tail = target_node;
	} else {
		// Put referenced node at the beginning of the list
		if (target_node->sp_next == NULL && target_node->sp_prev == NULL) {
			// New node or first node in list
			if (!(stack_head == stack_tail && stack_head == target_node)) {
				// Link a new node to the list
				target_node->sp_next = stack_head;
				stack_head->sp_prev = target_node;
				stack_head = target_node;
			}
		} else {
			// Node already in list (also at least two nodes)
			if (stack_tail == target_node) {
				stack_tail = target_node->sp_prev;
			}
			// Change the location of the node to the beginning one
			if (target_node->sp_prev != NULL) {
				target_node->sp_prev->sp_next = target_node->sp_next;
			}
			if (target_node->sp_next != NULL) {
				target_node->sp_next->sp_prev = target_node->sp_prev;
			}
			target_node->sp_prev = NULL;
			target_node->sp_next = stack_head;
			stack_head->sp_prev = target_node;
			stack_head = target_node;
		}
	}
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
}
