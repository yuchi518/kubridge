#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "kubridge.h"

int main(){
	char buf[200];
	int fd = -1;
	if ((fd = open("/dev/" DEV_NAME "3", O_RDWR)) < 0) {
		perror("open");
		return -1;
	}

	if(ioctl(fd, WRITE_IOCTL, "hello world") < 0)
		perror("first ioctl");

	if(ioctl(fd, READ_IOCTL, buf) < 0)
		perror("second ioctl");

	printf("message: %s\n", buf);
	return 0;
}

