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
#include <linux/uaccess.h> 
#include <linux/string.h>
#include <linux/slab.h> 
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Mukta"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
	struct aesd_dev *dev;
	PDEBUG("open");
	/**
	 * TODO: handle open
	 */
	dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
	filp->private_data = dev; /* for other methods */

	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	struct aesd_dev *dev = filp->private_data;
	size_t rtn_offset = 0;
	size_t buf_index = 0;
	size_t copy_bytes = 0;
	size_t count_copy = count;
	struct aesd_buffer_entry *buffer_entry = NULL;
	unsigned long not_copied_bytes = 0;
	int error = 0;
	
	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */
	 
	//lock 
	error = mutex_lock_interruptible(&dev->rwmutex);
	if(error != 0)
	{
		return error;
	}
	
	//Get buffer entry pointer from circular buffer as per given f_pos
	do
	{
		buffer_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->cbfifo, *f_pos, &rtn_offset);
		if(buffer_entry == NULL)
		{	//if no entry found then return 0
			mutex_unlock(&dev->rwmutex);
			return retval;
		}
		
		//get number of bytes to be copied from offset till end of buffer
		copy_bytes = buffer_entry->size - rtn_offset;
		
		//if number of bytes to be copied are greater than required bytes
		if(copy_bytes  > count)
			copy_bytes = count;
		
		//copy bytes from circular buffer to user buffer	
		not_copied_bytes = copy_to_user(&buf[buf_index], &buffer_entry->buffptr[rtn_offset], copy_bytes);
		//if(not_copied_bytes == 0 && (count_copy == copy_bytes))
		//{
		//	break;
		//}
		
		//update all pointers as per copied bytes
		*f_pos += (copy_bytes - not_copied_bytes);
		buf_index += (copy_bytes - not_copied_bytes);
		retval += (copy_bytes - not_copied_bytes);
		
		//get bytes remaining to be copied
		count_copy = count_copy - (copy_bytes - not_copied_bytes); 
		
	}while(count_copy);	//repeat till bytes = count are copied
	
	//unlock
	mutex_unlock(&dev->rwmutex);
	return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = -ENOMEM;
	struct aesd_dev *dev = filp->private_data;
	size_t buff_index = 0;
	unsigned long not_copied_bytes = 0;
	int error = 0;
	const char *free_entry = NULL;
	
	PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle write
	 */
	 
	//lock
	error = mutex_lock_interruptible(&dev->rwmutex);
	if(error != 0)
	{
		return error;
	}
	
	//check for size = 0 or buffptr = 0 -> new entry
	//malloc or realloc space as per this condition
	if(dev->current_entry.size == 0)
	{	//new entry so malloc space
		dev->current_entry.buffptr = kzalloc((sizeof(char) * count), GFP_KERNEL);
		if(dev->current_entry.buffptr == NULL)
		{
			retval = -ENOMEM;
			mutex_unlock(&dev->rwmutex);
			return retval;
		}
	} 
	else
	{	//existing entry so realloc pointer to increase size
		dev->current_entry.buffptr = krealloc(dev->current_entry.buffptr, (dev->current_entry.size + count), GFP_KERNEL);
		if(dev->current_entry.buffptr == NULL)
		{
			retval = -ENOMEM;
			mutex_unlock(&dev->rwmutex);
			return retval;
		}		
	}
	
	//update buffer index and entry size as per above allocation	
	buff_index = dev->current_entry.size;
	dev->current_entry.size += count;
	
	//copy from user
	not_copied_bytes = copy_from_user((void *)(dev->current_entry.buffptr + buff_index), buf, count);
	
	//update entry buffer size, retval of bytes
	retval = count - not_copied_bytes;
	buff_index = count - not_copied_bytes;
	dev->current_entry.size -= not_copied_bytes;
	
	
	//check for newline found
	if((strchr(dev->current_entry.buffptr, '\n')) != 0)
	{
		//if yes, add new buffer entry in cbfifo
		free_entry = aesd_circular_buffer_add_entry(&dev->cbfifo, &dev->current_entry);
		if(free_entry)
			kfree(free_entry);
			
		//Reset current_entry
		dev->current_entry.buffptr = NULL;
		dev->current_entry.size = 0;
	}	
	
	//unlock
	mutex_unlock(&dev->rwmutex);
	
	return retval;
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
	
	//init mutex
	mutex_init(&aesd_device.rwmutex);
	
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
	 
	//free all allocated space
	if(aesd_device.current_entry.buffptr)
		kfree(aesd_device.current_entry.buffptr);
		
	aesd_circular_buffer_delete(&aesd_device.cbfifo);

	unregister_chrdev_region(devno, 1);
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);

