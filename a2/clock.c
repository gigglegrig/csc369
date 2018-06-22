#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int clock_loc;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	int evict_frame;
	int i;
	evict_frame = -1;
	i = clock_loc;
	while (evict_frame == -1) {
		if (coremap[i].in_use) {
			if (coremap[i].pte->frame & PG_REF) {
				coremap[i].pte->frame = coremap[i].pte->frame & (~PG_REF);
			} else {
				evict_frame = i;
			}
		}
		if (++i == memsize) { i = 0; }
		clock_loc = i;
	}
	if (debug) {
		printf("Evicting frame %d\n", evict_frame);
	}
	return evict_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	p->frame = p->frame | PG_REF;
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock_loc = 0;
}
