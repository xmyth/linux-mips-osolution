#
# Makefile for the XGI framebuffer device driver
#

O_TARGET := XGIfb.o

export-objs    :=  XGI_main.o

obj-y    := XGI_main.o  vb_init.o vb_setmode.o vb_util.o vb_ext.o XGI_accel.o	

obj-m    := $(O_TARGET)

include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o
