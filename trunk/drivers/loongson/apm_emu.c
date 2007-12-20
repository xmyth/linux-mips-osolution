/*
 * APM emulation for Loongson-based machines
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/apm-emulation.h>

static void ls_apm_get_power_status(struct apm_power_info *info)
{
	int percentage = -1;
	int batteries = 0;
	int time_units = -1;
	int real_count = 0;
	int i;
	char charging = 0;
	long charge = -1;
	long amperage = 0;
	unsigned long btype = 0;

	info->battery_status = APM_BATTERY_STATUS_UNKNOWN;
	info->battery_flag = APM_BATTERY_FLAG_UNKNOWN;
	info->units = APM_UNITS_MINS;
	info->ac_line_status = APM_AC_ONLINE;

	info->battery_life = 100;
	info->time = 20;
}

static int __init apm_emu_init(void)
{
	apm_get_power_status = ls_apm_get_power_status;

	printk(KERN_INFO "apm_emu: Loonson APM Emulation initialized.\n");

	return 0;
}

static void __exit apm_emu_exit(void)
{
	if (apm_get_power_status == ls_apm_get_power_status)
		apm_get_power_status = NULL;

	printk(KERN_INFO "apm_emu: Loongson APM Emulation removed.\n");
}

module_init(apm_emu_init);
module_exit(apm_emu_exit);

MODULE_AUTHOR("West");
MODULE_DESCRIPTION("APM emulation for Loongson 2E/2F Board");
MODULE_LICENSE("GPL");
