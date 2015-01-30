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

#include "kubridge.h"

void* runTask(void *arg)
{
	//char buf[200];
	int fd = -1;
	int i;
	int index = (int)arg;
	char dev_path[32];
	printf("hello %d\n", index);

	sprintf(dev_path, "/dev/%s%d", DEV_NAME, index);
	printf("%s running\n", dev_path);

	if ((fd = open(dev_path, O_RDWR)) < 0) {
		perror("open");
		return NULL;
	}

	struct kub_test_str d;
	d.i = 0;
	d.k = 1;

	for (i=0; i<1000; i++)
	{
		if(ioctl(fd, WRITE_IOCTL, &d) < 0)
			perror("first ioctl\n");

		printf("[%d] data i=%d, k=%d\n", index, d.i, d.k);
		usleep(1000);

		if(ioctl(fd, READ_IOCTL, &d) < 0)
			perror("second ioctl\n");
	}

	close(fd);

	printf("%s close\n", dev_path);

	return NULL;
}

void *runTest(void* arg)
{
	printf("runTest %d\n", (int)arg);
}

int main()
{
	pthread_t t[4]={0,0,0,0};
	int i;
	printf("hello\n");

	for (i=0; i<4; i++)
	{
		//runTask((void*)i);
		pthread_create(&t[i], NULL, runTask, (void*)i);
		//pthread_create(&t[i], NULL, runTest, (void*)i);
	}

	for (i=0; i<4; i++)
	{
		pthread_join(t[i], NULL);
	}
	

	return 0;
}

