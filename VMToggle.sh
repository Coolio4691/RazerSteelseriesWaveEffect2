#!/bin/bash

if [ "${1}" = "off" ]; then

#kill barrier
killall barrier
killall barriers

#enable monitor to linux

nvidia-settings -a CurrentMetaMode="DPY-0: 1280x1024_75 @1280x1024 +0+56 {ViewPortIn=1280x1024, ViewPortOut=1280x1024+0+0, ForceCompositionPipeline=On, ForceFullCompositionPipeline=On}, DPY-2: 1920x1080_144 @1920x1080 +1280+0 {ViewPortIn=1920x1080, ViewPortOut=1920x1080+0+0}"

#swap monitor input to hdmi-2

ddcutil -d "2" setvcp 60 "0x12"

virsh shutdown $2 # send shutdown

exit
fi

#disable monitor to linux
nvidia-settings -a CurrentMetaMode="DPY-0: 1280x1024_75 @1280x1024 +0+0 {ViewPortIn=1280x1024, ViewPortOut=1280x1024+0+0, ForceCompositionPipeline=On, ForceFullCompositionPipeline=On}"

sleep 2

#unmount drives to be used for the virtual machine
umount -fq /media/Storage
umount -fq /dev/nvme0n1p3

#start barrier if it isnt already started
pgrep -x barrier > /dev/null || barrier > /dev/null 2>&1 & disown

virsh start $2
#swap monitor input to displayport
ddcutil -d "2" setvcp 60 "0x0f"

exit