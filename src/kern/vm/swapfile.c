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
#include <bitmap.h>
#include <spinlock.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include "swapfile.h"

static struct spinlock swap_lock = SPINLOCK_INITIALIZER;
static char path[] = "SWAPFILE";
static struct vnode *swap_vnode = NULL;
static struct bitmap *swap_map = NULL;

static int swap_io(paddr_t paddr, swap_index_t index, enum uio_rw mode);

void 
swap_bootstrap(void) {
    int result;
    struct bitmap *tmp_bitmap;
    struct vnode *tmp_vnode;

    tmp_bitmap = bitmap_create(SWAP_MAX_PAGES);
    if (tmp_bitmap == NULL) {
        panic("swap_bootstrap couldn't create swap bitmap");
    }

    result = vfs_open(path, O_RDWR | O_CREAT, 0, &tmp_vnode);
    if (result) {
        panic("swap_bootstrap couldn't create or open SWAPFILE");
    } 

    spinlock_acquire(&swap_lock);
    swap_map = tmp_bitmap;
    swap_vnode = tmp_vnode;
    spinlock_release(&swap_lock);
}

void 
swap_destroy(void) {
    spinlock_acquire(&swap_lock);
    
    if (swap_map != NULL) {
        bitmap_destroy(swap_map);
    }

    if (swap_vnode != NULL) {
        vfs_close(swap_vnode);
        vfs_remove(path);
    }

    spinlock_release(&swap_lock);
}

int 
swap_out(paddr_t paddr, swap_index_t *swap_index) {
    swap_index_t index;
    int result;

    spinlock_acquire(&swap_lock);

    result = bitmap_alloc(swap_map, &index);
    if (result == ENOSPC) {
        spinlock_release(&swap_lock);
        panic("Out of swap space");
    }

    spinlock_release(&swap_lock);

    result = swap_io(paddr, index, UIO_WRITE);
    if (result) {
        return result;
    }

    *swap_index = index;
    
    return 0;
}

int
swap_in(paddr_t paddr, swap_index_t swap_index) {
    int result;

    spinlock_acquire(&swap_lock);
    KASSERT(swap_index < SWAP_MAX_PAGES);
    KASSERT(bitmap_isset(swap_map, swap_index));
    spinlock_release(&swap_lock);
    
    result = swap_io(paddr, swap_index, UIO_READ);
    if (result) {
        return result;
    }

    spinlock_acquire(&swap_lock);
    KASSERT(bitmap_isset(swap_map, swap_index));
    bitmap_unmark(swap_map, swap_index);
    spinlock_release(&swap_lock);

    return 0;
}

static int
swap_io(paddr_t paddr, swap_index_t index, enum uio_rw mode) {
    struct iovec iov;
    struct uio u;
    vaddr_t kvaddr;
    off_t offset;
    int result;

    KASSERT(swap_vnode != NULL);
    KASSERT(index < SWAP_MAX_PAGES);
    
    kvaddr = PADDR_TO_KVADDR(paddr);
    offset = index * PAGE_SIZE;
    uio_kinit(&iov, &u, (void *)kvaddr, PAGE_SIZE, offset, mode);

    if (mode == UIO_READ) {
        result = VOP_READ(swap_vnode, &u);
    } else if (mode == UIO_WRITE) {
        result = VOP_WRITE(swap_vnode, &u);
    } else {
        return EINVAL;
    }

    if (result) {
        return result;
    }

    if (u.uio_resid != 0) {
        return EIO;
    }

    return 0;
}