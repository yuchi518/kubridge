#!/bin/sh
module="kubridge_k"
device="kubridge"
task_module="kubridge_k_tasks"

#rm -f /dev/${device}[0-3]
rmmod ./${task_module}[0-3].ko
rmmod ./$module.ko
