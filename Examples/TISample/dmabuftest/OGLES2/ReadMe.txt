step1:
insmod /lib/modules/3.14.57+/extra/omapdrm_pvr.ko
step2:
pvrsrvinit
step3:
dmabuftest --multi 4 -d /dev/video1 -c 720x240@YUYV -d /dev/video2 -c 720x240@YUYV -d /dev/video3 -c 720x240@YUYV -d /dev/video4 -c 720x240@YUYV --kmscube --connector 4 --fov 20&
