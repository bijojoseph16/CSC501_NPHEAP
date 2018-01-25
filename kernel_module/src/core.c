//////////////////////////////////////////////////////////////////////
//                             North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng
//
//   Description:
//     Skeleton of NPHeap Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "npheap.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <asm/io.h>
#include <linux/radix-tree.h>

extern struct miscdevice npheap_dev;
//struct to hold the size of an object and its location
typedef struct object_node {
    void * address;
    long size;
} object;

RADIX_TREE(requested_offsets, GFP_ATOMIC);
int npheap_mmap(struct file *filp, struct vm_area_struct *vma)
{
    object * new_object;
    void * mem_address;
    long size = vma->vm_end - vma->vm_start;
    new_object = (object *)radix_tree_lookup(&requested_offsets, vma->vm_pgoff);
    if (new_object == NULL) {
        printk(KERN_WARNING "Offset not requested before\n");
        mem_address = kmalloc(size, GFP_ATOMIC);
        new_object = (object *)kmalloc(sizeof(object), GFP_ATOMIC);
        new_object->size = size;
        new_object->address = mem_address;
        radix_tree_insert(&requested_offsets, vma->vm_pgoff, new_object);
    } else {
        printk(KERN_WARNING "Offset has been requested previously\n");
    }
    if (remap_pfn_range(vma, vma->vm_start, virt_to_phys(new_object->address)>>PAGE_SHIFT, new_object->size, vma->vm_page_prot)) {
        return -EAGAIN;
    }
    return 0;
}

int npheap_init(void)
{
    int ret;
    if ((ret = misc_register(&npheap_dev)))
        printk(KERN_ERR "Unable to register \"npheap\" misc device\n");
    else
        printk(KERN_ERR "\"npheap\" misc device installed\n");
    return ret;
}

void npheap_exit(void)
{
    misc_deregister(&npheap_dev);
}

