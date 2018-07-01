#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"

#define HASHSIZE 1024

extern unsigned int memsize;
extern int debug;
extern struct frame *coremap;
extern char *tracefile;


/* ----------------------- HashTable Data Structure -------------------------*/
struct linkedListNode {
    unsigned long key;
    struct linkedListNode * prev;
    struct linkedListNode * next;
};

struct hashEntryNode {
    unsigned long key;
    struct linkedListNode * start_ptr;
    struct linkedListNode * end_ptr;
    struct hashEntryNode * prev;
    struct hashEntryNode * next;
};

typedef struct hashEntryNode hashEntry;
typedef struct linkedListNode locationList;

hashEntry * hashTable[HASHSIZE];

int hashFunc(addr_t key) {
    int part1 = (key) >> PGDIR_SHIFT;
    int part2 = (((key) >> PAGE_SHIFT) & PGTBL_MASK);
    int part3 = ((key) << (PGDIR_SHIFT + PAGE_SHIFT)) >> (PGDIR_SHIFT + PAGE_SHIFT);
    int hash = 0;
    unsigned input = (part1 + part2) * HASHSIZE + part3;
    while (input >= HASHSIZE) {
        hash += input % HASHSIZE;
        input = (input * 3.1415926) / HASHSIZE;
    }
    return hash % HASHSIZE;
}

hashEntry * getEntryFromHashTable(addr_t vaddr) {
    hashEntry * target = hashTable[hashFunc(vaddr)];
    while (target != NULL && target->key != (unsigned long)vaddr) {
        if (target->next != NULL) {
            // Loop through Linkedlist
            target = target->next;
        } else {
            // End of hashEntry Linkedlist, not found
            return NULL;
        }
    }
    return target;
}

void addEntryToHashTable(hashEntry * newEntry) {

    // Add to the beginning of the linked list
    hashEntry * target = hashTable[hashFunc(newEntry->key)];
    newEntry->next = target;
    if (target != NULL) {
        target->prev = newEntry;
    } else {
        newEntry->prev = NULL;
    }
    hashTable[hashFunc(newEntry->key)] = newEntry;
}

void addLocation(addr_t vaddr, unsigned location) {
    locationList * newLoc = malloc(sizeof(locationList));
    newLoc->key = location;
    newLoc->prev = NULL;
    newLoc->next = NULL;

    hashEntry * target_entry = getEntryFromHashTable(vaddr);
    if (target_entry == NULL) {
        // No such vaddr in hashTable
        hashEntry * newEntry = malloc(sizeof(hashEntry));
        newEntry->key = vaddr;
        newEntry->start_ptr = newLoc;
        newEntry->end_ptr = newLoc;
        newEntry->prev = NULL;
        newEntry->next = NULL;
        addEntryToHashTable(newEntry);
    } else {
        // Exist, add to the end of list
        target_entry->end_ptr->next = newLoc;
        newLoc->prev = target_entry->end_ptr;
        target_entry->end_ptr = newLoc;
    }
}

void removeFirstLocation(addr_t vaddr) {
    hashEntry * entry_ptr = getEntryFromHashTable(vaddr);
    locationList * loc_ptr = entry_ptr->start_ptr;
    if (loc_ptr->prev == NULL && loc_ptr->next == NULL) {
        // Last node in LinkedList, delete hashEntry
        if (entry_ptr->prev != NULL) {
            entry_ptr->prev->next = entry_ptr->next;
        } else {
            // Deleting the first in list
            hashTable[hashFunc(vaddr)] = entry_ptr->next;
        }
        if (entry_ptr->next != NULL) {
            entry_ptr->next->prev = entry_ptr->prev;
        }
        free(loc_ptr);
        free(entry_ptr);
    } else {
        if (loc_ptr->prev != NULL) {
            loc_ptr->prev->next = loc_ptr->next;
        }
        if (loc_ptr->next != NULL) {
            loc_ptr->next->prev = loc_ptr->prev;
        }
        entry_ptr->start_ptr = loc_ptr->next;
        free(loc_ptr);
    }
}

/* ----------------------- End of HashTable Structure -------------------------*/


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
    unsigned long max_next_loc = 0;
    int evict_frame = -1;
    for (int i = 0; i < memsize; i++) {
        addr_t vaddr = *((addr_t *)(&physmem[(coremap[i].pte->frame >> PAGE_SHIFT)*SIMPAGESIZE] + sizeof(int)));
        hashEntry * entry_ptr = getEntryFromHashTable(vaddr);

        if (entry_ptr == NULL) {
            // No location recored in hashTable, never use in future
            max_next_loc = (unsigned int)-1;
            evict_frame = i;
            break;
        }

        unsigned long next_loc = entry_ptr->start_ptr->key;
        if (next_loc > max_next_loc) {
            max_next_loc = next_loc;
            evict_frame = i;
        }
    }

    if (debug == 1) {
        printf("Evicting frame %d, it's next showing time is %ld\n", evict_frame, max_next_loc);
    }

	return evict_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
    addr_t vaddr = *((addr_t *)(&physmem[(p->frame >> PAGE_SHIFT)*SIMPAGESIZE] + sizeof(int)));
    removeFirstLocation(vaddr);
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
    FILE* tracefd;
    char buf[MAXLINE];
    char type;
    addr_t vaddr;

    // Empty hashTable
    for (int i = 0; i < HASHSIZE; i++) {
        hashTable[i] = NULL;
    }

    // Open file
    if(tracefile != NULL) {
        if((tracefd = fopen(tracefile, "r")) == NULL) {
            perror("Error opening tracefile:");
            exit(1);
        }
    }

    int location = 0;
    // Read-in File
    while(fgets(buf, MAXLINE, tracefd) != NULL) {
        if(buf[0] != '=') {
            sscanf(buf, "%c %lx", &type, &vaddr);
            addLocation(vaddr, location);
        } else {
            continue;
        }
        location++;
    }
}


