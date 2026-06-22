#ifndef _SEGMENTS_H_
#define _SEGMENTS_H_

#include <types.h>

/*
 * Segment - data structure associated with the address space,
 * stores data reguarding a single segment
 */

#if OPT_PAGING
struct segment {
    vaddr_t vaddr;
    size_t memsz;
    size_t filesz;
    off_t offset;
    bool readable;
    bool writable;
    bool executable;
};
#endif /* OPT_PAGING */

#endif /* _SEGMENTS_H_ */