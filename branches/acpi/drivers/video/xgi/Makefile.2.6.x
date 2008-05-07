    obj-m	:= XGIfb.o
    XGIfb-objs := XGI_main.o XGI_accel.o vb_init.o vb_setmode.o vb_util.o vb_ext.o

    KDIR	:= /lib/modules/$(shell uname -r)/build
    PWD		:= $(shell pwd)

    default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
