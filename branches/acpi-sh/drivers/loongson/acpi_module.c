/*
 * ACPI Module for Loongson-based machines
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/preempt.h>
#include <linux/crc32.h>
#include <linux/irq.h>
#include <linux/suspend.h>
#include "via686b.h"  //This file comes from pmon definition
unsigned long g_crc = 0;

extern void trap_init(void);
extern void suspend_console(void);
extern void resume_console(void);
extern void thaw_processes(void);
extern void freeze_processes(void);
extern void (*flush_cache_all)(void);


#define mipssaveregcp0(reg, value) asm volatile("dmfc0 $8, $"#reg"\t\n" "sd $8,%0\t\n":"=m"(value))

#define mipsrestoreregcp0(reg, value) asm volatile("ld $8, 0\t\n" "dmtc0 $8, $"#reg"\t\n"::"m"(value))



#define mipssavereg(reg, value)   asm volatile("sd $"#reg",%0\t\n":"=m"(value))

#define mipsrestorereg(reg, value)   asm volatile("ld $"#reg",%0\t\n"::"m"(value))


#define mipssavefreg(reg, value)  asm volatile("sdc1 $"#reg", %0\t\n":"=m"(value))                                                       
#define mipsrestorefreg(reg, value)  asm volatile("ldc1 $"#reg", %0\t\n":"m"(value))                                                       

#ifdef CONFIG_32BIT
#define COM1_BASE_ADDR	0xbfd003f8
#define COM2_BASE_ADDR	0xbfd002f8
#define COM3_BASE_ADDR	0xbff003f8
#else
#define COM1_BASE_ADDR	0xffffffffbfd003f8
#define COM2_BASE_ADDR	0xffffffffbfd002f8
#define COM3_BASE_ADDR	0xffffffffbff003f8
#endif
//#define	NS16550HZ	1843200
#define	NS16550HZ	3686400

#define	NS16550_DATA	0
#define	NS16550_IER	1
#define	NS16550_IIR	2
#define	NS16550_FIFO	2
#define	NS16550_CFCR	3
#define	NS16550_MCR	4
#define	NS16550_LSR	5
#define	NS16550_MSR	6	
#define	NS16550_SCR	7

/* line status register */
#define	LSR_RCV_FIFO	0x80	/* error in receive fifo */
#define	LSR_TSRE	0x40	/* transmitter empty */
#define	LSR_TXRDY	0x20	/* transmitter ready */
#define	LSR_BI		0x10	/* break detected */
#define	LSR_FE		0x08	/* framing error */
#define	LSR_PE		0x04	/* parity error */
#define	LSR_OE		0x02	/* overrun error */
#define	LSR_RXRDY	0x01	/* receiver ready */
#define	LSR_RCV_MASK	0x1f


extern void trap_init(void);
extern void suspend_console(void);
extern void resume_console(void);
extern void thaw_processes(void);
extern void freeze_processes(void);

void PrintStrToSerial(unsigned char *buf, unsigned int len)
{
	volatile unsigned char v1;
	unsigned int i, j;

	for (i = 0; i < len ; i++)
	{	
		j = 0;
		do
		{
			v1 = *((volatile unsigned char *)(COM1_BASE_ADDR + NS16550_LSR));
			j++;
		}
		while((v1 & LSR_TXRDY) == 0 && j < 0x10000);

		*((volatile unsigned char *)(COM1_BASE_ADDR + NS16550_DATA)) = buf[i];	
	}
}

static void EnableDDRConfigSpace()
{
        volatile unsigned long  *t2 = 0xffffffffbfe00180;
        volatile unsigned long  a1 = *t2;
	 a1 &= 0x6ff;
        *(t2) = a1;
}

static void DisableDDRConfigSpace()
{
        volatile unsigned long  *t2 = 0xffffffffbfe00180;
        volatile unsigned long  a1 = *t2;
	 a1 |= 0x100;
        *(t2) = a1;
}


struct MIPS_CPU_REG_STATE{
	unsigned long long version;
	unsigned long long r[32];
	unsigned long long cp0[32];
	unsigned long long f[32];
	unsigned long long sRA;
	unsigned char stacksize[10240];
	unsigned long long i, len;
	unsigned long long crc;
};


struct MIPS_CPU_REG_STATE volatile * MipsCpuRegState = (struct MIPS_CPU_REG_STATE volatile *)(0xffffffffa0000000 + (255 << 20));

