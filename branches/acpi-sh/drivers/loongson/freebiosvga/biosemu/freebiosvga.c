/* Use biosemu/x86emu to POST VGA cards
 *
 * Copyright 2002 Fuxin Zhang,BaoJian Zheng
 * Institute of Computing Technology,Chinese Academy of Sciences,China 
 */

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/io.h>
#include "biosemui.h"

#include "z9sl10810fw.h"
/* Length of the BIOS image */
#define MAX_BIOSLEN         (64 * 1024L)

static	u32              debugFlags = 0;
static	BE_VGAInfo       VGAInfo[1] = {{0}};

static	RMREGS          regs;
static  RMSREGS         sregs;


int vga_bios_post(struct pci_dev *pdev)
{
	unsigned long romsize = 0;
	unsigned long romaddress = 0;

	romaddress = z9sl10810_rom_data;
	romsize = (readb(romaddress + 2)) * 512;

	memset(VGAInfo,0,sizeof(BE_VGAInfo));
	VGAInfo[0].pciInfo = pdev;
	VGAInfo[0].BIOSImage = (void*)kmalloc(romsize, GFP_KERNEL);
	if (VGAInfo[0].BIOSImage == NULL) {
		printk("Error alloc memory for vgabios\n");
		return -1;
	}
	VGAInfo[0].BIOSImageLen = romsize;
	memcpy(VGAInfo[0].BIOSImage,(char*)romaddress,romsize);

	BE_init(debugFlags,65536,&VGAInfo[0]);

	regs.h.ah = 0;
	regs.h.al = pdev->devfn;

	BE_callRealMode(0xC000,0x0003,&regs,&sregs);


	printk("vgabios_init: Emulation done\n");
	kfree(VGAInfo[0].BIOSImage);
	return 1;


}
