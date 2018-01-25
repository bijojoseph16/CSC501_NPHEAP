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

//Changes
#include <linux/radix-tree.h>

//struct to hold the size of an object and its location
typedef struct object_node {
    void * address;
    long size;
} object;

extern struct radix_tree_root requested_offsets;

//tree for the mutexes
RADIX_TREE(mutexes, GFP_ATOMIC);

// If exist, return the data.
long npheap_lock(struct npheap_cmd __user *user_cmd)
{
    //check to see if it exists
    int successful;
    struct mutex * lock = (struct mutex *)radix_tree_lookup(&mutexes, user_cmd->offset / PAGE_SIZE);
    printk(KERN_WARNING "locking object %llu\n", user_cmd->offset / PAGE_SIZE);
    if (lock == NULL) {
        //not been seen before, so create a new one
        lock = (struct mutex *) kmalloc(sizeof(struct mutex), GFP_ATOMIC);
        mutex_init(lock);
        printk(KERN_WARNING "new lock needed for %llu\n", user_cmd->offset / PAGE_SIZE);
        //now insert it into the tree
        //possibility already inserted by different thread, but this won't affect that
        successful = radix_tree_insert(&mutexes, user_cmd->offset / PAGE_SIZE, lock);
        if (successful != 0) {
            printk(KERN_WARNING "lock already created for %llu\n", user_cmd->offset / PAGE_SIZE);
            //already inserted, so free up this unnecessary memory
            kfree(lock);
        }
    }
    //guaranteed to be there now
    printk(KERN_WARNING "getting new lock for %llu\n", user_cmd->offset / PAGE_SIZE);
    lock = (struct mutex *)radix_tree_lookup(&mutexes, user_cmd->offset / PAGE_SIZE);
    mutex_lock(lock);    
    return 0;
}     

long npheap_unlock(struct npheap_cmd __user *user_cmd)
{
    //for unlock, we don't need to worry about if it is has been created or not
    struct mutex * lock = (struct mutex *)radix_tree_lookup(&mutexes, user_cmd->offset / PAGE_SIZE);
    if (lock != NULL) {
        printk(KERN_WARNING "unlocking object %llu\n", user_cmd->offset / PAGE_SIZE);
        mutex_unlock(lock);
    } else {
        printk(KERN_WARNING "no lock for ojbect %llu\n", user_cmd->offset / PAGE_SIZE);
    }
    return 0;
}

long npheap_getsize(struct npheap_cmd __user *user_cmd)
{
    object * old_object;
    printk(KERN_WARNING "requesting size of %llu\n", user_cmd->offset / PAGE_SIZE);
    old_object = (object *)radix_tree_lookup(&requested_offsets, user_cmd->offset / PAGE_SIZE);
    if (old_object != NULL) {
        printk(KERN_WARNING "requested size of object known\n");
        return old_object->size;
    } else {
        printk(KERN_WARNING "requested size of object not seen beore\n");
        return 0;
    }
}
long npheap_delete(struct npheap_cmd __user *user_cmd)
{
    //this is the one to do first
    object * old_object;
    printk(KERN_WARNING "offset is %llu\n", user_cmd->offset / PAGE_SIZE);
    old_object = (object *)radix_tree_delete(&requested_offsets, user_cmd->offset / PAGE_SIZE);
    if (old_object != NULL) {
        printk(KERN_WARNING "deleting object seen before");
        if (old_object->address != NULL) {
            kfree(old_object->address);
        }
        kfree(old_object);
    } else {
        printk(KERN_WARNING "object not here\n");
    }
    return 0;
}

long npheap_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
{
    switch (cmd) {
    case NPHEAP_IOCTL_LOCK:
        return npheap_lock((void __user *) arg);
    case NPHEAP_IOCTL_UNLOCK:
        return npheap_unlock((void __user *) arg);
    case NPHEAP_IOCTL_GETSIZE:
        return npheap_getsize((void __user *) arg);
    case NPHEAP_IOCTL_DELETE:
        return npheap_delete((void __user *) arg);
    default:
        return -ENOTTY;
    }
}
