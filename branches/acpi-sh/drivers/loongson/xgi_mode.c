//kernel module: syscall-exam-kern.c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include "xgi_mode.h"

/*----------------------------------------------------------------------------*/
/* XG21GPIODataTransfer                                                                 */
/*----------------------------------------------------------------------------*/
UCHAR MYXG21GPIODataTransfer(UCHAR ujDate)
{
	UCHAR  ujRet = 0;
	UCHAR  i = 0;

	for (i=0; i<8; i++)
	{
		ujRet = ujRet << 1;
		/* ujRet |= GETBITS(ujDate >> i, 0:0); */
        ujRet |= (ujDate >> i) & 1;
	}
	return ujRet;
}

/*----------------------------------------------------------------------------*/
/* input                                                                      */
/*      bl[5] : 1;LVDS signal on                                              */
/*      bl[1] : 1;LVDS backlight on                                           */
/*      bl[0] : 1:LVDS VDD on                                                 */
/*      bh: 100000b : clear bit 5, to set bit5                                */
/*          000010b : clear bit 1, to set bit1                                */
/*          000001b : clear bit 0, to set bit0                                */
/*----------------------------------------------------------------------------*/
void MYXGI_XG21BLSignalVDD(USHORT tempbh,USHORT tempbl,unsigned int P3d4)
{
	UCHAR CR4A,temp;
	temp=0;
	inXGIIDXREG(P3d4,0x4A,CR4A) ;//XGINew_GetReg1( pVBInfo->P3d4 , 0x4A ) ;
	tempbh &= 0x23;
	tempbl &= 0x23;
	andXGIIDXREG(P3d4 , 0x4A , ~tempbh ) ; /* enable GPIO write */

	if (tempbh&0x20)
	{
		temp = (tempbl>>4)&0x02;
		setXGIIDXREG(P3d4 , 0xB4 , ~0x02 , temp) ; /* CR B4[1] */
	}

	inXGIIDXREG(P3d4 , 0x48, temp) ;

	temp = MYXG21GPIODataTransfer(temp);
	temp &= ~tempbh;
	temp |= tempbl;
	outXGIIDXREG(P3d4 , 0x48 , temp ) ; 
}

/*----------------------------------------------------------------------------*/
/* MYXGI_PROCESS_LCD                                                         */
/*----------------------------------------------------------------------------*/
void MYXGI_PROCESS_LCD(unsigned char lcdstatus)
{
	unsigned int myp3d4;
	printk("MYXGI_PROCESS_LCD:lcdstatus=%d\n",lcdstatus);		
	myp3d4=XGI_P3D4;
	if(lcdstatus==0)
	{
		setXGIIDXREG(XGI_P3C4,0x09,~0x80,0x00);
		//MYXGI_XG21BLSignalVDD( 0x20 , 0x00, myp3d4) ; /* DVO/DVI signal off */
		MYXGI_XG21BLSignalVDD( 0x02 , 0x00, myp3d4) ; /* LVDS backlight off */
		//MYXGI_XG21BLSignalVDD( 0x01 , 0x00, myp3d4) ; /* LVDS VDD off */
	}
	else if(lcdstatus==1) 
	{
		setXGIIDXREG(XGI_P3C4,0x09,~0x80,0xC0);
		//MYXGI_XG21BLSignalVDD( 0x20 , 0x20, myp3d4) ; /* DVO/DVI signal on */
		MYXGI_XG21BLSignalVDD( 0x02 , 0x02, myp3d4) ; /* LVDS backlight on */
		//MYXGI_XG21BLSignalVDD( 0x01 , 0x01, myp3d4) ; /* LVDS VDD on */				
	}
}

/*----------------------------------------------------------------------------*/
/* MYXGI_PROCESS_CRT                                                         */
/*----------------------------------------------------------------------------*/
void MYXGI_PROCESS_CRT(unsigned char crtstatus)
{
	printk("MYXGI_PROCESS_CRT:crtstatus=%d\n",crtstatus);
	if(crtstatus==0)
	{
		setXGIIDXREG(XGI_P3C4,0x07,~0x20,0x20);
	}
	else if(crtstatus==1)
	{
		setXGIIDXREG(XGI_P3C4,0x07,~0x20,0x00);
	}
}

#define MYMAJOR_DEV_NUM	251
static int mychrdev_major;
static int myxgidispstatus;

