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
#include <linux/device.h>
#include <linux/errno.h>	/* error codes */
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/fs.h>		/* alloc_chrdev_region, everything... */
#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/slab.h>		/* kmalloc() */
#include <linux/sysfs.h>
#include <linux/semaphore.h> /* sem */
#include <linux/types.h>	/* size_t */
#include <linux/version.h>
#include <asm/uaccess.h>	/* copy_to_user, copy_from_user */
#include <linux/interrupt.h>

#include "kubridge.h"
#include "uthash.h"
#include "list.h"

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
	int count;			// count of packets

	UT_hash_handle hh;
};

// dev
struct kubridge_device {
	struct cdev cdev;
	struct device *node;			// point to /dev/name[0~N]
	struct semaphore sem;
	wait_queue_head_t in_q/*, out_q*/;       /* read and write queues in user viewpoint */
	struct kub_event_listener_info *listeners;
	struct kub_event_send_info *sends;

	unsigned int listen_cnt, send_cnt;

#if ENABLE_KUB_SYSFS	
	struct kobject *kobj;								// this is for sysfs, this is not same as cdev's kobj
#endif
};

static struct class *kub_class;
static struct kubridge_device *kub_devices=NULL;
static int major=0;
static int minor=0;
static int num_devs=KUB_NUM_OF_BRIDGES; 
module_param(major, int, 0);
module_param(minor, int, 0);
module_param(num_devs, int, 0);

/// ===================== 
int kub_register_event_listener(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, kub_event_handler listener)
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

	li = kzalloc(sizeof(*li), GFP_KERNEL);
	if (!li) {
		res = -ENOMEM;
		goto end;
	}

	li->cmd = cmd;
	li->sizeOfPayload = _IOC_SIZE(cmd);//sizeOfPayload;
	li->listener = listener;
	li->buff = kzalloc(li->sizeOfPayload, GFP_KERNEL);
	if (li->buff==NULL) {
		res = -ENOMEM;
		kfree(li);
		goto end;
	}

	//printk("add listener bridge=%d, cmd=%d\n", bridge, cmd);

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

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	HASH_FIND_INT(dev->listeners, &cmd, *info);

	if (*info) dev->listen_cnt++;
//end:
	up(&dev->sem);
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

int kub_send_event(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload, kub_event_handler complete)
{
	struct kub_event_send_info *sd = NULL;
	struct kub_event_send_pkt *pkt = NULL;
	int res = 0;

	if (down_interruptible(&kub_devices[bridge].sem))
		return -ERESTARTSYS;

	HASH_FIND_INT(kub_devices[bridge].sends, &cmd, sd);
	if (sd==NULL)
	{
		// create one
		sd = kzalloc(sizeof(*sd), GFP_KERNEL);
		if (!sd) {
			res = -ENOMEM;
			goto end;
		}
		sd->cmd = cmd;
		INIT_LIST_HEAD(&sd->packets);
		HASH_ADD_INT(kub_devices[bridge].sends, cmd, sd);
	}

	pkt = kzalloc(sizeof(*pkt), GFP_KERNEL);
	if (!pkt) {
		res = -ENOMEM;
		if (list_empty(&sd->packets))		// all don't, clean
			HASH_DEL(kub_devices[bridge].sends, sd);
		printk("kub_send_event, **NO memory**\n");
		goto end;
	}

	pkt->sizeOfPayload = _IOC_SIZE(cmd); //sizeOfPayload;
	pkt->payload = payload;
	pkt->complete = complete;

	sd->count ++;

	list_add_tail(&pkt->list, &sd->packets);

	wake_up(&kub_devices[bridge].in_q);
	//wake_up_interruptible(&dev->in_q);

end:
	up(&kub_devices[bridge].sem);
	return res;
}
EXPORT_SYMBOL(kub_send_event);

int kub_pop_send_event(struct kubridge_device *dev, IOCtlCmd cmd, struct kub_event_send_pkt **pkt)
{
	struct kub_event_send_info *sd = NULL;
	int res = 0;
	*pkt = NULL;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	HASH_FIND_INT(dev->sends, &cmd, sd);
	if (sd==NULL)		// no entry
		goto end;
	
	if (list_empty(&sd->packets))	// no data
		goto end;

	*pkt = list_entry(sd->packets.next, struct kub_event_send_pkt, list);
	list_del(&(*pkt)->list);
	sd->count--;

	dev->send_cnt++;

	if (list_empty(&sd->packets))		// all don't, clean
		HASH_DEL(dev->sends, sd);

end:
	up(&dev->sem);
	return res;	
}

