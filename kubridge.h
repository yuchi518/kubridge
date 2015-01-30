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

#ifndef KUB_MACIG					// You can change it in your makefile.
#define KUB_MACIG					((unsigned char)0xC9)
#endif
#define KUB_NUM_OF_BRIDGES		(1)
#define DEV_NAME					"kubridge"
typedef unsigned int IOCtlCmd;		// 8bits: type, 8bits: number, 13bits: size, 3bits:dir

// Reserved cmds in all bridges(devices).
#define IOC_READ_CMD_INFO		_IOR(KUB_MACIG, 0, int)		// int is not the only type 
#define IOC_READ_CMDS			_IOR(KUB_MACIG, 1, int)		// int is not the only type

/// === testing purpose
struct kub_test_str {
	int i, k;
};

#define READ_IOCTL				_IOR(KUB_MACIG, 2, struct kub_test_str)
#define WRITE_IOCTL				_IOW(KUB_MACIG, 3, struct kub_test_str)
/// === testing purpose end

#if __KERNEL__

typedef void (*kub_event_handler)(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload);

int kub_register_event_listener(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, kub_event_handler listener);
int kub_send_event(int bridge, IOCtlCmd cmd/*, size_t sizeOfPayload*/, void *payload, kub_event_handler complete);

#else

#endif

#endif

