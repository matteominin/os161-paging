#include <types.h>
#include <lib.h>
#include <segments.h>
#include <addrspace.h>

struct segment*
seg_find(struct addrspace* as, vaddr_t vaddr) {
    size_t i;
    struct segment* seg;

    KASSERT(as != NULL);

    for (i=0; i<as->as_nsegs; i++) {
        seg = &as->as_segments[i];
        
        if (vaddr >= seg->vaddr && 
            vaddr < seg->vaddr + seg->memsz) {
            return seg;
        }
    }

    return NULL;
}