int kub_check_send_events(struct kubridge_device *dev)
{
	//struct kub_event_send_info *sd = NULL, *tmp;
	int size=0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	size = HASH_COUNT(dev->sends);

	/*HASH_ITER(hh, dev->sends, sd, tmp) {
		if (!list_empty(&sd->packets)) {
			up(&dev->sem);
			return 1;
		}
	}*/

	up(&dev->sem);
	return size;
}

int kub_fill_send_events(struct kubridge_device *dev, IOCtlCmd *cmds, int cntOfCmds)
{
	struct kub_event_send_info *sd = NULL, *tmp;
	int i=0;
	
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	HASH_ITER(hh, dev->sends, sd, tmp) {
		if (!list_empty(&sd->packets)) {
			cmds[i] = sd->cmd;
			i++;
			if (i>=cntOfCmds) break;
		}
	}

	up(&dev->sem);
	return 0;
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

	//up(&dev->sem);
	//return 0;	
}

/// ===================== Device Driver implementation ============================
static ssize_t kub_class_show(struct device *child, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, DEV_NAME);
}
static DEVICE_ATTR(kub_class, 0444, kub_class_show, NULL);

static int device_open(struct inode *inode, struct file *filp)
{
	struct kubridge_device *dev; /* device information */

	/*  Find the device */
	dev = container_of(inode->i_cdev, struct kubridge_device, cdev);

    	/* now trim to 0 the length of the device if open was write-only */
	if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
		//if (down_interruptible(&dev->sem))
		//	return -ERESTARTSYS;
		//scullc_trim(dev); /* ignore errors */
		//up(&dev->sem);
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

static unsigned int device_poll(struct file *filep, poll_table *wait_tb)
{
    struct kubridge_device *dev = filep->private_data;
    unsigned int mask = 0;

    /*
     * The buffer is circular; it is considered full
     * if "wp" is right behind "rp" and empty if the
     * two are equal.
     */
    //down(&dev->sem);
    
    poll_wait(filep, &dev->in_q,  wait_tb);
    //poll_wait(filep, &dev->out_q, wait_tb);
    if (kub_check_send_events(dev) > 0)
        mask |= POLLIN | POLLRDNORM;    /* readable */
    //if (spacefree(dev))
        mask |= POLLOUT | POLLWRNORM;   /* writable */

    //up(&dev->sem);
    return mask;
}


//char buf[200];
static long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	struct kubridge_device *dev = filep->private_data;
	struct kub_event_listener_info *li = NULL;
	struct kub_event_send_pkt *pkt = NULL;

	int len = 0;

	//if (_IOC_TYPE(cmd) != KUB_MAGIC) return -ENOTTY;

	//if (down_interruptible (&dev->sem))
	//	return 0;

	if (cmd & IOC_IN)
	{
		{
			// in, read from user
			kub_get_event_listener(dev, cmd, &li);
			if (li)
			{
				copy_from_user(li->buff, (char*)arg, li->sizeOfPayload);
				len = li->sizeOfPayload;
				li->listener((int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd/*, li->sizeOfPayload*/, li->buff);
			}
			else
			{
				printk("No listener (bridge=%d, cmd=%d)\n", (int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd);
			}
		}
	}

	if (cmd & IOC_OUT)
	{
		// check reserved first
		if ((_IOC_TYPE(cmd) == KUB_MAGIC) && _IOC_NR(cmd)==_IOC_NR(IOC_READ_CMD_INFO))
		{
			if (_IOC_SIZE(cmd)==4)
			{
				int s = kub_check_send_events(dev);
				//printk("[%d] %d cmdms\n", (int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), s);
				//put_user(s, (char*)arg);
				copy_to_user((char*)arg, &s, 4);
				len = 4;
			}
			else
				return -ENOTTY;
		}
		else if ((_IOC_TYPE(cmd) == KUB_MAGIC) && _IOC_NR(cmd)==_IOC_NR(IOC_READ_CMDS(0)))
		{
			IOCtlCmd *cmds = NULL;
			len = _IOC_SIZE(cmd);
			cmds = kzalloc(len, GFP_KERNEL);
			if (cmds)
			{
				memset(cmds, 0, len);
				kub_fill_send_events(dev, cmds, len/sizeof(IOCtlCmd));
				copy_to_user((char*)arg, cmds, len);
				kfree(cmds);
			}
			else
				return -ENOMEM;
		}
		else
		{
			kub_pop_send_event(dev, cmd, &pkt);
			if (pkt)
			{
				// out, copy to user
				copy_to_user((char*)arg, pkt->payload, pkt->sizeOfPayload);
				len = pkt->sizeOfPayload;
				if (pkt->complete)
					pkt->complete((int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd/*, pkt->sizeOfPayload*/, pkt->payload);

				kfree(pkt);
			}
			else
			{
				printk("No packet (bridge=%d, cmd=%d)\n", (int)((unsigned long)dev-(unsigned long)kub_devices)/sizeof(*kub_devices), cmd);
			}
		}
	}

	//up(&dev->sem);
	
	return len;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = device_open,
	.release = device_release,
	//.read = device_read, 
	//.write = device_write,
	.unlocked_ioctl = device_ioctl,
	.poll = device_poll,
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


// kubinfo proc
#if ENABLE_KUB_PROC
static int kubinfo_proc_show(struct seq_file *m, void *v) {
	int i;
	unsigned int lCnt, sCnt;

	seq_printf(m, DEV_NAME " usage:\n");
	for (i=0; i<num_devs; i++)
	{
		if (down_interruptible(&kub_devices[i].sem))
			continue;
		lCnt = kub_devices[i].listen_cnt;
		sCnt = kub_devices[i].send_cnt;
		up(&kub_devices[i].sem);

		seq_printf(m, "[%d] listen %d times, send %d tiems.\n", i, lCnt, sCnt);
	}
  return 0;
}

static int kubinfo_proc_open(struct inode *inode, struct  file *file) {
  return single_open(file, kubinfo_proc_show, NULL);
}

static const struct file_operations kubinfo_fops = {
  .owner = THIS_MODULE,
  .open = kubinfo_proc_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};
#endif

// sysfs
#if ENABLE_KUB_SYSFS
// kobj_attr_show, kobj_attr_store, kobj_sysfs_ops are copied from linux kernel source.
/* default kobject attribute operations */
static ssize_t kobj_attr_show(struct kobject *kobj, struct attribute *attr,
			      char *buf)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->show)
		ret = kattr->show(kobj, kattr, buf);
	return ret;
}

static ssize_t kobj_attr_store(struct kobject *kobj, struct attribute *attr,
			       const char *buf, size_t count)
{
	struct kobj_attribute *kattr;
	ssize_t ret = -EIO;

	kattr = container_of(attr, struct kobj_attribute, attr);
	if (kattr->store)
		ret = kattr->store(kobj, kattr, buf, count);
	return ret;
}

static struct sysfs_ops kub_sysfs_ops = {
	.show	= kobj_attr_show,
	.store	= kobj_attr_store,
};

ssize_t usage_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i;
	unsigned int lCnt, sCnt;
	int charCnt = 0;

	for (i=0; i<num_devs; i++)
	{
		if (kub_devices[i].kobj==kobj) break;
	}

	if (i==num_devs) {
		printk("Can't find kobj\n");
		return 0;
	}

	charCnt += sprintf(buf+charCnt, DEV_NAME " usage:\n");
	if (down_interruptible(&kub_devices[i].sem))
			return 0;
	lCnt = kub_devices[i].listen_cnt;
	sCnt = kub_devices[i].send_cnt;
	up(&kub_devices[i].sem);

	charCnt += sprintf(buf+charCnt, "[%d] listen %d times, send %d tiems.\n", i, lCnt, sCnt);
	return charCnt;
}

ssize_t usage_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	// ignore all input =,=
	return count;
}

