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
You will see 4 threads try to communicate with kernel module at same time.

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
