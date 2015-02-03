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

// ==== task 0 ====
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
	
	sleep(1); // wait main loop running

	printf("run task0\n");
	
	kub_register_event_listener(dev_no, TSK0_ADD_RES, tsk0_result);
	kub_register_event_listener(dev_no, TSK0_SUB_RES, tsk0_result);
	kub_register_event_listener(dev_no, TSK0_MUL_RES, tsk0_result);
	kub_register_event_listener(dev_no, TSK0_DIV_RES, tsk0_result);

	printf("run task0 registered\n");

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

	sleep(2);
	((char*)&run_bits)[0] = 0;
}

/// ==== task 1 ====
int hitCnt;
void tsk1_result(int dev_no, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	struct tsk1_data data = *(struct tsk1_data*)payload;
	printf("[%d] gen %d-%d\n", dev_no, data.d[0], data.d[1]);

	if (data.d[1]==(1000-1)) hitCnt--;

	if (hitCnt==0)
		((char*)&run_bits)[1] = 0;
}

void *runTask1(void *arg)
{
	int dev_no = (int)arg;
	int genDataCnt = 1000;
	hitCnt = 2;

	sleep(1); // wait main loop running

	printf("run task1\n");
	kub_register_event_listener(dev_no, TSK1_DATA, tsk1_result);
	printf("run task1 registered\n");
	kub_send_event(dev_no, TSK1_GEN_CMD, &genDataCnt);
	printf("run task1 sent event\n");
}

/// ==== task end ====

int main()
{
	pthread_t t[4]={0,0,0,0};
	int i;
	printf("hello\n");

	((char*)&run_bits)[0] = 1;
	((char*)&run_bits)[1] = 1;
	((char*)&run_bits)[2] = 0;
	((char*)&run_bits)[3] = 0;

	// in fact, pthread should run after kub_main_loop runs.
	// Because
	pthread_create(&t[0], NULL, runTask0, (void*)(0));
	pthread_create(&t[1], NULL, runTask1, (void*)(1));

	kub_main_loop(&run_bits);

	return 0;
}