static struct kobj_attribute usage_attribute = __ATTR(usage, 0666, usage_show, usage_store);
static struct attribute *kubinfo_default_attrs[] = {
	&usage_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static void kub_kobj_release(struct kobject *kobj)
{
	pr_debug("kobject: (%p): %s\n", kobj, __func__);
	kfree(kobj);
}

static struct kobj_type kubinfo_ktype = {
	.release	= kub_kobj_release,
	.sysfs_ops	= &kub_sysfs_ops, //&kobj_sysfs_ops,
	.default_attrs = kubinfo_default_attrs,
};

static struct kset *kubinfo_kset;
#endif


// module init/deinit

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

	kub_class = class_create(THIS_MODULE, DEV_NAME);

	kub_devices = kzalloc(num_devs * sizeof(struct kubridge_device), GFP_KERNEL);
	if (!kub_devices) {
		res = -ENOMEM;
		goto fail_malloc;
	}

	res = 0;
	for (i=0; i<num_devs && res==0; i++)
	{
		sema_init(&kub_devices[i].sem, 1);
		init_waitqueue_head(&kub_devices[i].in_q);
		//init_waitqueue_head(&kub_devices[i].out_q);
		kub_devices[i].listeners = NULL;
		kub_devices[i].sends = NULL;
		kub_devices[i].listen_cnt = 0;
		kub_devices[i].send_cnt = 0;
		if ((res = setup_cdev(kub_devices+i, i))==0)
		{
			kub_devices[i].node = device_create(kub_class, NULL, MKDEV(major, minor+i), NULL, DEV_NAME_FMT, i);
			device_create_file(kub_devices[i].node, &dev_attr_kub_class);
		}

	}

