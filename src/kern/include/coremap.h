#ifndef _COREMAP_H_
#define _COREMAP_H_

#include <types.h>
#include "opt-paging.h"

#if OPT_PAGING

/* Initialize physical-frame management. */
void coremap_bootstrap(void);

/* Allocate/free a physically contiguous run of page frames. */
paddr_t coremap_alloc_pages(unsigned long npages);
void coremap_free_pages(paddr_t paddr);

#endif /* OPT_PAGING */

#endif /* _COREMAP_H_ */
