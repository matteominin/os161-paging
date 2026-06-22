#ifndef _SEGMENTS_H_
#define _SEGMENTS_H_

#include <types.h>

struct addrspace;

/*
 * Segment - data structure associated with the address space,
 * stores data reguarding a single segment
 */
struct segment {
    vaddr_t vaddr;
    size_t memsz;
    size_t filesz;
    off_t offset;
    bool readable;
    bool writable;
    bool executable;
};

/*
 * Functions in segments.c:
 *
 *    seg_find - returns the segment of address (as) space that contains
 *               virtual address (vaddr). Returns NULL if not found.
 */

struct segment *seg_find(struct addrspace* as, vaddr_t vaddr);

#endif /* _SEGMENTS_H_ */