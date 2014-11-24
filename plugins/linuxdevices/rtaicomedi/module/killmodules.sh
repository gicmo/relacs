#! /bin/bash
lsmod | grep -q dynclampmodule && rmmod dynclampmodule && echo "removed dynclampmodule"
lsmod | grep -q rtmodule && rmmod rtmodule && echo "removed rtmodule"

modprobe -r kcomedilib && echo "removed kcomedilib"

lsmod | grep -q rtai_math && { rmmod rtai_math && echo "removed rtai_math"; }
lsmod | grep -q rtai_fifos && { rmmod rtai_fifos && echo "removed rtai_fifos"; }
lsmod | grep -q rtai_sched && { rmmod rtai_sched && echo "removed rtai_sched"; }
lsmod | grep -q rtai_hal && { rmmod rtai_hal && echo "removed rtai_hal"; }