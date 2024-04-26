/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Xavier COPONET");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    // Open the device
    struct aesd_dev *dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;

    // Increment the usage count of the module
    try_module_get(THIS_MODULE);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    // Release the device and decrement the usage count of the module
    module_put(THIS_MODULE);

    return 0;
}

/*
* return value == count, requested number of bytes read successfully
* return value > 0 but < count, only part of the requested number of bytes read
* return value == 0, EOF; no data read
* return value < 0, error occurred. EFAULT, ERESTARTSYS, EINTR
*/
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *buffer = &dev->buffer;

    mutex_lock(&dev->mutex);
    struct aesd_buffer_entry * entry;

    // int index;
    // AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
    //     PDEBUG("index %d, entry size %zu",index, entry->size);
    //     PDEBUG("%p", entry->buffptr);
    // }
    
    size_t offset = 0;
    size_t out_buff_idx = 0;
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(buffer, *f_pos, &offset);

    while(entry != NULL) {
        size_t to_copy = entry->size - offset;
        if(to_copy > count) {
            to_copy = count;
        }

        if(copy_to_user(buf + out_buff_idx, entry->buffptr + offset, to_copy)) {
            retval = -EFAULT;
        } else {
            retval += to_copy;
            out_buff_idx += to_copy;
            *f_pos += to_copy;
        }

        entry = aesd_circular_buffer_find_entry_offset_for_fpos(buffer, *f_pos, &offset);
    }


    mutex_unlock(&dev->mutex);
    return retval;
}

/*
* return value == count, requested number of bytes written successfully
* return value > 0 but < count, only part of the requested number of bytes written, may retry
* return value == 0, no data written, may retry
* return value < 0, error occurred. ENOMEM, EFAULT
*/
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    // char * localbuf = kmalloc(count + 1, GFP_KERNEL);
    // if(localbuf == NULL) {
    //     return -ENOMEM;
    // }
    // if(copy_from_user(localbuf, buf, count)) {
    //     kfree(localbuf);
    //     return -EFAULT;
    // }
    // localbuf[count] = '\0';
    // PDEBUG("write %s", localbuf);
    

    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *buffer = &dev->buffer;

    mutex_lock(&dev->mutex);
    struct aesd_buffer_entry * entry = &dev->tempEntry;
    
    // char * localbuf = entry->buffptr;
    // size_t localbuf_size = entry->size;
    if(entry->buffptr == NULL) {
        entry->buffptr = kmalloc(count, GFP_KERNEL);
        if(entry->buffptr == NULL) {
            mutex_unlock(&dev->mutex);
            return -ENOMEM;
        }
        if(copy_from_user(entry->buffptr, buf, count)) {
            mutex_unlock(&dev->mutex);
            return -EFAULT;
        }
        entry->size = count;
    } else {
        // append to the buffer
        char * tmp = kmalloc(entry->size + count, GFP_KERNEL);
        if(tmp == NULL) {
            mutex_unlock(&dev->mutex);
            return -ENOMEM;
        }
        memcpy(tmp, entry->buffptr, entry->size);
        if(copy_from_user(tmp + entry->size, buf, count)) {
            kfree(tmp);
            mutex_unlock(&dev->mutex);
            return -EFAULT;
        }
        kfree(entry->buffptr);
        entry->buffptr = tmp;
        entry->size += count;
    }

    // PDEBUG("entry buffer %s, size %d", entry->buffptr, entry->size);
    // for(int i = 0; i < entry->size; i++) {
    //     PDEBUG("|%c|", entry->buffptr[i]);
    // }
    if(entry->buffptr[entry->size-1] == '\n') {
    // if(localbuf[count-1] == '\n') {
        PDEBUG("write entry buffer %s, size %d", entry->buffptr, entry->size);
        // write to the buffer
        struct aesd_buffer_entry localEntry;

        localEntry.buffptr = entry->buffptr;
        localEntry.size = entry->size;
        char* ovewritten = aesd_circular_buffer_add_entry(buffer, &localEntry);
        if(ovewritten != NULL) {
            PDEBUG("freeing overwritten entry %p, %s", ovewritten, ovewritten);
            kfree(ovewritten);
            ovewritten = NULL;
        }
        entry->buffptr = NULL;
        entry->size = 0;
    }
    else {
        PDEBUG("buffering the command");
    }

    // struct aesd_buffer_entry entry;
    
    // char * localbuf = NULL;
    
    // localbuf = kmalloc(count, GFP_KERNEL);
    // if(localbuf == NULL) {
    //     mutex_unlock(&dev->mutex);
    //     return -ENOMEM;
    // }
    // if(copy_from_user(localbuf, buf, count)) {
    //     mutex_unlock(&dev->mutex);
    //     return -EFAULT;
    // }
    // entry.buffptr = localbuf;
    // entry.size = count;
    // char* ovewritten = aesd_circular_buffer_add_entry(buffer, &entry);
    // if(ovewritten != NULL) {
    //     PDEBUG("freeing overwritten entry %p", ovewritten);
    //     kfree(ovewritten);
    //     ovewritten = NULL;
    // }


    mutex_unlock(&dev->mutex);

    return count;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    aesd_circular_buffer_init(&aesd_device.buffer);
    aesd_device.tempEntry.buffptr = NULL;
    aesd_device.tempEntry.size = 0;
    mutex_init(&aesd_device.mutex);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    PDEBUG("unregistering device");

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    mutex_lock(&aesd_device.mutex);
    struct aesd_buffer_entry *entry;
    int index;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
        PDEBUG("index %d, entry size %zu",index, entry->size);
        PDEBUG("%p", entry->buffptr);
    }


    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
        PDEBUG("Freeing entry %d, %p", index, entry->buffptr);
        if(entry->buffptr != NULL)
        {   
            PDEBUG("%s", entry->buffptr);
            kfree(entry->buffptr);
            entry->buffptr = NULL;
        }
    }

    if(aesd_device.tempEntry.buffptr != NULL) {
        kfree(aesd_device.tempEntry.buffptr);
        aesd_device.tempEntry.buffptr = NULL;
    }

    mutex_unlock(&aesd_device.mutex);
    mutex_destroy(&aesd_device.mutex);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
