#ifndef _PT_H
#define _PT_H

#include <types.h>

#define PT_L1_BITS      10
#define PT_L2_BITS      10
#define PT_PAGE_BITS       12
#define PT_L1_ENTRIES   (1 << PT_L1_BITS)
#define PT_L2_ENTRIES   (1 << PT_L2_BITS)
#define L1_INDEX(addr)  ((addr >> (PT_L2_BITS + PT_PAGE_BITS)) & (PT_L1_ENTRIES - 1))
#define L2_INDEX(addr)  (((addr) >> PT_PAGE_BITS) & (PT_L2_ENTRIES - 1))

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
    struct pt_entry *list[PT_L1_ENTRIES];    
};

/*
 * Two-level per-process page table.
 * Virtual address are split as follows: [ L1 index | L2 index | offset ]. 
 *
 * Functions in pt.c:
 *
 *    pt_create - allocate an empty page table (L1 directory, all entries
 *                NULL). Second-level tables are created on demand.
 *                Returns NULL on out-of-memory error.
 *
 *    pt_get_frame - look up the page for VADDR and return its state:
 *                PT_PRESENT (in RAM, frame stored in *paddr),
 *                PT_SWAPPED (on the swap file) or PT_NOT_PRESENT.
 *
 *    pt_set_frame - mark the page for VADDR as present in RAM at the
 *                physical address PADDR.
 *                Returns 0 on success, ENOMEM if the
 *                second-level table could not be allocated.
 *
 *    pt_destroy - free the page table, including all second-level tables.
 */

struct pt *pt_create(void);
int pt_get_frame(struct pt *pt, vaddr_t vaddr, paddr_t *paddr);
int pt_set_frame(struct pt *pt, vaddr_t vaddr, paddr_t paddr);
void pt_destroy(struct pt *pt);

#endif /* _PT_H */