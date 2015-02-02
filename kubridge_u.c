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
#include <semaphore.h> 
#include <unistd.h>
#include <poll.h>
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

// dev
struct kubridge_device {
	sem_t sem;
	int fd;
	struct kub_event_listener_info *listeners;
};

static struct kubridge_device *kub_devices=NULL;

//static sem_t mutex;

int kub_register_event_listener(int dev_no, IOCtlCmd cmd/*, size_t sizeOfPayload*/, kub_event_handler listener)
{
	struct kub_event_listener_info *li;
	int res=0;

	sem_wait(&kub_devices[dev_no].sem);

	HASH_FIND_INT(kub_devices[dev_no].listeners, &cmd, li);
	if (li)
	{
		HASH_DEL(kub_devices[dev_no].listeners, li);
		free(li->buff);
		free(li);
		li = NULL;
	}

	li = malloc(sizeof(*li));
	if (!li) {
		res = -1;
		goto end;
	}

	if (listener==NULL)
		goto end;

	li->cmd = cmd;
	li->sizeOfPayload = _IOC_SIZE(cmd);//sizeOfPayload;
	li->listener = listener;
	li->buff = malloc(sizeof(li->sizeOfPayload));
	if (li->buff==NULL) {
		res = -1;
		free(li);
		goto end;
	}

	//printk("add listener bridge=%d, cmd=%d\n", bridge, cmd);

	HASH_ADD_INT(kub_devices[dev_no].listeners, cmd, li);

end:
	sem_post(&kub_devices[dev_no].sem);
	return res;
}

int kub_get_event_listener(struct kubridge_device *dev, IOCtlCmd cmd, struct kub_event_listener_info **info)
{
	int res=0;
	*info = NULL;

	sem_wait(&dev->sem);

	HASH_FIND_INT(dev->listeners, &cmd, *info);
//end:
	sem_post(&dev->sem);
	return res;
}

void kub_release_event_listeners(struct kubridge_device *dev)
{
	struct kub_event_listener_info *li=NULL, *tmp;
	//if (down_interruptible(&dev->sem))
	//	return -ERESTARTSYS;

	HASH_ITER(hh, dev->listeners, li, tmp) {
		printf("HT Del %d\n", li->cmd);
   	HASH_DEL(dev->listeners, li);  /* delete; users advances to next */
   	//free(li->buff);
   	//free(li);            /* optional- if you want to free  */
	}

	//up(&dev->sem);
	//return 0;
}

int kub_send_event(int dev_no, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload)
{
	int res = 0;

	sem_wait(&kub_devices[dev_no].sem);

	res = ioctl(kub_devices[dev_no].fd, cmd, payload);

	sem_post(&kub_devices[dev_no].sem);

	return res;
}

void kub_main_loop(volatile int *run_bits)
{
	int i,j;
	int timeout_msecs = 100;
	char dev_path[64];
	struct pollfd fds[KUB_NUM_OF_BRIDGES];
	int cmd_num;
	int max_cmds_buff = 0;
	IOCtlCmd *cmds_buff = NULL;
	struct kub_event_listener_info *li;

	//sem_init(&mutex, 1, 1);
	//sem_wait(&mutex);
	kub_devices = malloc(KUB_NUM_OF_BRIDGES * sizeof(struct kubridge_device));
	for (i=0; i < KUB_NUM_OF_BRIDGES; i++)
	{
		sem_init(&kub_devices[i].sem, 1, 1);
		kub_devices[i].listeners = NULL;
		sprintf(dev_path, "/dev/%s%d", DEV_NAME, i+KUB_DEV_NO_START);
		if ((kub_devices[i].fd = open(dev_path, O_RDWR)) < 0) goto SHUT_DOWN;
		fds[i].fd = kub_devices[i].fd;
		fds[i].events = POLLIN;
	}
	//sem_post(&mutex);

	while(*run_bits)
	{
		i = poll(fds, KUB_NUM_OF_BRIDGES, timeout_msecs);
		if (i>0)
		for (i=0; i < KUB_NUM_OF_BRIDGES; i++)
		{
			if (fds[i].revents & POLLIN)
			{
				//sem_wait(&kub_devices[i].sem);
				ioctl(fds[i].fd, IOC_READ_CMD_INFO, &cmd_num);
				printf("[%d] Read to read %d ioctl cmd\n", i, cmd_num);

				if (cmd_num > max_cmds_buff)
				{
					if (cmds_buff) free(cmds_buff);
					cmds_buff = malloc(sizeof(IOCtlCmd)*cmd_num);
					max_cmds_buff = cmd_num;
				}

				ioctl(fds[i].fd, IOC_READ_CMDS(cmd_num), cmds_buff);

				printf("[%d] Read %d ioctl cmd\n", i, cmd_num);

				for (j=0; j<cmd_num; j++)
				{
					if (cmds_buff[j] != 0)
					{
						kub_get_event_listener(&kub_devices[i], cmds_buff[j], &li);
						if (li)
						{
							ioctl(fds[i].fd, cmds_buff[j], li->buff);
							li->listener(i, cmds_buff[j], li->buff);
						}
					}
				}

				//sem_post(&kub_devices[i].sem);
			}
		}
	}

	i = KUB_NUM_OF_BRIDGES-1;
	//sem_destroy(&mutex);
SHUT_DOWN:
	for (; i >= 0; i--)
	{
		printf("%d",i);
		if (kub_devices[i].fd >= 0) close(kub_devices[i].fd);
		printf("-%d(%p)",i, kub_devices[i].listeners);
		kub_release_event_listeners(&kub_devices[i]);
		printf("-%d\n",i);
	}
	/*free(kub_devices);
	if (cmds_buff) free(cmds_buff);*/
}

