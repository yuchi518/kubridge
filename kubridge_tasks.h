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

#ifndef _KUBRIDGE_TASKS_H_
#define _KUBRIDGE_TASKS_H_

#include "kubridge.h"

/// === testing purpose
struct kub_test_str {
	int i, k;
};

#define READ_IOCTL				_IOR(KUB_MAGIC, 2, struct kub_test_str)
#define WRITE_IOCTL				_IOW(KUB_MAGIC, 3, struct kub_test_str)


// task 
struct tsk0_str {
	int op;				// 0:add, 1:sub, 2:mul, 3:div
	int a, b;
};

struct tsk0_res {
	int op;
	int a, b, r;
};

#define TSK0_ADD_CMD				_IOW(KUB_MAGIC, 4, struct tsk0_str)
#define TSK0_ADD_RES				_IOR(KUB_MAGIC, 4, struct tsk0_res)
#define TSK0_SUB_CMD				_IOW(KUB_MAGIC, 5, struct tsk0_str)
#define TSK0_SUB_RES				_IOR(KUB_MAGIC, 5, struct tsk0_res)
#define TSK0_MUL_CMD				_IOW(KUB_MAGIC, 6, struct tsk0_str)
#define TSK0_MUL_RES				_IOR(KUB_MAGIC, 6, struct tsk0_res)
#define TSK0_DIV_CMD				_IOW(KUB_MAGIC, 7, struct tsk0_str)
#define TSK0_DIV_RES				_IOR(KUB_MAGIC, 7, struct tsk0_res)

/// === testing purpose end

#endif


