obj-$(CONFIG_LS_VGA_POST) += vga_post.o
vga_post-objs := atibios.o biosemu.o besys.o bios.o x86emu/decode.o x86emu/ops2.o x86emu/ops.o x86emu/prim_ops.o x86emu/sys.o x86emu/debug.o

EXTRA_CFLAGS += -DCONFIG_STLS -D__KERNEL__ -DCONFIG_BIOSEMU -DCFG_LEGACY_VGA_SUPPORT

#########################################################################
