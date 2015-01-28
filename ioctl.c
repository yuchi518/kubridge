#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#define MY_MACIG 'G'
#define READ_IOCTL _IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL _IOW(MY_MACIG, 1, int)
 
static int major=0, minor=0, num_devs=2; 
#define DEV_NAME		"my_device"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
#else
#include <linux/cdev.h>
static struct cdev dev;
#endif

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
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int  device_ioctl(struct inode *inode, struct file *filep, unsigned int cmd, unsigned long arg)
#else
static long device_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
#endif
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
	.read = device_read, 
	.write = device_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = device_ioctl,
#else
	.unlocked_ioctl = device_ioctl,
#endif
	
};

static int __init cdevexample_module_init(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	major = register_chrdev(major, DEV_NAME, &fops);
	if (major < 0) {
     		printk ("Registering the character device failed with %d\n", major);
	     	return major;
	}
#else
	int res;
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
	}
	if (res<0)
	{
		printk(DEV_NAME " can't get major = %d\n", major);
		return res;
	}

	cdev_init(&dev, &fops);
	dev.owner = THIS_MODULE;
	dev.ops = &fops;
	res = cdev_add(&dev, devno, num_devs);
	if (res)
	{
		printk(KERN_NOTICE " " DEV_NAME  " Error %d", res);
		return res;
	}
#endif

	printk("cdev example: assigned major: %d\n", major);
	printk("create node with mknod /dev/cdev_example c %d %d\n", major, minor);
 	return 0;
}

static void __exit cdevexample_module_exit(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	unregister_chrdev(major, DEV_NAME);
#else
	cdev_del(&dev);
#endif
}  

module_init(cdevexample_module_init);
module_exit(cdevexample_module_exit);
MODULE_LICENSE("GPL");
