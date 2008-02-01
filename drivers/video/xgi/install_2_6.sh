#!/bin/sh
install -m 644 XGIfb.ko /lib/modules/`uname -r`/kernel/drivers/video/XGIfb.ko
/sbin/depmod -a
modprobe XGIfb mode=800x600x32

