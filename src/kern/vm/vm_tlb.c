/*
 * TLB and page-fault handling for the demand-paged VM system.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <current.h>
#include <proc.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <segments.h>
#include <pt.h>
#include <coremap.h>
#include <swapfile.h>
#include <vm_tlb.h>
#include <vm.h>

/*
 * This is kept as a separate function because later replacement policies
 * can change it without changing vm_fault().
 */
int
tlb_get_rr_victim(void)
{
	static unsigned int next_victim;
	unsigned int victim;

	victim = next_victim;
	next_victim = (next_victim + 1) % NUM_TLB;
	return (int)victim;
}

static int
tlb_get_free_slot(void)
{
	int i;
	uint32_t entryhi;
	uint32_t entrylo;

	for (i = 0; i < NUM_TLB; i++) {
		tlb_read(&entryhi, &entrylo, i);
		if ((entrylo & TLBLO_VALID) == 0) {
			return i;
		}
	}

	return -1;
}

static int
load_fault_page(struct addrspace *as, vaddr_t faultaddress, paddr_t *paddr)
{
	int result;

	*paddr = coremap_alloc_pages(1);
	if (*paddr == 0) {
		return ENOMEM;
	}

	/* load_page also zero-fills the frame before copying ELF data. */
	result = load_page(as, faultaddress, *paddr);
	if (result) {
		coremap_free_pages(*paddr);
		return result;
	}

	return 0;
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	struct addrspace *as;
	struct segment *seg;
	enum pte_status status;
	paddr_t paddr;
	swap_index_t swap_index;
	vaddr_t page;
	uint32_t entryhi;
	uint32_t entrylo;
	int result;
	int slot;
	int spl;

	switch (faulttype) {
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break;
	case VM_FAULT_READONLY:
		/* The trap code terminates the offending user process. */
		return EFAULT;
	default:
		return EINVAL;
	}

	if (curproc == NULL) {
		return EFAULT;
	}

	as = proc_getas();
	if (as == NULL) {
		return EFAULT;
	}

	page = faultaddress & PAGE_FRAME;
	seg = seg_find(as, page);
	if (seg == NULL) {
		return EFAULT;
	}

	status = pt_get_frame(as->as_pt, page, &paddr, &swap_index);
	if (status == PTE_VALID) {
		/* The page is resident; this is only a TLB reload. */
	} else if (status == PTE_SWAPPED) {
		/* Swap-in support is used when a page-table entry is swapped. */
		paddr = coremap_alloc_pages(1);
		if (paddr == 0) {
			return ENOMEM;
		}
		result = swap_in(paddr, swap_index);
		if (result) {
			coremap_free_pages(paddr);
			return result;
		}
		result = pt_set_frame(as->as_pt, page, paddr);
		if (result) {
			coremap_free_pages(paddr);
			return result;
		}
	} else {
		result = load_fault_page(as, page, &paddr);
		if (result) {
			return result;
		}
		result = pt_set_frame(as->as_pt, page, paddr);
		if (result) {
			coremap_free_pages(paddr);
			return result;
		}
	}

	if (faulttype == VM_FAULT_WRITE && !seg->writable) {
		return EACCES;
	}

	entryhi = page;
	entrylo = paddr | TLBLO_VALID | TLBLO_DIRTY;

	/* TLB accesses must not be interrupted by a context switch. */
	spl = splhigh();
	slot = tlb_get_free_slot();
	if (slot < 0) {
		slot = tlb_get_rr_victim();
	}
	tlb_write(entryhi, entrylo, slot);
	splx(spl);

	return 0;
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
}
