#ifndef _VM_TLB_H_
#define _VM_TLB_H_

/* Select the next TLB slot when all entries are in use. */
int tlb_get_rr_victim(void);

#endif /* _VM_TLB_H_ */
