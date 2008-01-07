/*
 * APM emulation for Loongson-based machines
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/apm-emulation.h>
#include <linux/i2c.h>

static void ls_apm_get_power_status(struct apm_power_info *info)
{
	struct i2c_adapter *p;
	union i2c_smbus_data data, data1, data2, data3;

 	//data batter mode; data1 remain capacity; data2 full capacity; data3 charge/discharge current

	info->battery_flag = APM_BATTERY_FLAG_UNKNOWN;
	info->units = APM_UNITS_MINS;

	p = i2c_get_adapter(0);

	if (p == NULL)
	{
		printk(KERN_ERR "i2c adapter not found! \n");
	}

	//Charge Current
	if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x16, I2C_SMBUS_WORD_DATA,&data)
		&& !i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0xa, I2C_SMBUS_WORD_DATA,&data3))
		;
	else
	{
		//Battery not existed.
		info->ac_line_status = APM_AC_ONLINE;
		info->battery_status = APM_BATTERY_STATUS_UNKNOWN;
		info->battery_flag = APM_BATTERY_FLAG_UNKNOWN;
		info->units = APM_UNITS_MINS;
		info->battery_life = 0;
		info->time = 0;
		return;	
	}

	if (data.word & 0x0040 && data3.word != 0)
	{
		info->ac_line_status = APM_AC_OFFLINE;

		if (data.word & 0x0200)
		{
			info->battery_status = APM_BATTERY_STATUS_LOW;
		}
		if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x0f, I2C_SMBUS_WORD_DATA,&data1))
		{
			//printk(KERN_INFO "apm_emu: 0xf %d\n", data1.word);
			if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x10, I2C_SMBUS_WORD_DATA,&data2))
			{
				//printk(KERN_INFO "apm_emu: 0x10 %d\n", data2.word);
				info->battery_life = data1.word * 100 / data2.word;
		
				if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x11, I2C_SMBUS_WORD_DATA,&data3))
				{
					//printk(KERN_INFO "apm_emu: 11 %d\n", data3.word);
					info->time = data3.word;
				}
				else
				{
					info->time = 0xffff;
				}
			}
			else
			{
				info->battery_life = 0;
				info->time = 0;
			}
		}
		else
		{
			info->battery_life = 0;
			info->time = 0;
		}
	}
	else
	{
		info->battery_status = APM_AC_ONLINE;
		if (data.word & 0x0020)
		{
			info->battery_status = APM_BATTERY_STATUS_HIGH;
			info->battery_life = 100;
		}
		else
		{
			info->battery_status = APM_BATTERY_STATUS_CHARGING;
			if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x0f, I2C_SMBUS_WORD_DATA,&data1))
			{
				//printk(KERN_INFO "apm_emu: 0xf %d\n", data1.word);
				if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x10, I2C_SMBUS_WORD_DATA,&data2))
				{
					//printk(KERN_INFO "apm_emu: 0x10 %d\n", data2.word);
					info->battery_life = data1.word * 100 / data2.word;

					if (!i2c_smbus_xfer(p, 0xb, 0, I2C_SMBUS_READ, 0x13, I2C_SMBUS_WORD_DATA,&data3))
					{
						//printk(KERN_INFO "apm_emu: 13 %d\n", data3.word);
						info->time = data3.word;
					}
					else
					{
						info->time = 0xffff;
					}
				}
				else
				{
					info->battery_life = 0;
					info->time = 0;
				}
			}
			else
			{
				info->battery_life = 0;
				info->time = 0;
			}
			
		}
	}

	
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
