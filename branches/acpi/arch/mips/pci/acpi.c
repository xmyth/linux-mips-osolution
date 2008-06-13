#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/irq.h>
//#include <asm/numa.h>
//#include "pci.h"

struct pci_bus * __devinit pci_acpi_scan_root(struct acpi_device *device, int domain, int busnum)
{
	struct pci_bus *bus;
	struct pci_sysdata *sd;
	int pxm;

	return bus;
}

struct pci_bus * __devinit pcibios_scan_root(int busnum)
{
	printk(KERN_DEBUG "PCI: Probing PCI hardware (bus %02x)\n", busnum);

	return NULL;
}

void eisa_set_level_irq(unsigned int irq)
{
	printk(KERN_DEBUG "PCI: setting IRQ %u as level-triggered\n", irq);
}

int sbf_port = -1;	/* set via acpi_boot_init() */
struct pci_raw_ops *raw_pci_ops;
void (*pm_idle)(void);
EXPORT_SYMBOL(pm_idle);

char wakeup_end = 0;
// g_acpi_temp_wakup_code
char wakeup_start = 0;


void acpi_copy_wakeup_routine()
{
	printk(KERN_INFO "ACPI: acpi_copy_wakeup_routine\n");
}

void do_suspend_lowlevel()
{
	printk(KERN_INFO "ACPI: do_suspend_lowlevel\n");
}
/*
void acpi_blacklisted()
{
	printk(KERN_INFO "ACPI: acpi_blacklisted\n");
}*/

extern int pci_routeirq;
static int __init pci_acpi_init(void)
{
	return 0;
}
subsys_initcall(pci_acpi_init);