	if (res)
	{
		i--;		// i is fail, no need to del
		for (; i>=0; i--)
		{
			device_remove_file(kub_devices[i].node, &dev_attr_kub_class);
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

	// kubinfo proc
#if ENABLE_KUB_PROC
	proc_create(PROC_NAME, 0, NULL, &kubinfo_fops);
#endif
	
	// sysfs
#if ENABLE_KUB_SYSFS
	kubinfo_kset = kset_create_and_add(SYSFS_NAME, NULL, kernel_kobj);
	if (kubinfo_kset)
	{
		for (i=0; i<num_devs; i++)
		{
			//As we have a kset for this kobject, we need to set it before calling the kobject core.
			kub_devices[i].kobj = kzalloc(sizeof(*kub_devices[i].kobj), GFP_KERNEL);
			if (!kub_devices[i].kobj) {
				printk("Can't create kubridge's sysfs.\n");
				break;
			}

			kub_devices[i].kobj->kset = kubinfo_kset;

			/*
			 * Initialize and add the kobject to the kernel.  All the default files
			 * will be created here.  As we have already specified a kset for this
			 * kobject, we don't have to set a parent for the kobject, the kobject
			 * will be placed beneath that kset automatically.
			 */
			if (kobject_init_and_add(kub_devices[i].kobj, &kubinfo_ktype, NULL, SYSFS_SUBNAME, i)) {
				printk("init_and_add error\n");
				break;
			}

			/*
			 * We are always responsible for sending the uevent that the kobject
			 * was added to the system.
			 */
			kobject_uevent(kub_devices[i].kobj, KOBJ_ADD);

			printk("Create %d kobj\n", i);
		}
		
		if (i!=num_devs)
		{
			for (; i>=0; i--)
			{
				if (kub_devices[i].kobj) {
					kobject_put(kub_devices[i].kobj);
					kub_devices[i].kobj = NULL;
				}
			}
			kset_unregister(kubinfo_kset);
			kubinfo_kset = NULL;
		}
	}
	else
	{
		// because this is information, is not load, just alert but continue
		printk("kubinfo's kset was not created.\n");
		//return -ENOMEM;
	}
#endif

 	return 0;

fail_malloc:
	unregister_chrdev_region(devno, num_devs);
	return res;
}

static void __exit kubridge_exit(void)
{
	int i;

#if ENABLE_KUB_SYSFS
	if (kubinfo_kset)
	{
		for (i=0; i<num_devs; i++)
		{
			if (kub_devices[i].kobj) {
				kobject_put(kub_devices[i].kobj);
				kub_devices[i].kobj = NULL;
			}
		}
		kset_unregister(kubinfo_kset);
	}
#endif

#if ENABLE_KUB_PROC
	// kubinfo proc
	remove_proc_entry(PROC_NAME, NULL);
#endif

	// kubridge
	for (i = 0; i < num_devs; i++) {
		device_remove_file(kub_devices[i].node, &dev_attr_kub_class);
		device_destroy(kub_class, MKDEV(major, minor+i));
		cdev_del(&kub_devices[i].cdev);
		kub_release_event_listeners(&kub_devices[i]);
		kub_release_send_events(&kub_devices[i]);
	}
	kfree(kub_devices);

	class_destroy(kub_class);

	unregister_chrdev_region(MKDEV(major, minor), num_devs);
}  

module_init(kubridge_init);
module_exit(kubridge_exit);
MODULE_AUTHOR("Yuchi");
MODULE_LICENSE("GPL");