ssize_t mychrdev_read(struct file * file, char __user * buf, size_t count, loff_t * ppos)
{
	int xgiop,xgibase,xgidx,xgivalue;
	unsigned char byte1;
	mydev_info_t *mydev_info;
	//printk("mychrdev_read\n");
	//printk("buf=0x%08X,ppos=0x%08X\n",buf,ppos);
	
	if(count != MYCHRDEV_OP_FLAG)
	{
		printk("mychrdev_readx! error param! count=0x%X\n",(int)count);
		return -1;
	}

	mydev_info = (mydev_info_t*)buf;
	printk("opmode=0x%08X,baseaddr=0x%08X,index=0x%08X,value=0x%08X\n",mydev_info->opmode,mydev_info->baseaddr,mydev_info->index,mydev_info->value);
	//printk("count=0x%08X,ppos=0x%08X\n",count,*ppos);
	
	xgiop=mydev_info->opmode;
	xgibase=mydev_info->baseaddr;
	xgidx=mydev_info->index;
	xgivalue=mydev_info->value;

	if(!( ((xgiop==0)||(xgiop==1)) && ((xgibase==0x4c44)||(xgibase==0x4c54)) && (xgivalue<=0xff) ))
	{
		printk(KERN_ALERT"error xgiop param\n");
		return -1;
	}

	if(xgiop==0)
	{
		do{
			MYIOREG(xgibase)=xgidx;
			byte1=MYIOREG(xgibase+1);
		}while(0);
		printk(KERN_ALERT"read:xgibase=0x%04X,xgidx=0x%02X,xgivalue=0x%02X\n",xgibase,xgidx,byte1);
		mydev_info->value = byte1;
	 }	
	else if(xgiop==1)
	{
		do{
			MYIOREG(xgibase)=xgidx;
			MYIOREG(xgibase+1)=xgivalue;
		}while(0);

		////read after the write
		do{
			MYIOREG(xgibase)=xgidx;
			byte1=MYIOREG(xgibase+1);
		}while(0);
		printk(KERN_ALERT"read after change register:xgibase=0x%04X,xgidx=0x%02X,xgivalue=0x%02X\n",xgibase,xgidx,byte1);
		mydev_info->value = byte1;
	 }

	return 0;
}

ssize_t mychrdev_write(struct file * file, const char __user * buf, size_t count, loff_t * ppos)
{
	//printk("mychrdev_write! myxgidispstatus=%d\n",(myxgidispstatus%3)+1);
	if(count != MYCHRDEV_OP_FLAG)
	{
		printk("mychrdev_write! error param! count=0x%X\n",(int)count);
		return -1;
	}
	if(myxgidispstatus==3)
	{
		myxgidispstatus=1;
		MYXGI_PROCESS_CRT(1);
		MYXGI_PROCESS_LCD(0);
	}
	else if(myxgidispstatus==2)
	{
		myxgidispstatus=3;
		MYXGI_PROCESS_CRT(1);
		MYXGI_PROCESS_LCD(1);
	}
	else if(myxgidispstatus==1)
	{
		myxgidispstatus=2;
		MYXGI_PROCESS_CRT(0);
		MYXGI_PROCESS_LCD(1);
	}
	return 0;
}

int mychrdev_ioctl(struct inode * inode, struct file * file, unsigned int cmd, unsigned long argp)
{
	printk("mychrdev_ioctl\n");
/*	mydev_info_t *mydev_info = (mydev_info_t*)argp;
	printk("opmode=0x%08X,baseaddr=0x%08X,index=0x%08X,value=0x%08X\n",mydev_info->opmode,mydev_info->baseaddr,mydev_info->index,mydev_info->value);*/
	return 0;
}

int mychrdev_open(struct inode * inode, struct file * file)
{
	//printk("mychrdev_open\n");
	return 0;
}
/*
int mychrdev_mmap(struct file * file, struct vm_area_struct * vma)
{
	printk("mychrdev_mmap\n");
	return 0;
 }
*/
int mychrdev_release(struct inode * inode, struct file * file)
{
	//printk("mychrdev_release\n");
	return 0;
}
/*
loff_t mychrdev_llseek(struct file * file, loff_t offset, int seek_flags)
{
	printk("mychrdev_llseek\n");
	return 0;
}
*/
struct file_operations mychrdev_fops = {
		.owner = THIS_MODULE,
		.read = mychrdev_read,
		.write = mychrdev_write,
		.ioctl = mychrdev_ioctl,
		.open = mychrdev_open,
		//.llseek = mychrdev_llseek,
		.release = mychrdev_release,
		//.mmap = mychrdev_mmap,
};


static int __init mychardev_init(void)
{
	int result;
	mychrdev_major = MYMAJOR_DEV_NUM;
	//mychrdev_major = register_chrdev(0, "mychrdev", &mychrdev_fops);
	//if (mychrdev_major <= 0) {
	result = register_chrdev(mychrdev_major, "mychrdev", &mychrdev_fops);
	if (result != 0) 
	{
		printk("Fail to register char device mychrdev.\n");
		return -1;
	}
	printk("char device mychrdev is registered, major is %d, result is %d\n", mychrdev_major,result);
	myxgidispstatus=3;

	return 0;
}

static void __exit mychardev_remove(void)
{
	unregister_chrdev(mychrdev_major, NULL);
}

module_init(mychardev_init);
module_exit(mychardev_remove);
MODULE_LICENSE("GPL");

/*
$ insmod ./syscall-exam-kern.ko
char device mychrdev is registered, major is 254
$ mknod /dev/mychrdev c `dmesg | grep "char device mychrdev" | sed 's/.*major is //g'` 0
$ cat /dev/mychrdev
$ echo "abcdefghijklmnopqrstuvwxyz" > /dev/mychrdev
$ cat /dev/mychrdev
abcdefghijklmnopqrstuvwxyz
$ ./syscall-exam-user
User process: syscall-exam-us(1433)
Available space: 65509 bytes
Data len: 27 bytes
Offset in physical: cc0 bytes
mychrdev content by mmap:
abcdefghijklmnopqrstuvwxyz
$ cat /dev/mychrdev
abcde
*/
