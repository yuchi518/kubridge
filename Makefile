obj-m += ioctl.o


kernel:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

user:
	gcc ioctl_user.c -o ioctl_user

all: kernel user


clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm ioctl_user