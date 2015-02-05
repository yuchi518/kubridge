# KUBridge
Kernel/User mode bridge. KUBridge uses IOCtrl to transfer commands(short message) between kernel module and user program.
I try to let the interface as simple as possible.

## Version
0.2

## Tech

KUBridge uses a number of open source projects to work properly:

* [uthash] - A hash lib in C lang. I modified a little code to support kernel mode. (Link: http://troydhanson.github.io/uthash/)
* [list] - double linked list, comes from linux kernel source.

## Installation
You should understand how to compile the kernel module and what tools you should install first. I test the code in Ubuntu 12.04.

Make the test code.
```sh
$ make
```
Install the kernel module.
```sh
$ sudo ./load.sh
```
Run the user program.
```sh
$sudo ./kubridge_u
```
You will see 2 tasks try to communicate with kernel module at same time.

You can see the kubridge usage by following command.
```
$cat /proc/kubinfo
kubridge usage:
[0] listen 10000 times, send  10000 times
[1] listen 1 times, send  2000 times
[2] listen 0 times, send  0 times
[3] listen 0 times, send  0 times
```
or
```
$cat /sys/kernel/kub/info/0/usage
kubridge usage:
[0] listen 10000 times, send 10000 tiems.
$cat /sys/kernel/kub/info/1/usage
kubridge usage:
[1] listen 1 times, send 2000 tiems.
```

Uninstall the kernel module.
```sh
$sudo ./unload.sh
```

## Development
If you are interested in how to write your kernel module and user programe, you can refer the __kubridge_k_task[0-1].c__, __kubridge_u_tasks.c__ and __kubridge_u.h__.

* task0 - A calculator, kernel module supports four operators (+,-,*,/).
* task1 - Data generator, kernel module generates data continually.

In your kernel module, you should only includes following 4 fiels. Make sure compiling these codes with a predefine `__KERNEL__` (Add it in your makefile). 
```
kubrideg.h kubridge_k.c list.h uthash.h 
```
In your user programe, you should includes following 4 files.
```
kubridge.h kubridge_u.c list.h uthash.h
```

## Todo's

 - Many things.

## License
GPLv2
