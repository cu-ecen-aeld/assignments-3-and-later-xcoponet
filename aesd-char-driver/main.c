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

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
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
    /**
     * TODO: handle write
     */
    struct aesd_dev *dev = filp->private_data;
    struct aesd_circular_buffer *buffer = &dev->buffer;

    mutex_lock(&dev->mutex);
    struct aesd_buffer_entry entry;
    
    static char * localbuf = NULL;
    static size_t localbuf_size = 0;
    if(localbuf == NULL) {
        localbuf = kmalloc(count, GFP_KERNEL);
        if(localbuf == NULL) {
            mutex_unlock(&dev->mutex);
            return -ENOMEM;
        }
    } else {
        // append to the buffer
        char * tmp = kmalloc(localbuf_size + count, GFP_KERNEL);
        if(tmp == NULL) {
            mutex_unlock(&dev->mutex);
            return -ENOMEM;
        }
        memcpy(tmp, localbuf, localbuf_size);
        kfree(localbuf);
        localbuf = tmp;
        localbuf_size += count;
    }

    if(localbuf[localbuf_size-1] == '\n') {
        // write to the buffer
        entry.buffptr = localbuf;
        entry.size = localbuf_size;
        aesd_circular_buffer_add_entry(buffer, &entry);
        localbuf = NULL;
        localbuf_size = 0;
    }


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

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    mutex_destroy(&aesd_device.mutex);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);