static void Enter_STR(void)
{
	unsigned int v0, a0;

	const unsigned char STR_VALUE = 4;

#ifdef CONFIG_32BIT
	#define AddrBase 0xbfd00000
#else
	#define AddrBase 0xffffffffbfd00000
#endif
	*(volatile unsigned char *)(AddrBase + SMBUS_HOST_DATA0) = STR_VALUE;
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

	printk("0x%llx\n", MipsCpuRegState->r[29]);
	printk("0x%llx\n", *(unsigned long long *)MipsCpuRegState->r[29]);

	//suspend_console();


	EnableDDRConfigSpace();
	*(volatile unsigned long *)(0xffffffffaffffe80) = 0; //Shutdown ODT
	*(volatile unsigned long *)(0xffffffffaffffe84) = 0; //Shutdown ODT

	volatile unsigned long state = *(volatile unsigned long *)(0xffffffffaffffe34);
	state |= 1;
	
	g_crc = crc32_le(0, (unsigned char *) (0xffffffffaff00000), 0x100000);
	printk("crc = 0x%llx \n", g_crc);

	*(volatile unsigned long *)(0xffffffffaffffe34) = state;
	DisableDDRConfigSpace();


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
	while(1);
}


extern void kernel_entry(void);

extern void (*flush_cache_all)(void);

extern void local_flush_tlb_all(void);



static noinline int mips_save_ret_addr(void)
{
	mipssavereg(31, MipsCpuRegState->r[31]);
	//MipsCpuRegState->r[31] =  (unsigned long long *)start_kernel;
	printk("%llx \n", MipsCpuRegState->r[31]);

	flush_cache_all();

	local_flush_tlb_all();

	__asm__ volatile ("sync");


	g_crc = crc32_le(0, (unsigned char *) (0xffffffffaff00000), 0x100000);
	printk("crc = 0x%llx \n", g_crc);
	
	asm volatile("li $3, 0Xffffffffbfc01108\t\n"::);
	asm volatile ("jr $3\t\n" "nop\t\n"::);

	//__asm__ volatile ("li a0, 0xffffffffbfc01108\t\n");
	//__asm__ volatile ("jr a0\t\n" "nop\t\n");

	//Enter_STR();	
}

static noinline void SerialOutput()
{
	volatile unsigned char v1;
	unsigned long long i;
	unsigned long long len = 25;
	const char * buf= "Welcome Back From STR! \n\r";
	unsigned long long j;
	

	for (i = 0; i < len ; i++)
	{
		do
		{
			v1 = *((volatile unsigned char *)(COM1_BASE_ADDR + NS16550_LSR));
			
		}
		while((v1 & LSR_TXRDY) == 0);

		*((volatile unsigned char *)(COM1_BASE_ADDR + NS16550_DATA)) = buf[i];	
	}
}



