/*
 * Physical page-frame allocator.
 *
 * Before vm_bootstrap, allocations are served by ram_stealmem. Once the
 * coremap is initialized, every physical frame is described by one coremap
 * entry and allocated frames can be returned for reuse.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <vm.h>
#include <coremap.h>

enum coremap_state {
	CM_FREE,
	CM_FIXED,
	CM_HEAD,
	CM_TAIL
};

struct coremap_entry {
	enum coremap_state state;
	unsigned long run_length;
};

static struct coremap_entry *coremap;
static size_t coremap_nframes;
static size_t coremap_first_allocatable;
static size_t coremap_free_count;
static paddr_t coremap_ram_end;
static bool coremap_ready;

static struct spinlock coremap_lock = SPINLOCK_INITIALIZER;

/*
 * Take over physical-memory management from ram_stealmem. The coremap
 * describes all physical frames, including permanently reserved frames.
 * Its own storage is carved directly out of the first available RAM so
 * initialization cannot recurse through kmalloc.
 */
void
coremap_bootstrap(void)
{
	paddr_t first_free;
	paddr_t managed_start;
	paddr_t ram_end;
	size_t available_pages;
	size_t frame_count;
	size_t map_bytes;
	size_t map_pages;
	size_t first_allocatable;
	size_t i;

	spinlock_acquire(&coremap_lock);
	KASSERT(!coremap_ready);

	/* ram_getfirstfree clears ram.c's bounds, so get the end first. */
	ram_end = ram_getsize();
	first_free = ram_getfirstfree();

	KASSERT(ram_end > 0);
	KASSERT(first_free > 0);
	KASSERT(first_free <= ram_end);
	KASSERT((ram_end & (PAGE_SIZE - 1)) == 0);
	KASSERT((first_free & (PAGE_SIZE - 1)) == 0);

	frame_count = ram_end / PAGE_SIZE;
	KASSERT(frame_count <= ((size_t)-1) / sizeof(*coremap));

	map_bytes = frame_count * sizeof(*coremap);
	map_pages = map_bytes / PAGE_SIZE;
	if (map_bytes % PAGE_SIZE != 0) {
		map_pages++;
	}

	available_pages = (ram_end - first_free) / PAGE_SIZE;
	if (map_pages > available_pages) {
		spinlock_release(&coremap_lock);
		panic("coremap: not enough RAM for frame metadata\n");
	}

	managed_start = first_free + map_pages * PAGE_SIZE;
	first_allocatable = managed_start / PAGE_SIZE;
	coremap = (struct coremap_entry *)PADDR_TO_KVADDR(first_free);

	for (i = 0; i < first_allocatable; i++) {
		coremap[i].state = CM_FIXED;
		coremap[i].run_length = 0;
	}
	for (; i < frame_count; i++) {
		coremap[i].state = CM_FREE;
		coremap[i].run_length = 0;
	}

	coremap_nframes = frame_count;
	coremap_first_allocatable = first_allocatable;
	coremap_free_count = frame_count - first_allocatable;
	coremap_ram_end = ram_end;
	coremap_ready = true;

	spinlock_release(&coremap_lock);

	DEBUG(DB_VM, "coremap: %zu frames, %zu free, %zu metadata pages\n",
	      coremap_nframes, coremap_free_count, map_pages);
}

/*
 * Allocate NPAGES physically contiguous frames. A run length is stored in
 * its head entry because free_kpages receives only the run's base address.
 */
paddr_t
coremap_alloc_pages(unsigned long npages)
{
	paddr_t paddr;
	size_t i;
	size_t j;
	size_t run_length;
	size_t run_start;

	if (npages == 0 ||
	    npages > ((paddr_t)-1) / PAGE_SIZE) {
		return 0;
	}

	spinlock_acquire(&coremap_lock);

	if (!coremap_ready) {
		if (npages > ram_getsize() / PAGE_SIZE) {
			spinlock_release(&coremap_lock);
			return 0;
		}
		paddr = ram_stealmem(npages);
		spinlock_release(&coremap_lock);
		return paddr;
	}

	if (npages > coremap_free_count) {
		spinlock_release(&coremap_lock);
		return 0;
	}

	run_length = 0;
	run_start = coremap_first_allocatable;
	for (i = coremap_first_allocatable; i < coremap_nframes; i++) {
		if (coremap[i].state == CM_FREE) {
			if (run_length == 0) {
				run_start = i;
			}
			run_length++;
			if (run_length == npages) {
				coremap[run_start].state = CM_HEAD;
				coremap[run_start].run_length = npages;
				for (j = 1; j < run_length; j++) {
					coremap[run_start + j].state = CM_TAIL;
					coremap[run_start + j].run_length = 0;
				}
				coremap_free_count -= run_length;
				paddr = (paddr_t)(run_start * PAGE_SIZE);
				spinlock_release(&coremap_lock);
				return paddr;
			}
		}
		else {
			run_length = 0;
		}
	}

	spinlock_release(&coremap_lock);
	return 0;
}

/*
 * Free an allocation using its starting physical address. Pages obtained
 * before coremap bootstrap remain fixed and are intentionally not reclaimed.
 */
void
coremap_free_pages(paddr_t paddr)
{
	size_t frame;
	size_t i;
	size_t run_length;

	if (paddr == 0) {
		return;
	}

	spinlock_acquire(&coremap_lock);

	if (!coremap_ready) {
		spinlock_release(&coremap_lock);
		return;
	}

	KASSERT((paddr & (PAGE_SIZE - 1)) == 0);
	KASSERT(paddr < coremap_ram_end);

	frame = paddr / PAGE_SIZE;
	if (frame < coremap_first_allocatable) {
		/* An early ram_stealmem allocation cannot be returned. */
		spinlock_release(&coremap_lock);
		return;
	}

	KASSERT(frame < coremap_nframes);
	KASSERT(coremap[frame].state == CM_HEAD);

	run_length = coremap[frame].run_length;
	KASSERT(run_length > 0);
	KASSERT(run_length <= coremap_nframes - frame);
	for (i = 1; i < run_length; i++) {
		KASSERT(coremap[frame + i].state == CM_TAIL);
	}

	for (i = 0; i < run_length; i++) {
		coremap[frame + i].state = CM_FREE;
		coremap[frame + i].run_length = 0;
	}
	coremap_free_count += run_length;
	KASSERT(coremap_free_count <=
		coremap_nframes - coremap_first_allocatable);

	spinlock_release(&coremap_lock);
}

void
vm_bootstrap(void)
{
	coremap_bootstrap();
}

/* Allocate/free kernel pages in the direct-mapped KSEG0 region. */
vaddr_t
alloc_kpages(unsigned npages)
{
	paddr_t paddr;

	paddr = coremap_alloc_pages(npages);
	if (paddr == 0) {
		return 0;
	}
	return PADDR_TO_KVADDR(paddr);
}

void
free_kpages(vaddr_t addr)
{
	if (addr == 0) {
		return;
	}

	KASSERT((addr & (PAGE_SIZE - 1)) == 0);
	KASSERT(addr >= MIPS_KSEG0);
	KASSERT(addr < MIPS_KSEG1);

	coremap_free_pages(KVADDR_TO_PADDR(addr));
}
