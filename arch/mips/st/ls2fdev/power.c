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

int acpi_sleep_prepare(void)
{
	__flush_cache_all();
	local_flush_tlb_all();
	return 0;
}

static struct pm_ops acpi_pm_ops;


extern void do_suspend_lowlevel(void);

/**
 *	acpi_pm_set_target - Set the target system sleep state to the state
 *		associated with given @pm_state, if supported.
 */

static int acpi_pm_set_target(suspend_state_t pm_state)
{
	printk("acpi_pm_set_target\n");
	return 0;
}

/**
 *	acpi_pm_prepare - Do preliminary suspend work.
 *	@pm_state: ignored
 *
 *	If necessary, set the firmware waking vector and do arch-specific
 *	nastiness to get the wakeup code to the waking vector.
 */

static int acpi_pm_prepare(suspend_state_t pm_state)
{
	acpi_sleep_prepare();
	printk("acpi_pm_prepare\n");
	return 0;
}

/**
 *	acpi_pm_enter - Actually enter a sleep state.
 *	@pm_state: ignored
 *
 *	Flush caches and go to sleep. For STR we have to call arch-specific
 *	assembly, which in turn call acpi_enter_sleep_state().
 *	It's unfortunate, but it works. Please fix if you're feeling frisky.
 */


extern void trap_init(void);

extern int g_debug_dumpstack;
extern int vga_bios_post(void);

static int acpi_pm_enter(suspend_state_t pm_state)
{

//	unsigned long c,i;
	struct pci_dev *pdev = NULL;
	unsigned long flags = 0;
	//printk("acpi_pm_enter\n");
	
	save_nb_registers();

	__flush_cache_all();

	local_irq_save(flags);

	do_suspend_lowlevel();


	/*
	
	for (i = 0; i < 64; i += 4) {
		pci_read_config_dword(pdev, i, (u32 *)&c);
		printk("	%lx:%lx", i, c);
		if ((i + 4 ) % 0x10 == 0)
			printk("\n");
	}

	for (i = 0; i < 0x98; i += 4) {
		if ( i % 0x10 == 0)
			printk("\n");
		printk("%2x:%8x ", (unsigned int)i, *(volatile unsigned int *) (0xffffffffbfe00100 + i));
	}
	printk("\n");

	*(volatile unsigned int *)(0xffffffffbfe00100+0x10) = 0x2040;
	*(volatile unsigned int *)(0xffffffffbfe00100+0x90) = 0x404040;
	
	for (i = 0; i < 0x98; i += 4) {
		if ( i % 0x10 == 0)
			printk("\n");
		printk("%2x:%8x ", (unsigned int)i, *(volatile unsigned int *) (0xffffffffbfe00100 + i));
	}
	printk("\n");

	printk("sizeof unsigned long = %d\n", (int)sizeof(unsigned long));



	vga_bios_post();
*/
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

	BootVideoCardBIOS(pdev,  0);
	local_irq_restore(flags);

	//printk(KERN_DEBUG "Back to C!\n");
	//printk(KERN_INFO "acpi_pm_enter exit\n");
//	g_debug_dumpstack = 1;

	return 0;
}

/**
 *	acpi_pm_finish - Finish up suspend sequence.
 *	@pm_state: ignored
 *
 *	This is called after we wake back up (or if entering the sleep state
 *	failed). 
 */

static int acpi_pm_finish(suspend_state_t pm_state)
{
	struct pci_dev *pdev = NULL;

	for_each_pci_dev(pdev) {
		if ((pdev->class >> 8) == PCI_CLASS_DISPLAY_VGA) {
			printk(KERN_INFO "pci->class = %x\n", pdev->class);
			break;
		}
	}

/*
	//pci_restore_state(pdev);

	//pci_reenable_device(pdev);
	
	for (i = 0; i < 64; i += 4) {
		pci_read_config_dword(pdev, i, (u32 *)&c);
		printk("	%lx:%lx", i, c);
		if ((i + 4 ) % 0x10 == 0)
			printk("\n");
	}
	vga_bios_post(pdev);
*/
	//BootVideoCardBIOS(pdev,  0);

	printk("acpi_pm_finish\n");
	return 0;
}

static int acpi_pm_state_valid(suspend_state_t pm_state)
{

	printk("acpi_pm_state_valid\n");

	switch (pm_state) {
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

static struct pm_ops acpi_pm_ops = {
	.valid = acpi_pm_state_valid,
	.set_target = acpi_pm_set_target,
	.prepare = acpi_pm_prepare,
	.enter = acpi_pm_enter,
	.finish = acpi_pm_finish,
};


static int __init acpi_sleep_init(void)
{

	pm_set_ops(&acpi_pm_ops);

	printk("acpi_sleep_init done!\n");
	return 0;
}

static void __exit acpi_sleep_exit(void)
{
}


module_init(acpi_sleep_init);
module_exit(acpi_sleep_exit);
MODULE_AUTHOR("Aimeng");
MODULE_DESCRIPTION("ACPI Module for Loongson 2E/2F Board");
MODULE_LICENSE("GPL");