static noinline int mips_save_register_state(void)
{
//	volatile unsigned int i, len;
	unsigned int crc = 0;

	//printk("Stack %llx\n", &i);
	//
	//
	local_irq_disable();

	MipsCpuRegState->version = 0xaa554321131455aa;

	mipssavereg(0, MipsCpuRegState->r[0]);
	//  mipssavereg(1, MipsCpuRegState->r[1]);  
	mipssavereg(2, MipsCpuRegState->r[2]);  
	mipssavereg(3, MipsCpuRegState->r[3]);  
	mipssavereg(4, MipsCpuRegState->r[4]);  
	mipssavereg(5, MipsCpuRegState->r[5]);  
	mipssavereg(6, MipsCpuRegState->r[6]);  
	mipssavereg(7, MipsCpuRegState->r[7]);  
	mipssavereg(8, MipsCpuRegState->r[8]);  
	mipssavereg(9, MipsCpuRegState->r[9]);  
	mipssavereg(10, MipsCpuRegState->r[10]);  
	mipssavereg(11, MipsCpuRegState->r[11]);  
	mipssavereg(12, MipsCpuRegState->r[12]);  
	mipssavereg(13, MipsCpuRegState->r[13]);  
	mipssavereg(14, MipsCpuRegState->r[14]);  
	mipssavereg(15, MipsCpuRegState->r[15]);  
	mipssavereg(16, MipsCpuRegState->r[16]);  
	mipssavereg(17, MipsCpuRegState->r[17]);  
	mipssavereg(18, MipsCpuRegState->r[18]);  
	mipssavereg(19, MipsCpuRegState->r[19]);  
	mipssavereg(20, MipsCpuRegState->r[20]);  
	mipssavereg(21, MipsCpuRegState->r[21]);  
	mipssavereg(22, MipsCpuRegState->r[22]);  
	mipssavereg(23, MipsCpuRegState->r[23]);  
	mipssavereg(24, MipsCpuRegState->r[24]);  
	mipssavereg(25, MipsCpuRegState->r[25]);  
	mipssavereg(26, MipsCpuRegState->r[26]);  
	mipssavereg(27, MipsCpuRegState->r[27]);  
	mipssavereg(28, MipsCpuRegState->r[28]);  
	mipssavereg(29, MipsCpuRegState->r[29]);  
	mipssavereg(30, MipsCpuRegState->r[30]);  
	mipssavereg(31, MipsCpuRegState->sRA);

	
/*	
	mipssavefreg(f0, MipsCpuRegState->f[0]);  
	mipssavefreg(f1, MipsCpuRegState->f[1]);  
	mipssavefreg(f2, MipsCpuRegState->f[2]);  
	mipssavefreg(f3, MipsCpuRegState->f[3]);  
	mipssavefreg(f4, MipsCpuRegState->f[4]);  
	mipssavefreg(f5, MipsCpuRegState->f[5]);  
	mipssavefreg(f6, MipsCpuRegState->f[6]);  
	mipssavefreg(f7, MipsCpuRegState->f[7]);  
	mipssavefreg(f8, MipsCpuRegState->f[8]);  
	mipssavefreg(f9, MipsCpuRegState->f[9]);  
	mipssavefreg(f10, MipsCpuRegState->f[10]);  
	mipssavefreg(f11, MipsCpuRegState->f[11]);  
	mipssavefreg(f12, MipsCpuRegState->f[12]);  
	mipssavefreg(f13, MipsCpuRegState->f[13]);  
	mipssavefreg(f14, MipsCpuRegState->f[14]);  
	mipssavefreg(f15, MipsCpuRegState->f[15]);  
	mipssavefreg(f16, MipsCpuRegState->f[16]);  
	mipssavefreg(f17, MipsCpuRegState->f[17]);  
	mipssavefreg(f18, MipsCpuRegState->f[18]);  
	mipssavefreg(f19, MipsCpuRegState->f[19]);  
	mipssavefreg(f20, MipsCpuRegState->f[20]);  
	mipssavefreg(f21, MipsCpuRegState->f[21]);  
	mipssavefreg(f22, MipsCpuRegState->f[22]);  
	mipssavefreg(f23, MipsCpuRegState->f[23]);  
	mipssavefreg(f24, MipsCpuRegState->f[24]);  
	mipssavefreg(f25, MipsCpuRegState->f[25]);  
	mipssavefreg(f26, MipsCpuRegState->f[26]);  
	mipssavefreg(f27, MipsCpuRegState->f[27]);  
	mipssavefreg(f28, MipsCpuRegState->f[28]);  
	mipssavefreg(f29, MipsCpuRegState->f[29]);  
	mipssavefreg(f30, MipsCpuRegState->f[30]);  
	mipssavefreg(f31, MipsCpuRegState->f[31]);  
*/


	mipssaveregcp0(0, MipsCpuRegState->cp0[0]);
	mipssaveregcp0(1, MipsCpuRegState->cp0[1]);
	mipssaveregcp0(2, MipsCpuRegState->cp0[2]);
	mipssaveregcp0(3, MipsCpuRegState->cp0[3]);
	mipssaveregcp0(4, MipsCpuRegState->cp0[4]);
	mipssaveregcp0(5, MipsCpuRegState->cp0[5]);
	mipssaveregcp0(6, MipsCpuRegState->cp0[6]);
	mipssaveregcp0(7, MipsCpuRegState->cp0[7]);
	mipssaveregcp0(8, MipsCpuRegState->cp0[8]);
	mipssaveregcp0(9, MipsCpuRegState->cp0[9]);
	mipssaveregcp0(10, MipsCpuRegState->cp0[10]);
	mipssaveregcp0(11, MipsCpuRegState->cp0[11]);
	mipssaveregcp0(12, MipsCpuRegState->cp0[12]);
	mipssaveregcp0(13, MipsCpuRegState->cp0[13]);
	mipssaveregcp0(14, MipsCpuRegState->cp0[14]);
	mipssaveregcp0(15, MipsCpuRegState->cp0[15]);
	mipssaveregcp0(16, MipsCpuRegState->cp0[16]);
	mipssaveregcp0(17, MipsCpuRegState->cp0[17]);
	mipssaveregcp0(18, MipsCpuRegState->cp0[18]);
	mipssaveregcp0(19, MipsCpuRegState->cp0[19]);
	mipssaveregcp0(20, MipsCpuRegState->cp0[20]);
	mipssaveregcp0(21, MipsCpuRegState->cp0[21]);
	mipssaveregcp0(22, MipsCpuRegState->cp0[22]);
	mipssaveregcp0(23, MipsCpuRegState->cp0[23]);
	mipssaveregcp0(24, MipsCpuRegState->cp0[24]);
	mipssaveregcp0(25, MipsCpuRegState->cp0[25]);
	mipssaveregcp0(26, MipsCpuRegState->cp0[26]);
	mipssaveregcp0(27, MipsCpuRegState->cp0[27]);
	mipssaveregcp0(28, MipsCpuRegState->cp0[28]);
	mipssaveregcp0(29, MipsCpuRegState->cp0[29]);
	mipssaveregcp0(30, MipsCpuRegState->cp0[30]);
	mipssaveregcp0(31, MipsCpuRegState->cp0[31]);




	printk("0x%llx\n", MipsCpuRegState->r[29]);
	printk("0x%llx\n", *(unsigned long long *)MipsCpuRegState->r[29]);
//	printk("0x%llx \n", *(unsigned long long *)(MipsCpuRegState->r[29]+8));

//	suspend_console();

	MipsCpuRegState->len = (unsigned long long)&(MipsCpuRegState->i);
	
	mipsrestorereg(29, MipsCpuRegState->len);


	mips_save_ret_addr();

	crc = crc32_le(0, (unsigned char *) (0xffffffffaff00000), 0x100000);

	
	//PrintStrToSerial("Return From STR1!\r\n", 18);

	MipsCpuRegState->len = (unsigned long long)&(MipsCpuRegState->i);
	
	mipsrestorereg(29, MipsCpuRegState->len);

	//resume_console();

	printk("crc = 0x%lx \n", crc);

	printk("Return From STR\n");
	
	//trap_init();
	
	*(volatile char *)(0xffffffffbfe0011c+4) = 0;
	*(volatile char *)(0xffffffffbfe0011c) = 0;

	//MipsCpuRegState->version = 0xbb554321131455bb;

	//mipsrestorereg(31, MipsCpuRegState->sRA);
	for (MipsCpuRegState->i = 0; MipsCpuRegState->i < 0x500000; MipsCpuRegState->i++)
		MipsCpuRegState->len++;


	MipsCpuRegState->len = 0;


	*(volatile char *)(0xffffffffbfe0011c) = 0xf;
	
	for (MipsCpuRegState->i = 0; MipsCpuRegState->i < 0x500000; MipsCpuRegState->i++)
			MipsCpuRegState->len++;

	MipsCpuRegState->len = 0;
	*(volatile char *)(0xffffffffbfe0011c) = 0x0;

	for (MipsCpuRegState->i = 0; MipsCpuRegState->i < 0x500000; MipsCpuRegState->i++)
			MipsCpuRegState->len++;


//	PrintStrToSerial("Welcome Back From STR! \n\r", 25);
//	flush_tlb_all();

	mipsrestorereg(29, MipsCpuRegState->r[29]);


	*(unsigned long long *)MipsCpuRegState->r[29] = MipsCpuRegState->sRA;

	printk("0x%llx\n", MipsCpuRegState->r[29]);
	printk("0x%llx\n", *(unsigned long long *)MipsCpuRegState->r[29]);

}


static int __init acpi_module_init(void)
{
	printk(KERN_INFO "acpi_module: ACPI Module initialized.\n");

	return 0;
}

static void __exit acpi_module_exit(void)
{
	printk(KERN_INFO "acpi_module: ACPI Module removed.\n");
}

module_init(acpi_module_init);
module_exit(acpi_module_exit);

MODULE_AUTHOR("Aimeng");
MODULE_DESCRIPTION("ACPI Module for Loongson 2E/2F Board");
MODULE_LICENSE("GPL");

