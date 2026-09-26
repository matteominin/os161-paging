#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <pt.h>

struct pt_l1 *
pt_create(void) {
    struct pt_l1 *pt_l1;

    pt_l1 = kmalloc(sizeof(struct pt_l1));
    if (pt_l1 == NULL) return NULL;

    bzero(pt_l1->list, sizeof(pt_l1->list));
    return pt_l1;
}

enum pte_status
pt_get_frame(struct pt_l1 *pt_l1, vaddr_t vaddr, paddr_t *paddr,
             swap_index_t *swap_index) {
    size_t l1, l2;
    struct pt_l2* pt_l2;
    struct pt_entry entry;
    
    KASSERT(pt_l1 != NULL);
    KASSERT(paddr != NULL);
    KASSERT(swap_index != NULL);

    l1 = L1_INDEX(vaddr);
    pt_l2 = pt_l1->list[l1];
    if (pt_l2 == NULL) {
        return PTE_INVALID;
    }

    l2 = L2_INDEX(vaddr);
    entry = pt_l2->list[l2];
    if (entry.status == PTE_VALID) {
        *paddr = entry.paddr;
        return PTE_VALID;
    }

    if (entry.status == PTE_SWAPPED) {
        *swap_index = entry.swap_index;
        return PTE_SWAPPED;
    }

    return PTE_INVALID;
}

int 
pt_set_frame(struct pt_l1 *pt_l1, vaddr_t vaddr, paddr_t paddr) {
    size_t l1, l2;
    struct pt_l2* pt_l2;

    KASSERT(pt_l1 != NULL);
    KASSERT((paddr & PAGE_FRAME) == paddr);

    l1 = L1_INDEX(vaddr);
    pt_l2 = pt_l1->list[l1];

    if (pt_l2 == NULL) {
        pt_l2 = kmalloc(sizeof(struct pt_l2));
        if (pt_l2 == NULL) {
            return ENOMEM;
        }

        bzero(pt_l2->list, PT_L2_ENTRIES * sizeof(struct pt_entry));
        pt_l1->list[l1] = pt_l2;
    }

    l2 = L2_INDEX(vaddr);
    pt_l2->list[l2].paddr = paddr;
    pt_l2->list[l2].status = PTE_VALID;

    return 0;
}

void 
pt_destroy(struct pt_l1 *pt_l1) {
    int i;
    KASSERT(pt_l1 != NULL);

    for (i=0; i<PT_L1_ENTRIES; i++) {
        kfree(pt_l1->list[i]);
    }

    kfree(pt_l1);
}
