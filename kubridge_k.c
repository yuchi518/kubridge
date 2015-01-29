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
#include <linux/semaphore.h> /* sem */
#include <linux/types.h>	/* size_t */
#include <linux/version.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#include "kubridge.h"
#include "uthash.h"

// read
struct kub_event_listener_info
{
	IOCtlCmd cmd;

	size_t sizeOfPayload;
	void *buff;
	kub_event_handler listener;

	UT_hash_handle hh;
};

// send
struct kub_event_send_pkt
{
	struct list_head list;
	size_t sizeOfPayload;
	void *payload;

	kub_event_handler complete;
};

struct kub_event_send_info
{
	IOCtlCmd cmd;
	struct list_head packets;

	UT_hash_handle hh;
};

// dev
struct kubridge_device {
	struct cdev cdev;
	struct semaphore sem;
	struct kub_event_listener_info *listeners;
	struct kub_event_send_info *sends;
};

static struct kubridge_device *kub_devices=NULL;
static int major=0;
static int minor=0;
static int num_devs=KUB_NUM_OF_BRIDGES; 
module_param(major, int, 0);
module_param(minor, int, 0);
module_param(num_devs, int, S_IRUGO);

/// ===================== 
int kub_register_event_listener(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, kub_event_handler listener)
{
	struct kub_event_listener_info *li=NULL;
	int res = 0;

	if (down_interruptible(&kub_devices[bridge].sem))
		return -ERESTARTSYS;

	HASH_FIND_INT(kub_devices[bridge].listeners, &cmd, li);
	if (li)
	{
		HASH_DEL(kub_devices[bridge].listeners, li);
		kfree(li->buff);
		kfree(li);
		li = NULL;
	}

	if (listener==NULL)
		goto end;

	li = kmalloc(sizeof(*li), GFP_KERNEL);
	if (!li) {
		res = -ENOMEM;
		goto end;
	}

	li->cmd = cmd;
	li->sizeOfPayload = sizeOfPayload;
	li->listener = listener;
	li->buff = kmalloc(sizeof(li->sizeOfPayload), GFP_KERNEL);
	if (li->buff==NULL) {
		res = -ENOMEM;
		kfree(li);
		goto end;
	}

	printk("add listener bridge=%d, cmd=%d\n", bridge, cmd);

	HASH_ADD_INT(kub_devices[bridge].listeners, cmd, li);

end:
	up(&kub_devices[bridge].sem);
	return res;
}
EXPORT_SYMBOL(kub_register_event_listener);

int kub_get_event_listener(struct kubridge_device *dev, IOCtlCmd cmd, struct kub_event_listener_info **info)
{
	int res=0;
	*info = NULL;

	printk("kub_get_event_listener 1\n");

	//if (down_interruptible(&dev->sem))
	//	return -ERESTARTSYS;

	printk("kub_get_event_listener 2\n");
	//printk("should find\n");
	HASH_FIND_INT(dev->listeners, &cmd, *info);

	printk("kub_get_event_listener 3\n");
//end:
	//up(&dev->sem);
	return res;
}

void kub_release_event_listeners(struct kubridge_device *dev)
{
	struct kub_event_listener_info *li=NULL, *tmp;
	//if (down_interruptible(&dev->sem))
	//	return -ERESTARTSYS;

	HASH_ITER(hh, dev->listeners, li, tmp) {
   	HASH_DEL(dev->listeners, li);  /* delete; users advances to next */
   	kfree(li->buff);
   	kfree(li);            /* optional- if you want to free  */
	}

	//up(&dev->sem);
	//return 0;
}

int kub_send_event(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, void *payload, kub_event_handler complete)
{
	struct kub_event_send_info *sd = NULL;
	struct kub_event_send_pkt *pkt = NULL;
	int res = 0;

	printk("kub_send_event 1\n");

	//if (down_interruptible(&kub_devices[bridge].sem))
	//		return -ERESTARTSYS;

	printk("kub_send_event 2\n");

	HASH_FIND_INT(kub_devices[bridge].sends, &cmd, sd);
	if (sd==NULL)
	{
		// create one
		sd = kmalloc(sizeof(*sd), GFP_KERNEL);
		if (!sd) {
			res = -ENOMEM;
			goto end;
		}
		sd->cmd = cmd;
		INIT_LIST_HEAD(&sd->packets);
		HASH_ADD_INT(kub_devices[bridge].sends, cmd, sd);
	}

	printk("kub_send_event 3\n");

	pkt = kmalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt) {
		res = -ENOMEM;
		goto end;
	}

	pkt->sizeOfPayload = sizeOfPayload;
	pkt->payload = payload;

	printk("kub_send_event 4\n");

	list_add_tail(&pkt->list, &sd->packets);

end:
	//up(&kub_devices[bridge].sem);
	return res;
}
EXPORT_SYMBOL(kub_send_event);

