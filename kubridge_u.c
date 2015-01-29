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

#include "kubridge.h"

struct task_test_str
{
	int i;
	int k;
};

int main(){
	//char buf[200];
	int fd = -1;
	int i;

	if ((fd = open("/dev/" DEV_NAME "3", O_RDWR)) < 0) {
		perror("open");
		return -1;
	}

	struct task_test_str d;
	d.i = 0;
	d.k = 1;

	for (i=0; i<10; i++)
	{
		if(ioctl(fd, WRITE_IOCTL, &d) < 0)
			perror("first ioctl");

		printf("data i=%d, k=%d\n", d.i, d.k);

		if(ioctl(fd, READ_IOCTL, &d) < 0)
			perror("second ioctl");
	}

	/*
	if(ioctl(fd, WRITE_IOCTL, "hello world") < 0)
		perror("first ioctl");

	if(ioctl(fd, READ_IOCTL, buf) < 0)
		perror("second ioctl");

	printf("message: %s\n", buf);
	*/
	return 0;
}

