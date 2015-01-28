#ifndef _KUBRIDGE_H_
#define _KUBRIDGE_H_

#define MY_MACIG		((unsigned char)0x73)
#define READ_IOCTL	_IOR(MY_MACIG, 0, int)
#define WRITE_IOCTL	_IOW(MY_MACIG, 1, int)
#define DEV_NAME		"kubridge"

#endif

