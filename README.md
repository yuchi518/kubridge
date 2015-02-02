# KUBridge
Kernel/User mode data bridge. KUBridge uses IOCtrl to transfer commands between kernel module and user program.
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
If you are interested in how to write your kernel module and user programe, you can refer the kubridge_k_task0.c and kubridge_u_tasks.c

## Todo's

 - Many things.

## License
GPLv2
