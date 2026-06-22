#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <pt.h>

struct pt *
pt_create(void) {
    struct pt *pt;

    pt = kmalloc(sizeof(struct pt));
    if (pt == NULL) return NULL;

    bzero(pt->list, sizeof(pt->list));
    return pt;
}

int 
pt_get_frame(struct pt *pt, vaddr_t vaddr, paddr_t *paddr) {
    size_t l1, l2;
    struct pt_entry* l2_table;
    struct pt_entry entry;
    
    KASSERT(pt != NULL);
    KASSERT(paddr != NULL);

    l1 = L1_INDEX(vaddr);
    l2_table = pt->list[l1];
    if (l2_table == NULL) {
        return PT_NOT_PRESENT;
    }

    l2 = L2_INDEX(vaddr);
    entry = l2_table[l2];
    if (entry.valid) {
        *paddr = entry.paddr;
        return PT_PRESENT;
    }

    if (entry.swapped) {
        return PT_SWAPPED;
    }

    return PT_NOT_PRESENT;
}

int 
pt_set_frame(struct pt *pt, vaddr_t vaddr, paddr_t paddr) {
    size_t l1, l2;
    struct pt_entry* l2_table;

    KASSERT(pt != NULL);
    KASSERT((paddr & PAGE_FRAME) == paddr);

    l1 = L1_INDEX(vaddr);
    l2_table = pt->list[l1];

    if (l2_table == NULL) {
        l2_table = kmalloc(PT_L2_ENTRIES * sizeof(struct pt_entry));
        if (l2_table == NULL) {
            return ENOMEM;
        }

        bzero(l2_table, PT_L2_ENTRIES * sizeof(struct pt_entry));
    }

    l2 = L2_INDEX(vaddr);
    l2_table[l2].paddr = paddr;
    l2_table[l2].valid = true;
    l2_table[l2].swapped = false;

    return 0;
}

void 
pt_destroy(struct pt *pt) {
    int i;
    KASSERT(pt != NULL);

    for (i=0; i<PT_L1_ENTRIES; i++) {
        kfree(pt->list[i]);
    }

    kfree(pt);
}
