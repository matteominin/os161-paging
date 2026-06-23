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

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>
#include <vfs.h>
#include <vnode.h>

#include <segments.h>
#include <pt.h>

#define VM_STACKPAGES	18


static int as_add_segment(struct addrspace *as, vaddr_t vaddr, size_t memsz,
		 size_t filesz, off_t offset, int readable, int writeable, 
		 int executable);

/*
 * If OPT_DUMBVM is set, this file is not compiled, linked, or used in
 * any way: the cheesy versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(struct vnode *v)
{
	struct addrspace *as;

	KASSERT(v != NULL);
	
	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	as->as_segments = NULL;
	as->as_nsegs = 0;
	as->as_pt = pt_create();
	if (as->as_pt == NULL) {
		kfree(as);
		return NULL;
	}

	VOP_INCREF(v);
	as->as_v = v;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;
	size_t segsize;

	KASSERT(old != NULL);
	KASSERT(ret != NULL);

	newas = as_create(old->as_v);
	if (newas==NULL) {
		return ENOMEM;
	}

	if (old->as_nsegs > 0) {
		segsize = old->as_nsegs * sizeof(struct segment);

		newas->as_segments = kmalloc(segsize);	
		if (newas->as_segments == NULL) {
			as_destroy(newas);
			return ENOMEM;
		}
		memcpy(newas->as_segments, old->as_segments, segsize);
		newas->as_nsegs = old->as_nsegs;
	}  

	// TODO: copy pt (discuss how to manage frames)

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	KASSERT(as != NULL);

	kfree(as->as_segments);
	vfs_close(as->as_v);
	pt_destroy(as->as_pt);
	kfree(as);
}

void
as_activate(void)
{
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}

	/* TODO: invalidate the whole TLB here. */
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are stored in the
 * segment and used to set page protections (e.g. read-only text).
 */

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsz, 
		 size_t filesz, off_t offset, int readable, int writeable, int executable)
{
	int result;

	KASSERT(as != NULL);

	memsz += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;
	memsz = (memsz + PAGE_SIZE - 1) & PAGE_FRAME;
	
	result = as_add_segment(as, vaddr, memsz, filesz, 
		 offset, readable, writeable, executable);

	return result;
}

int
as_prepare_load(struct addrspace *as)
{
	/* On-demand paging: no physical frames are pre-allocated here. */
	KASSERT(as != NULL);

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/* Nothing to finalize: pages are loaded lazily on page fault. */
	KASSERT(as != NULL);


	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	int result;

	KASSERT(as != NULL);
	KASSERT(stackptr != NULL);
	 
	size_t sz = VM_STACKPAGES * PAGE_SIZE;
	vaddr_t vaddr = USERSTACK - sz;
	
	result = as_add_segment(as, vaddr, sz, 0, 0, true, true, false);
	if (result) {
		return result;
	}

	*stackptr = USERSTACK;
	return 0;
}

static int
as_add_segment(struct addrspace *as, vaddr_t vaddr, size_t memsz,
		 size_t filesz, off_t offset, int readable, int writeable, 
		 int executable) {
	struct segment *newarr;

	KASSERT(as != NULL);
	KASSERT((vaddr & PAGE_FRAME) == vaddr);

	// allocate new segments array
	newarr = kmalloc((as->as_nsegs + 1) * sizeof(struct segment));
	if (newarr == NULL) {
		return ENOMEM;
	}

	// copy prev segments into the new array
	if (as->as_segments != NULL) {
		memcpy(newarr, as->as_segments, as->as_nsegs * sizeof(struct segment));
		kfree(as->as_segments);
	}

	as->as_segments = newarr;
	
	// add the new segment
	as->as_segments[as->as_nsegs].vaddr = vaddr;
	as->as_segments[as->as_nsegs].memsz = memsz;
	as->as_segments[as->as_nsegs].filesz = filesz;
	as->as_segments[as->as_nsegs].offset = offset;
	as->as_segments[as->as_nsegs].readable = readable;
	as->as_segments[as->as_nsegs].writable = writeable;
	as->as_segments[as->as_nsegs].executable = executable;
	
	as->as_nsegs++;

	return 0;
}