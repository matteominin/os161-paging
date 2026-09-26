/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>
#include "vm.h"

#define SWAPFILE_MAX_SIZE (9 * 1024 * 1024)
#define SWAP_MAX_PAGES (SWAPFILE_MAX_SIZE / PAGE_SIZE)

typedef uint32_t swap_index_t;

/*
 * Swapfile, handles page tranfer from ram to memory and viceversa
 *
 * Functions in swapfile.h:
 *
 *    swap_bootstrap - allocates empty swapfile on disk
 *
 *    swap_destroy - destorys swap file on disk
 *
 *    swap_out - takes the page with the given `paddr` and stores it in the swapfile.
 *      returns the index inside swapfile
 *
 *    swap_in - takes the page at index `swap_index` in the swapfile
 *      and moves it to the given`paddr`
 */

void swap_bootstrap(void);
void swap_destroy(void);

int swap_out(paddr_t paddr, swap_index_t *swap_index);
int swap_in(paddr_t paddr, swap_index_t swap_index);
// TODO: is it necessary to just free a page from swap?

#endif /* _SWAPFILE_H_ */