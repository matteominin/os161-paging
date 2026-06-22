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

struct segment *seg_find(struct addrspace* as, vaddr_t vaddr);

#endif /* _SEGMENTS_H_ */