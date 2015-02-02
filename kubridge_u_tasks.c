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

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#include "kubridge_tasks.h"

int run_bits;

void tsk0_result(int dev_no, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	char op;
	struct tsk0_res res = *(struct tsk0_res*)payload;
	switch(res.op)
	{
		case 0: op = '+'; break;
		case 1: op = '-'; break;
		case 2: op = '*'; break;
		case 3: op = '/'; break;
		default: op = '?'; break;
	}

	printf("[%d] %d %c %d = %d\n", dev_no, res.a, op, res.b, res.r);
}

void *runTask0(void *arg)
{
	int dev_no = (int)arg;
	struct tsk0_str tsk;
	int i;

	kub_register_event_listener(dev_no, TSK0_ADD_RES, tsk0_result);
	kub_register_event_listener(dev_no, TSK0_SUB_RES, tsk0_result);
	kub_register_event_listener(dev_no, TSK0_MUL_RES, tsk0_result);
	kub_register_event_listener(dev_no, TSK0_DIV_RES, tsk0_result);

	for (i=0; i<10000; i++)
	{
		tsk.a = i;
		tsk.b = i+1;
		tsk.op = i % 4;
		switch(tsk.op)
		{
		case 0:
			kub_send_event(dev_no, TSK0_ADD_CMD, &tsk);
			break;
		case 1:
			kub_send_event(dev_no, TSK0_SUB_CMD, &tsk);
			break;
		case 2:
			kub_send_event(dev_no, TSK0_MUL_CMD, &tsk);
			break;
		case 3:
			kub_send_event(dev_no, TSK0_DIV_CMD, &tsk);
			break;
		default:
			break;
		}
	}

	sleep(5);
	((char*)&run_bits)[0] = 0;
}

int main()
{
	pthread_t t[4]={0,0,0,0};
	int i;
	printf("hello\n");

	pthread_create(&t[0], NULL, runTask0, (void*)(0));

	((char*)&run_bits)[0] = 1;
	((char*)&run_bits)[1] = 0;
	((char*)&run_bits)[2] = 0;
	((char*)&run_bits)[3] = 0;

	kub_main_loop(&run_bits);

	return 0;
}