int kub_pop_send_event(struct kubridge_device *dev, IOCtlCmd cmd, struct kub_event_send_pkt **pkt)
{
	struct kub_event_send_info *sd = NULL;
	int res = 0;
	*pkt = NULL;

	//if (down_interruptible(&dev->sem))
	//		return -ERESTARTSYS;

	HASH_FIND_INT(dev->sends, &cmd, sd);
	if (sd==NULL)		// no entry
		goto end;
	
	if (list_empty(&sd->packets))	// no data
		goto end;

	*pkt = list_entry(sd->packets.next, struct kub_event_send_pkt, list);
	list_del(&(*pkt)->list);

end:
	//up(&dev->sem);
	return res;	
}

void kub_release_send_events(struct kubridge_device *dev)
{
	struct kub_event_send_info *sd = NULL, *tmp;
	struct kub_event_send_pkt *pkt;

	//if (down_interruptible(&dev->sem))
	//		return -ERESTARTSYS;

	HASH_ITER(hh, dev->sends, sd, tmp) {
   	HASH_DEL(dev->sends, sd);  /* delete; users advances to next */

		while (!list_empty(&sd->packets))
		{
			pkt = list_entry(sd->packets.next, struct kub_event_send_pkt, list);
			list_del(&pkt->list);
			kfree(pkt);
		}

   	kfree(sd);            /* optional- if you want to free  */
	}

	up(&dev->sem);
	//return 0;	
}

/// ===================== Device Driver implementation ============================

static int device_open(struct inode *inode, struct file *filp)
{
	struct kubridge_device *dev; /* device information */

	/*  Find the device */
	dev = container_of(inode->i_cdev, struct kubridge_device, cdev);

    	/* now trim to 0 the length of the device if open was write-only */
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
		//scullc_trim(dev); /* ignore errors */
		up(&dev->sem);
	}

	/* and use filp->private_data to point to the device data */
	filp->private_data = dev;

	return 0;          /* success */
}

static int device_release(struct inode *inode, struct file *filp)
{
	return 0;
}

#if 0
static char msg[200];
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	struct kubridge_device *dev = filp->private_data;
	if (down_interruptible (&dev->sem))
		return -ERESTARTSYS;

  	return simple_read_from_buffer(buffer, length, offset, msg, 200);
}

static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	if (len > 199) return -EINVAL;
	copy_from_user(msg, buff, len);
	msg[len] = '\0';
	return len;
}
#endif

char buf[200];
static long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct kubridge_device *dev = filep->private_data;
	struct kub_event_listener_info *li = NULL;
	struct kub_event_send_pkt *pkt = NULL;

	int len = 0;
	if (down_interruptible (&dev->sem))
		return -ERESTARTSYS;

	if (cmd & IOC_IN)
	{
		printk("IOC_IN\n");
		// in, read from user
		kub_get_event_listener(dev, cmd, &li);
		if (li)
		{
			copy_from_user(li->buff, (char*)arg, li->sizeOfPayload);
			len = li->sizeOfPayload;
			li->listener((int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd, li->sizeOfPayload, li->buff);
		}
		else
		{
			printk("No listener (bridge=%d, cmd=%d)\n", (int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd);
		}
	}

	if (cmd & IOC_OUT)
	{
		printk("IOC_OUT\n");
		kub_pop_send_event(dev, cmd, &pkt);
		if (pkt)
		{
			// out, copy to user
			copy_to_user((char*)arg, pkt->payload, pkt->sizeOfPayload);
			len = pkt->sizeOfPayload;
			if (pkt->complete)
				pkt->complete((int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd, pkt->sizeOfPayload, pkt->payload);

			kfree(pkt);
		}
		else
		{
			printk("No packet (bridge=%d, cmd=%d)\n", (int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd);
		}
	}

	printk("device_ioctl\n");
	up(&dev->sem);
	
	return len;
	/*
	
	switch(cmd) {
	case READ_IOCTL:	
		copy_to_user((char *)arg, buf, 200);
		break;
	case WRITE_IOCTL:
		copy_from_user(buf, (char *)arg, len);
		break;
	default:
		up(&dev->sem);
		return -ENOTTY;
	}

	
	return len;*/

}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	//.read = device_read, 
	//.write = device_write,
	.unlocked_ioctl = device_ioctl,
};

static inline int setup_cdev(struct kubridge_device *dev, int index)
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
		sema_init(&kub_devices[i].sem, 1);
		kub_devices[i].listeners = NULL;
		kub_devices[i].sends = NULL;
		res = setup_cdev(kub_devices+i, i);
	}

	if (res)
	{
		i--;		// i is fail, no need to del
		for (; i>=0; i--)
		{
			cdev_del(&kub_devices[i].cdev);
			kub_release_event_listeners(&kub_devices[i]);
			kub_release_send_events(&kub_devices[i]);
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
		kub_release_event_listeners(&kub_devices[i]);
		kub_release_send_events(&kub_devices[i]);
	}
	kfree(kub_devices);
	unregister_chrdev_region(MKDEV(major, minor), num_devs);
}  

module_init(kubridge_init);
module_exit(kubridge_exit);
MODULE_AUTHOR("Yuchi");
MODULE_LICENSE("GPL");

