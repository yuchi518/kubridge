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

#ifndef _KUBRIDGE_H_
#define _KUBRIDGE_H_

#define KUB_MACIG					((unsigned char)0x73)
#define KUB_NUM_OF_BRIDGES		(1)
#define READ_IOCTL				_IOR(KUB_MACIG, 0, int)
#define WRITE_IOCTL				_IOW(KUB_MACIG, 1, int)
#define DEV_NAME					"kubridge"
typedef int IOCtlCmd;

#if __KERNEL__

typedef void (*kub_event_handler)(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, void *payload);

int kub_register_event_listener(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, kub_event_handler listener);
int kub_send_event(int bridge, IOCtlCmd cmd, size_t sizeOfPayload, void *payload, kub_event_handler complete);

#else

#endif

#endif

