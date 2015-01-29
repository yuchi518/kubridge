#!/bin/sh
module="kubridge_k"
device="kubridge"

rm -f /dev/${device}[0-3]
rmmod ./$module.ko
rmmod ./kubridge_k_tasks.ko
