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

	info->ac_line_status = APM_AC_ONLINE;
	info->battery_flag = APM_BATTERY_FLAG_UNKNOWN;
	info->units = APM_UNITS_MINS;
	info->battery_status = APM_BATTERY_STATUS_HIGH;

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
		info->ac_line_status = APM_AC_ONLINE;
		
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



#define mipssaveregcp0(reg, value) asm volatile("li $8, 0\t\n" "mfc0 $8, $"#reg"\t\n" "sd $8,%0\t\n":"=m"(value));


static int __init apm_emu_init(void)
{
	unsigned long cp0[32];
	int i;

	

	mipssaveregcp0(0, cp0[0]);
	mipssaveregcp0(1, cp0[1]);
	mipssaveregcp0(2, cp0[2]);
	mipssaveregcp0(3, cp0[3]);
	mipssaveregcp0(4, cp0[4]);
	mipssaveregcp0(5, cp0[5]);
	mipssaveregcp0(6, cp0[6]);
	mipssaveregcp0(7, cp0[7]);
	mipssaveregcp0(8, cp0[8]);
	mipssaveregcp0(9, cp0[9]);
	mipssaveregcp0(10, cp0[10]);
	mipssaveregcp0(11, cp0[11]);
	mipssaveregcp0(12, cp0[12]);
	mipssaveregcp0(13, cp0[13]);
	mipssaveregcp0(14, cp0[14]);
	mipssaveregcp0(15, cp0[15]);
	mipssaveregcp0(16, cp0[16]);
	mipssaveregcp0(17, cp0[17]);
	mipssaveregcp0(18, cp0[18]);
	mipssaveregcp0(19, cp0[19]);
	mipssaveregcp0(20, cp0[20]);
	mipssaveregcp0(21, cp0[21]);
	mipssaveregcp0(22, cp0[22]);
	mipssaveregcp0(23, cp0[23]);
	mipssaveregcp0(24, cp0[24]);
	mipssaveregcp0(25, cp0[25]);
	mipssaveregcp0(26, cp0[26]);
	mipssaveregcp0(27, cp0[27]);
	mipssaveregcp0(28, cp0[28]);
	mipssaveregcp0(29, cp0[29]);
	mipssaveregcp0(30, cp0[30]);
	mipssaveregcp0(31, cp0[31]);


	for (i = 0; i < 32; i++)
	  printk("cp0_%d = %x  ",i, cp0[i]);
	
	printk("\n");
//	asm volatile(
//		"mfc0 $8,$12\t\n"
//		"sd $8, %0\t\n":"=m"(cp0)
//	);


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
