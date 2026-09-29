#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <swapfile.h>
#include <vmstats.h>
#include "opt-dumbvm.h"

static unsigned int stats_counts[VMSTAT_COUNT];
static bool stats_active;
static struct spinlock stats_lock = SPINLOCK_INITIALIZER;

static const char *const stats_names[VMSTAT_COUNT] = {
	"TLB Faults",
	"TLB Faults with Free",
	"TLB Faults with Replace",
	"TLB Invalidations",
	"TLB Reloads",
	"Page Faults (Zeroed)",
	"Page Faults (Disk)",
	"Page Faults from ELF",
	"Page Faults from Swapfile",
	"Swapfile Writes"
};

void
vmstats_init(void)
{
	unsigned int i;

	spinlock_acquire(&stats_lock);
	for (i = 0; i < VMSTAT_COUNT; i++) {
		stats_counts[i] = 0;
	}
	stats_active = true;
	spinlock_release(&stats_lock);
}

void
vmstats_inc(unsigned int index)
{
	KASSERT(index < VMSTAT_COUNT);

	spinlock_acquire(&stats_lock);
	if (stats_active) {
		stats_counts[index]++;
	}
	spinlock_release(&stats_lock);
}

void
vmstats_print(void)
{
	unsigned int counts[VMSTAT_COUNT];
	unsigned int i;
	unsigned int tlb_faults;
	unsigned int free_or_replace;
	unsigned int reload_or_page_fault;
	unsigned int disk_faults;
	unsigned int file_or_swap_reads;

	/* Do not hold a spinlock while kprintf may block. */
	spinlock_acquire(&stats_lock);
	for (i = 0; i < VMSTAT_COUNT; i++) {
		counts[i] = stats_counts[i];
	}
	spinlock_release(&stats_lock);

	kprintf("VMSTATS:\n");
	for (i = 0; i < VMSTAT_COUNT; i++) {
		kprintf("VMSTAT %25s = %10u\n", stats_names[i], counts[i]);
	}

	tlb_faults = counts[VMSTAT_TLB_FAULT];
	free_or_replace = counts[VMSTAT_TLB_FAULT_FREE] +
		counts[VMSTAT_TLB_FAULT_REPLACE];
	reload_or_page_fault = counts[VMSTAT_TLB_RELOAD] +
		counts[VMSTAT_PAGE_FAULT_ZERO] +
		counts[VMSTAT_PAGE_FAULT_DISK];
	disk_faults = counts[VMSTAT_PAGE_FAULT_DISK];
	file_or_swap_reads = counts[VMSTAT_ELF_FILE_READ] +
		counts[VMSTAT_SWAP_FILE_READ];

	if (tlb_faults != free_or_replace) {
		kprintf("WARNING: TLB Faults (%u) != Free + Replace (%u)\n",
			tlb_faults, free_or_replace);
	}
	if (tlb_faults != reload_or_page_fault) {
		kprintf("WARNING: TLB Faults (%u) != Reloads + Zeroed + Disk (%u)\n",
			tlb_faults, reload_or_page_fault);
	}
	if (disk_faults != file_or_swap_reads) {
		kprintf("WARNING: Page Faults (Disk) (%u) != ELF + Swapfile (%u)\n",
			disk_faults, file_or_swap_reads);
	}
}

void
vm_shutdown(void)
{
	vmstats_print();
#if !OPT_DUMBVM
	swap_destroy();
#endif
}
