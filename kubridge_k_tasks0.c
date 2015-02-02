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
#include <linux/slab.h> /* kmalloc() */

#include "kubridge_tasks.h"

#define BRIDGE_IDX		(0)

void task_event_complete(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	printk("[task%d] task_event_complete\n", BRIDGE_IDX);
	kfree(payload);
}

void task_event_handler(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	char op;
	IOCtlCmd r_cmd;
	struct tsk0_res *res = kmalloc(sizeof(struct tsk0_res), GFP_KERNEL);
	*(struct tsk0_str*)res = *(struct tsk0_str*)payload;		// make sure the memory space is valid
	
	switch(res->op)
	{
	case 0: // add
		op = '+';
		res->r = res->a + res->a;
		r_cmd = TSK0_ADD_RES;
		break;
	case 1: // sub
		op = '-';
		res->r = res->a - res->a;
		r_cmd = TSK0_SUB_RES;
		break;
	case 2: // mul
		op = '*';
		res->r = res->a * res->a;
		r_cmd = TSK0_MUL_RES;
		break;
	case 3: // div
		op = '/';
		res->r = res->a / res->a;
		r_cmd = TSK0_DIV_RES;
		break;
	default:
		op = '?';
		res->r = 0;
		r_cmd = 0;
		break;
	}

	printk("[task%d] task_event_handler (%d %c %d = %d)\n", BRIDGE_IDX, res->a, op, res->b, res->r);
	if (r_cmd) kub_send_event(BRIDGE_IDX, r_cmd/*, sizeof(struct kub_test_str)*/, res, task_event_complete);
	printk("[task%d] task_event_handler done\n", BRIDGE_IDX);
}

static int __init kubridge_tasks_init(void)
{
	kub_register_event_listener(BRIDGE_IDX, TSK0_ADD_CMD/*, sizeof(struct kub_test_str)*/, task_event_handler);
	kub_register_event_listener(BRIDGE_IDX, TSK0_SUB_CMD/*, sizeof(struct kub_test_str)*/, task_event_handler);
	kub_register_event_listener(BRIDGE_IDX, TSK0_MUL_CMD/*, sizeof(struct kub_test_str)*/, task_event_handler);
	kub_register_event_listener(BRIDGE_IDX, TSK0_DIV_CMD/*, sizeof(struct kub_test_str)*/, task_event_handler);
	return 0;
}

static void __exit kubridge_tasks_exit(void)
{

}


module_init(kubridge_tasks_init);
module_exit(kubridge_tasks_exit);
MODULE_AUTHOR("Yuchi");
MODULE_LICENSE("GPL");
