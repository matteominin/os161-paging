#include <types.h>
#include <lib.h>
#include <vm.h>
#include <pt.h>

struct pt *
pt_create(size_t npages, vaddr_t base) {
    struct pt *pt;
    size_t i;

    KASSERT(npages > 0);
    KASSERT((base & PAGE_FRAME) == base);

    pt = kmalloc(sizeof(struct pt));
    if (pt == NULL) return NULL;

    pt->npages = npages;
    pt->base = base;
    pt->entries = kmalloc(npages * sizeof(struct pt_entry));
    if (pt->entries == NULL) {
        kfree(pt);
        return NULL;
    }

    for (i=0; i<npages; i++) {
        pt->entries[i].paddr = 0;
        pt->entries[i].valid = false;
        pt->entries[i].swapped = false;
    }

    return pt;
}

int 
pt_get_frame(struct pt *pt, vaddr_t vaddr, paddr_t *paddr) {
    size_t index;
    struct pt_entry entry;
    
    KASSERT(pt != NULL);
    KASSERT(paddr != NULL);

    index = (vaddr - pt->base) / PAGE_SIZE;

    KASSERT(index < pt->npages);

    entry = pt->entries[index];

    if (entry.valid) {
        *paddr = entry.paddr;
        return PT_PRESENT;
    }

    if (entry.swapped) {
        return PT_SWAPPED;
    }

    return PT_NOT_PRESENT;
}

void 
pt_set_frame(struct pt *pt, vaddr_t vaddr, paddr_t paddr) {
    size_t index;

    KASSERT(pt != NULL);
    KASSERT((paddr & PAGE_FRAME) == paddr);

    index = (vaddr - pt->base) / PAGE_SIZE;

    KASSERT(index < pt->npages);

    pt->entries[index].paddr = paddr;
    pt->entries[index].valid = true;
    pt->entries[index].swapped = false;
}

void 
pt_destroy(struct pt *pt) {
    KASSERT(pt != NULL);

    kfree(pt->entries);
    kfree(pt);
}
