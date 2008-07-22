/*
 * sleep.c - ACPI sleep support.
 *
 * Copyright (c) 2005 Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>
 * Copyright (c) 2004 David Shaohua Li <shaohua.li@intel.com>
 * Copyright (c) 2000-2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 *
 * This file is released under the GPLv2.
 *
 */

#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/dmi.h>
#include <linux/device.h>
#include <linux/suspend.h>
#include <linux/pci.h>
#include <asm/cacheflush.h>


#include <asm/io.h>

extern void local_flush_tlb_all(void);
extern int BootVideoCardBIOS(struct pci_dev *pcidev, int cleanUp);


u32 saved_nb_registers[52];
u32 saved_nb_pci_header[64];

void save_nb_registers(void) {
	int i;

	for (i = 0; i < 52; i++) {
		saved_nb_registers[i] = *(volatile u32 *)(0xffffffffbfe00100 + i * 4);
		if (i == 14) {
			printk("INTEN: Offset 0x%x:0x%x\n", i*4, saved_nb_registers[i]);
		}
		if (i == 0x20) {
			printk("Chip_config0: Offset 0x%x:0x%x\n", i*4, saved_nb_registers[i]);
		}
	}

	for (i = 0; i < 64; i++) {
		saved_nb_pci_header[i] = *(volatile u32 *)(0xffffffffbfe00000 + i * 4);
	}
}


void restore_nb_registers(void) {
	int i;
	for (i = 0; i < 52; i++) {
		if ((*(volatile u32 *)(0xffffffffbfe00100 + i * 4)) != saved_nb_registers[i]) {
			if (i == 0xe) {
				*(volatile u32 *)(0xffffffffbfe00134) = ~0; 
				*(volatile u32 *)(0xffffffffbfe00130) = saved_nb_registers[i]; 
			}
			
			if (i >= 0xc && i <= 0xf)
				continue;

			*(volatile u32 *)(0xffffffffbfe00100 + i * 4) = saved_nb_registers[i];
			printk("NB IO Writing Back 0x%x with 0x%x\n", i*4, saved_nb_registers[i]);
		}
	}
	for (i = 0; i < 64; i++) {
		if ((*(volatile u32 *)(0xffffffffbfe00000 + i * 4)) != saved_nb_pci_header[i]) {
			*(volatile u32 *)(0xffffffffbfe00000 + i * 4) = saved_nb_pci_header[i];
			printk("NB pci header Writing Back 0x%x with 0x%x\n", i*4, saved_nb_pci_header[i]);
		}
	}
}

int ls_sleep_prepare(void)
{
	__flush_cache_all();
	local_flush_tlb_all();
	return 0;
}

static struct pm_ops ls_pm_ops;


extern void do_suspend_lowlevel(void);

/**
 *	ls_pm_set_target - Set the target system sleep state to the state
 *		associated with given @pm_state, if supported.
 */

static int ls_pm_set_target(suspend_state_t pm_state)
{
	printk("ls_pm_set_target\n");
	return 0;
}

/**
 *	ls_pm_prepare - Do preliminary suspend work.
 *	@pm_state: ignored
 *
 *	If necessary, set the firmware waking vector and do arch-specific
 *	nastiness to get the wakeup code to the waking vector.
 */

static int ls_pm_prepare(suspend_state_t pm_state)
{
	ls_sleep_prepare();
	printk("ls_pm_prepare\n");
	return 0;
}

/**
 *	ls_pm_enter - Actually enter a sleep state.
 *	@pm_state: ignored
 *
 *	Flush caches and go to sleep. For STR we have to call arch-specific
 *	assembly, which in turn call ls_enter_sleep_state().
 *	It's unfortunate, but it works. Please fix if you're feeling frisky.
 */


extern void trap_init(void);

extern int g_debug_dumpstack;
extern int vga_bios_post(void);

static int ls_pm_enter(suspend_state_t pm_state)
{

	struct pci_dev *pdev = NULL;
	unsigned long flags = 0;
	
	save_nb_registers();

	__flush_cache_all();

	local_irq_save(flags);

	do_suspend_lowlevel();

	local_flush_tlb_all();
	restore_nb_registers();
	
	for_each_pci_dev(pdev) {
		if ((pdev->class >> 8) == PCI_CLASS_DISPLAY_VGA) {
			printk(KERN_INFO "pci->class = %x\n", pdev->class);
			break;
		}
	}

	pci_restore_state(pdev);

	pci_reenable_device(pdev);

	BootVideoCardBIOS(pdev,  1);
	local_irq_restore(flags);

	return 0;
}

/**
 *	ls_pm_finish - Finish up suspend sequence.
 *	@pm_state: ignored
 *
 *	This is called after we wake back up (or if entering the sleep state
 *	failed). 
 */

static int ls_pm_finish(suspend_state_t pm_state)
{
	printk("ls_pm_finish\n");
	return 0;
}

static int ls_pm_state_valid(suspend_state_t pm_state)
{

	printk("ls_pm_state_valid\n");

	switch (pm_state) {
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

static struct pm_ops ls_pm_ops = {
	.valid = ls_pm_state_valid,
	.set_target = ls_pm_set_target,
	.prepare = ls_pm_prepare,
	.enter = ls_pm_enter,
	.finish = ls_pm_finish,
};


static int __init ls_sleep_init(void)
{

	pm_set_ops(&ls_pm_ops);

	printk("ls_sleep_init done!\n");
	return 0;
}

static void __exit ls_sleep_exit(void)
{
}


module_init(ls_sleep_init);
module_exit(ls_sleep_exit);
MODULE_AUTHOR("Aimeng");
MODULE_DESCRIPTION("ACPI Module for Loongson 2E/2F Board");
MODULE_LICENSE("GPL");

