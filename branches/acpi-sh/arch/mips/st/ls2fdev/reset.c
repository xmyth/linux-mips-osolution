/*
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * Copyright (C) 2007 Lemote, Inc. & Institute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 */
#include <linux/pm.h>

#include <asm/reboot.h>

#include "via686b.h"  //This file comes from pmon definition

static void loongson2e_restart(char *command)
{
#ifdef CONFIG_32BIT
	*(unsigned long *)0xbfe00104 &= ~(1 << 2);
	*(unsigned long *)0xbfe00104 |= (1 << 2);
#else
	*(unsigned long *)0xffffffffbfe00104 &= ~(1 << 2);
	*(unsigned long *)0xffffffffbfe00104 |= (1 << 2);
#endif
	__asm__ __volatile__("jr\t%0"::"r"(0xbfc00000));
}

static void loongson2f_soft_off(void);

static void loongson2e_halt(void)
{
	loongson2f_soft_off();
	while (1) ;
}

static void loongson2e_power_off(void)
{
	loongson2e_halt();
}

void mips_reboot_setup(void)
{
	_machine_restart = loongson2e_restart;
	_machine_halt = loongson2e_halt;
	pm_power_off = loongson2e_power_off;
}

static void loongson2f_soft_off(void)
{
	unsigned int v0, a0;
	//Power off value is 1
	const unsigned char power_off_value = 1;

#ifdef CONFIG_32BIT
	#define AddrBase 0xbfd00000
#else
	#define AddrBase 0xffffffffbfd00000
#endif
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_DATA0) = power_off_value;
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_DATA1) = 0;
	
	//ST7 chip addr 
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_ADDRESS) = 0x24;
	
	//Offset 
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_COMMAND) = 0x0;

	//Byte Write
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_CONTROL) = 0x08;

	v0 = *(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS);
	v0 &= 0x1f;
	if (v0)
	{
		*(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS) = v0;
		v0 = *(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS);
	}
	
	//Start
	v0 = *(volatile unsigned char *)(AddrBase + SMBUS_HOST_CONTROL);
	v0 |= 0x40;
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_CONTROL) = v0;

	do
	{
		//Wait
		for (a0 = 0x1000; a0 != 0;);
			a0--;
		v0 = *(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS);
	}while(v0 & SMBUS_HOST_STATUS_BUSY);

	v0 = *(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS);
	v0 &= 0x1f;
	if (v0)
	{
		*(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS) = v0;
		v0 = *(volatile unsigned char *)(AddrBase + SMBUS_HOST_STATUS);
	}
}
