#!/bin/sh

module="kubridge_k"
device="kubridge"
task_module="kubridge_k_tasks"
mode="664"

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
#insmod ./$module.ko $* || exit 1
#insmod ./$module.ko major=299 minor=7 num_devs=4 || exit 1
insmod ./$module.ko minor=7 num_devs=4 || exit 1
insmod ./${task_module}0.ko
insmod ./${task_module}1.ko
insmod ./${task_module}2.ko
insmod ./${task_module}3.ko

# remove stale nodes
#rm -f /dev/${device}[0-3]
#sleep 1
#major=`cat /proc/devices | awk "\\\$2==\"$module\" {print \\\$2}"`
major=`cat /proc/devices | awk "{if (\\\$2==\"$device\") {print \\\$1}}"`
echo $major
#mknod /dev/${device}0 c $major 7
#mknod /dev/${device}1 c $major 8
#mknod /dev/${device}2 c $major 9
#mknod /dev/${device}3 c $major 10

# give appropriate group/permissions, and change the group.
# Not all distributions have staff, some have "wheel" instead.
#group="staff"
#grep -q '^staff:' /etc/group || group="wheel"
#chgrp $group /dev/${device}[0-3]
#chmod $mode /dev/${device}[0-3]


