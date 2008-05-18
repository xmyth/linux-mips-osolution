/*
 * ACPI Module for Loongson-based machines
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

extern int vga_bios_post(struct pci_dev *pdev);

static int __init vga_post_module_init(void)
{
	struct pci_dev *pdev = NULL;

	for_each_pci_dev(pdev) {
		if ((pdev->class >> 8) == PCI_CLASS_DISPLAY_VGA) {
			printk(KERN_INFO "pci->class = %x\n", pdev->class);
			break;
		}
	}
	
	vga_bios_post(pdev);

	printk(KERN_INFO "vga_post: initialized.\n");

	return 0;
}

static void __exit vga_post_module_exit(void)
{
	printk(KERN_INFO "vga_post: removed.\n");
}

module_init(vga_post_module_init);
module_exit(vga_post_module_exit);

MODULE_AUTHOR("West");
MODULE_DESCRIPTION("VGA POST Test Module for Loongson 2E/2F Board");
MODULE_LICENSE("GPL");

