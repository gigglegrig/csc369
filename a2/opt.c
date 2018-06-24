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

extern char *tracefile;
FILE* tracefd;
unsigned int currloc;
char buf[MAXLINE];


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
    unsigned int max_next_loc = 0;
    int evict_frame = -1;
    addr_t vaddr;
    char type;

    // Save current file pointer location
    fpos_t fd_saveloc;
    fgetpos(tracefd, &fd_saveloc);
    unsigned int saveloc = currloc;

    for (int i = 0; i < memsize; i++) {
        fsetpos(tracefd, &fd_saveloc);
        currloc = saveloc;

        // For every frame update it's next use location
        if (coremap[i].next_use_loc <= currloc) {
            addr_t target_vaddr = *((addr_t *)(&physmem[(coremap[i].pte->frame >> PAGE_SHIFT)*SIMPAGESIZE] + sizeof(int)));
            while(fgets(buf, MAXLINE, tracefd) != NULL) {
                if(buf[0] != '=') {
                    sscanf(buf, "%c %lx", &type, &vaddr);
                } else {
                    currloc++;
                    continue;
                }

                if (vaddr == target_vaddr) {
                    // Find a match, save it into frame and break
                    coremap[i].next_use_loc = currloc;
                    // Compare with current maximum
                    if (currloc > max_next_loc) {
                        max_next_loc = currloc;
                        evict_frame = i;
                    }
                    break;
                }

                currloc++;
            }
            if (coremap[i].next_use_loc < saveloc) {
                // Didn't find until EOF, find the perfect evict
                evict_frame = i;
                coremap[i].next_use_loc = -1;
                break;
            }
        } else {
            if (coremap[i].next_use_loc > max_next_loc) {
                max_next_loc = coremap[i].next_use_loc;
                evict_frame = i;
            }
        }
    }

    // Restore file pointer and location
    fsetpos(tracefd, &fd_saveloc);
    currloc = saveloc;

    printf("Evicting frame %d, it's next showing time is %d\n", evict_frame, coremap[evict_frame].next_use_loc);

    coremap[evict_frame].next_use_loc = 0;

	return evict_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
    currloc++;
    char* res = fgets(buf, MAXLINE, tracefd);
    if (res == NULL) {
        fclose(tracefd);
    }
    while (buf[0] == '=' || buf[0] == '\n') {
        currloc++;
        res = fgets(buf, MAXLINE, tracefd);
        if (res == NULL) {
            fclose(tracefd);
        }
    }
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
    if(tracefile != NULL) {
        if((tracefd = fopen(tracefile, "r")) == NULL) {
            perror("Error opening tracefile:");
            exit(1);
        }
    }
}

