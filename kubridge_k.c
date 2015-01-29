/*
 * Kernel/User mode data bridge lib.
 * Copyright (C) 2015 Ubiquiti Networks (yuchi.chen@ubnt.com)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#include <linux/cdev.h>
#include <linux/errno.h>	/* error codes */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/fs.h>		/* everything... */
#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/types.h>	/* size_t */
#include <linux/version.h>
#include <asm/uaccess.h>

#include "kubridge.h"

struct kubridge_device {
	struct cdev cdev;
};

static struct kubridge_device *kub_devices=NULL;
static int major=0, minor=0, num_devs=1; 
module_param(major, int, 0);
module_param(minor, int, 0);
module_param(num_devs, int, S_IRUGO);
MODULE_AUTHOR("Yuchi");
MODULE_LICENSE("Dual BSD/GPL");

static int device_open(struct inode *inode, struct file *filp)
{
	struct kubridge_device *dev; /* device information */

	/*  Find the device */
	dev = container_of(inode->i_cdev, struct kubridge_device, cdev);

    	/* now trim to 0 the length of the device if open was write-only */
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
		//if (down_interruptible (&dev->sem))
		//	return -ERESTARTSYS;
		//scullc_trim(dev); /* ignore errors */
		//up (&dev->sem);
	}

	/* and use filp->private_data to point to the device data */
	filp->private_data = dev;

	return 0;          /* success */
}

static int device_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static char msg[200];
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
  	return simple_read_from_buffer(buffer, length, offset, msg, 200);
}

static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	if (len > 199) return -EINVAL;
	copy_from_user(msg, buff, len);
	msg[len] = '\0';
	return len;
}

char buf[200];
static long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int len = 200;
	switch(cmd) {
	case READ_IOCTL:	
		copy_to_user((char *)arg, buf, 200);
		break;
	case WRITE_IOCTL:
		copy_from_user(buf, (char *)arg, len);
		break;
	default:
		return -ENOTTY;
	}

	return len;

}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	.read = device_read, 
	.write = device_write,
	.unlocked_ioctl = device_ioctl,
};

static int setup_cdev(struct kubridge_device *dev, int index)
{
	int devno = MKDEV(major, minor+index);
    
	cdev_init(&dev->cdev, &fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &fops;
	return cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	//if (err)
	//	printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

static int __init kubridge_init(void)
{
	int res, i;
	dev_t devno;

	if (major)
	{
		devno = MKDEV(major, minor);
		res = register_chrdev_region(devno, num_devs, DEV_NAME);
	}
	else
	{
		res = alloc_chrdev_region(&devno, minor, num_devs, DEV_NAME);
		major = MAJOR(devno);
		minor = MINOR(devno);
	}

	if (res<0)
	{
		printk(DEV_NAME " can't get major = %d\n", major);
		return res;
	}

	kub_devices = kmalloc(num_devs * sizeof(struct kubridge_device), GFP_KERNEL);
	if (!kub_devices) {
		res = -ENOMEM;
		goto fail_malloc;
	}

	res = 0;
	for (i=0; i<num_devs && res==0; i++)
	{
		res = setup_cdev(kub_devices+i, i);
	}

	if (res)
	{
		i--;		// i is fail, no need to del
		for (; i>=0; i--)
		{
			cdev_del(&kub_devices[i].cdev);
		}
		kfree(kub_devices);
		printk(KERN_NOTICE " " DEV_NAME  " Error %d", res);
		goto fail_malloc;
	}

	printk("cdev example: assigned major: %d\n", major);
	printk("create node with mknod /dev/" DEV_NAME " c %d %d\n", major, minor);
 	return 0;

fail_malloc:
	unregister_chrdev_region(devno, num_devs);
	return res;
}

static void __exit kubridge_exit(void)
{
	int i;

	for (i = 0; i < num_devs; i++) {
		cdev_del(&kub_devices[i].cdev);
	}
	kfree(kub_devices);
	unregister_chrdev_region(MKDEV(major, minor), num_devs);
}  

module_init(kubridge_init);
module_exit(kubridge_exit);
MODULE_LICENSE("GPL");
