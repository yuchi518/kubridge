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

#include <linux/delay.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>	/* size_t */
#include <linux/slab.h> 	/* kmalloc() */
#include <linux/sched.h>

#include "kubridge_tasks.h"

#define BRIDGE_IDX		(1)

void task_event_complete(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	//printk("[task%d] task_event_complete\n", BRIDGE_IDX);
	kfree(payload);
}

int thread_function0(void *data)
{
	struct tsk1_data *td = NULL;
	int cnt, i;

	cnt = (int)data;

	for (i=0; i<cnt; i++)
	{
		td = kmalloc(sizeof(*td), GFP_KERNEL);
		td->d[0] = 0;
		td->d[1] = i;
		kub_send_event(BRIDGE_IDX, TSK1_DATA/*, sizeof(struct kub_test_str)*/, td, task_event_complete);
		mdelay(1);
		//yield();
	}

	return 0;
}

int thread_function1(void *data)
{
	struct tsk1_data *td = NULL;
	int cnt, i;

	cnt = (int)data;

	for (i=0; i<cnt; i++)
	{
		td = kmalloc(sizeof(*td), GFP_KERNEL);
		td->d[0] = 1;
		td->d[1] = i;
		kub_send_event(BRIDGE_IDX, TSK1_DATA/*, sizeof(struct kub_test_str)*/, td, task_event_complete);
		mdelay(1);
		//yield();
	}

	return 0;
}

void task_event_handler(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	int cnt;
	struct task_struct *task0, *task1;

	cnt = *(int*)payload;
	printk("[task%d] task_event_handler ready to gen %d data.\n", BRIDGE_IDX, cnt);

	//task = kthread_create(&thread_function,(void *)cnt,"pradeep");
   task0 = kthread_run(&thread_function0,(void *)cnt,"kb_task1_th0");
   task1 = kthread_run(&thread_function1,(void *)cnt,"kb_task1_th0");
	
	printk("[task%d] task_event_handler done\n", BRIDGE_IDX);
}

static int __init kubridge_tasks_init(void)
{
	kub_register_event_listener(BRIDGE_IDX, TSK1_GEN_CMD/*, sizeof(struct kub_test_str)*/, task_event_handler);
	return 0;
}

static void __exit kubridge_tasks_exit(void)
{

}


module_init(kubridge_tasks_init);
module_exit(kubridge_tasks_exit);
MODULE_AUTHOR("Yuchi");
MODULE_LICENSE("GPL");
