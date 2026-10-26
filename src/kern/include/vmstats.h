#ifndef _VMSTATS_H_
#define _VMSTATS_H_

#include <types.h>
#include "opt-paging.h"

#if OPT_PAGING
/* VM activity counters required by the paging assignment. */
#define VMSTAT_TLB_FAULT          0
#define VMSTAT_TLB_FAULT_FREE     1
#define VMSTAT_TLB_FAULT_REPLACE  2
#define VMSTAT_TLB_INVALIDATE     3
#define VMSTAT_TLB_RELOAD         4
#define VMSTAT_PAGE_FAULT_ZERO    5
#define VMSTAT_PAGE_FAULT_DISK    6
#define VMSTAT_ELF_FILE_READ      7
#define VMSTAT_SWAP_FILE_READ     8
#define VMSTAT_SWAP_FILE_WRITE    9
#define VMSTAT_COUNT              10

void vmstats_init(void);
void vmstats_inc(unsigned int index);
/* Record one completed fault: reload, zero fill, ELF read, or swap read. */
void vmstats_record_fault(unsigned int source, bool replaced);
void vmstats_print(void);

#endif /* OPT_PAGING */

#endif /* _VMSTATS_H_ */
