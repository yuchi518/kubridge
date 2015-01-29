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
#include <linux/kernel.h>	/* printk() */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>	/* size_t */

#include "kubridge.h"

struct task_test_str
{
	int i;
	int k;
};

struct task_test_str data;

void task_event_complete(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, void *payload)
{
	printk("task_event_complete\n");
}

void task_event_handler(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, void *payload)
{
	data = *(struct task_test_str*)payload;
	printk("task_event_handler (%d,%d)\n", data.i, data.k);
	data.i++;
	data.k++;
	kub_send_event(3, READ_IOCTL, sizeof(struct task_test_str), &data, task_event_complete);
	printk("task_event_handler done\n");
}

static int __init kubridge_tasks_init(void)
{
	kub_register_event_listener(3, WRITE_IOCTL, sizeof(struct task_test_str), task_event_handler);
	return 0;
}

static void __exit kubridge_tasks_exit(void)
{

}


module_init(kubridge_tasks_init);
module_exit(kubridge_tasks_exit);
MODULE_AUTHOR("Yuchi");
MODULE_LICENSE("GPL");
