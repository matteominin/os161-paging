#ifndef _PT_H
#define _PT_H

#include <types.h>

enum pt_status {
    PT_PRESENT,
    PT_SWAPPED,
    PT_NOT_PRESENT
};

struct pt_entry {
    paddr_t paddr;
    bool valid;
    bool swapped;
};

struct pt {
    struct pt_entry* entries;    
    size_t npages;
    vaddr_t base;
};

/*
 * Functions in pt.c:
 *
 *    pt_create - allocate a page table of NPAGES entries for the segment
 *                starting at BASE. Every entry starts "not present".
 *                Returns NULL on out-of-memory error.
 *
 *    pt_get_frame - look up the page for VADDR and return its state:
 *                PT_PRESENT (in RAM, frame stored in *paddr),
 *                PT_SWAPPED (on the swap file) or PT_NOT_PRESENT.
 *
 *    pt_set_frame - mark the page for VADDR as present in RAM at the
 *                physical address PADDR.
 *
 *    pt_destroy - free the page table and its entries.
 */

struct pt *pt_create(size_t npages, vaddr_t base);
int pt_get_frame(struct pt *pt, vaddr_t vaddr, paddr_t *paddr);
void pt_set_frame(struct pt *pt, vaddr_t vaddr, paddr_t paddr);
void pt_destroy(struct pt *pt);

#endif /* _PT_H */