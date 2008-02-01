/*
 * XGI 300/630/730/540/315/550/650/740 frame buffer device
 * for Linux kernels 2.4.x and 2.5.x
 *
 * Partly based on the VBE 2.0 compliant graphic boards framebuffer driver,
 * which is (c) 1998 Gerd Knorr <kraxel@goldbach.in-berlin.de>
 *
 * Authors:   	XGI (www.XGI.com.tw)
 *		(Various others)
 *		Thomas Winischhofer <thomas@winischhofer.net>:
 *			- XGI Xabre (330) support
 *			- many fixes and enhancements for all chipset series,
 *			- extended bridge handling, TV output for Chrontel 7005
 *                      - 650/LVDS support (for LCD panels up to 1600x1200)
 *                      - 650/740/Chrontel 7019 support
 *                      - 30xB/30xLV LCD, TV and VGA2 support
 *			- memory queue handling enhancements,
 *                      - 2D acceleration and y-panning,
 *                      - portation to 2.5 API
 *			- etc.
 *			(see http://www.winischhofer.net/
 *			for more information and updates)
 */

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/selection.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/vt_kern.h>
#include <linux/capability.h>
#include <linux/fs.h>
#include <linux/agp_backend.h>
#include <linux/types.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#include <linux/spinlock.h>
#endif

#include "osdef.h"


#ifndef XGIFB_PAN
#define XGIFB_PAN
#endif

/*
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#include <video/XGIfb.h>
#else
#include <linux/XGIfb.h>
#endif
*/
#include <asm/io.h>
#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#include <video/fbcon.h>
#include <video/fbcon-cfb8.h>
#include <video/fbcon-cfb16.h>
#include <video/fbcon-cfb24.h>
#include <video/fbcon-cfb32.h>
#endif
#include "XGIfb.h"
#include "vgatypes.h"
#include "XGI_main.h"


/* data for XGI components */
struct video_info xgi_video_info;

/* -------------------- Macro definitions ---------------------------- */

#undef XGIFBDEBUG 	/* TW: no debugging */

#ifdef XGIFBDEBUG
#define DPRINTK(fmt, args...) printk(KERN_DEBUG "%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#ifdef XGIFBACCEL
#ifdef FBCON_HAS_CFB8
extern struct display_switch fbcon_XGI8;
#endif
#ifdef FBCON_HAS_CFB16
extern struct display_switch fbcon_XGI16;
#endif
#ifdef FBCON_HAS_CFB32
extern struct display_switch fbcon_XGI32;
#endif
#endif
#endif


#if 1
#define DEBUGPRN(x)
#else
#define DEBUGPRN(x) printk(KERN_INFO x "\n");
#endif


/* --------------- Hardware Access Routines -------------------------- */

#ifdef LINUX_KERNEL
int
XGIfb_mode_rate_to_dclock(VB_DEVICE_INFO *XGI_Pr, PXGI_HW_DEVICE_INFO HwDeviceExtension,
			  unsigned char modeno, unsigned char rateindex)
{
    USHORT ModeNo = modeno;
    USHORT ModeIdIndex = 0, ClockIndex = 0;
    USHORT RefreshRateTableIndex = 0;
    
    ULONG  temp = 0;
    int    Clock;
    XGI_Pr->ROMAddr  = HwDeviceExtension->pjVirtualRomBase;
    InitTo330Pointer( HwDeviceExtension->jChipType, XGI_Pr ) ;
    
    temp = XGI_SearchModeID( ModeNo , &ModeIdIndex,  XGI_Pr ) ;
    if(!temp) {
    	printk(KERN_ERR "Could not find mode %x\n", ModeNo);
    	return 65;
    }
    
    RefreshRateTableIndex = XGI_Pr->EModeIDTable[ModeIdIndex].REFindex;
    RefreshRateTableIndex += (rateindex - 1);
    ClockIndex = XGI_Pr->RefIndex[RefreshRateTableIndex].Ext_CRTVCLK;
    if(HwDeviceExtension->jChipType < XGI_315H) {
       ClockIndex &= 0x3F;
    }
    Clock = XGI_Pr->VCLKData[ClockIndex].CLOCK * 1000 * 1000;
    
    return(Clock);
}

int
XGIfb_mode_rate_to_ddata(VB_DEVICE_INFO *XGI_Pr, PXGI_HW_DEVICE_INFO HwDeviceExtension,
			 unsigned char modeno, unsigned char rateindex,
			 ULONG *left_margin, ULONG *right_margin, 
			 ULONG *upper_margin, ULONG *lower_margin,
			 ULONG *hsync_len, ULONG *vsync_len,
			 ULONG *sync, ULONG *vmode)
{
    USHORT ModeNo = modeno;
    USHORT ModeIdIndex = 0, index = 0;
    USHORT RefreshRateTableIndex = 0;
    
    unsigned short VRE, VBE, VRS, VBS, VDE, VT;
    unsigned short HRE, HBE, HRS, HBS, HDE, HT;
    unsigned char  sr_data, cr_data, cr_data2, cr_data3;
    int            A, B, C, D, E, F, temp, j;
    XGI_Pr->ROMAddr  = HwDeviceExtension->pjVirtualRomBase;
    InitTo330Pointer( HwDeviceExtension->jChipType, XGI_Pr ) ;
    
    temp = XGI_SearchModeID( ModeNo, &ModeIdIndex, XGI_Pr);
    if(!temp) return 0;
    
    RefreshRateTableIndex = XGI_Pr->EModeIDTable[ModeIdIndex].REFindex;
    RefreshRateTableIndex += (rateindex - 1);
    index = XGI_Pr->RefIndex[RefreshRateTableIndex].Ext_CRT1CRTC;

    sr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[5];

    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[0];

    /* Horizontal total */
    HT = (cr_data & 0xff) |
         ((unsigned short) (sr_data & 0x03) << 8);
    A = HT + 5;

    /*cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[1];
	
     Horizontal display enable end 
    HDE = (cr_data & 0xff) |
          ((unsigned short) (sr_data & 0x0C) << 6);*/
    HDE = (XGI_Pr->RefIndex[RefreshRateTableIndex].XRes >> 3) -1;      
    E = HDE + 1;

    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[3];
	
    /* Horizontal retrace (=sync) start */
    HRS = (cr_data & 0xff) |
          ((unsigned short) (sr_data & 0xC0) << 2);
    F = HRS - E - 3;

    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[1];
	
    /* Horizontal blank start */
    HBS = (cr_data & 0xff) |
          ((unsigned short) (sr_data & 0x30) << 4);

    sr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[6];
	
    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[2];

    cr_data2 = XGI_Pr->XGINEWUB_CRT1Table[index].CR[4];
	
    /* Horizontal blank end */
    HBE = (cr_data & 0x1f) |
          ((unsigned short) (cr_data2 & 0x80) >> 2) |
	  ((unsigned short) (sr_data & 0x03) << 6);

    /* Horizontal retrace (=sync) end */
    HRE = (cr_data2 & 0x1f) | ((sr_data & 0x04) << 3);

    temp = HBE - ((E - 1) & 255);
    B = (temp > 0) ? temp : (temp + 256);

    temp = HRE - ((E + F + 3) & 63);
    C = (temp > 0) ? temp : (temp + 64);

    D = B - F - C;
    
    *left_margin = D * 8;
    *right_margin = F * 8;
    *hsync_len = C * 8;

    sr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[14];

    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[8];
    
    cr_data2 = XGI_Pr->XGINEWUB_CRT1Table[index].CR[9];
    
    /* Vertical total */
    VT = (cr_data & 0xFF) |
         ((unsigned short) (cr_data2 & 0x01) << 8) |
	 ((unsigned short)(cr_data2 & 0x20) << 4) |
	 ((unsigned short) (sr_data & 0x01) << 10);
    A = VT + 2;

    //cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[10];
	
    /* Vertical display enable end */
/*    VDE = (cr_data & 0xff) |
          ((unsigned short) (cr_data2 & 0x02) << 7) |
	  ((unsigned short) (cr_data2 & 0x40) << 3) |
	  ((unsigned short) (sr_data & 0x02) << 9); */
    VDE = XGI_Pr->RefIndex[RefreshRateTableIndex].YRes  -1;
    E = VDE + 1;

    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[10];

    /* Vertical retrace (=sync) start */
    VRS = (cr_data & 0xff) |
          ((unsigned short) (cr_data2 & 0x04) << 6) |
	  ((unsigned short) (cr_data2 & 0x80) << 2) |
	  ((unsigned short) (sr_data & 0x08) << 7);
    F = VRS + 1 - E;

    cr_data =  XGI_Pr->XGINEWUB_CRT1Table[index].CR[12];

    cr_data3 = (XGI_Pr->XGINEWUB_CRT1Table[index].CR[14] & 0x80) << 5;

    /* Vertical blank start */
    VBS = (cr_data & 0xff) |
          ((unsigned short) (cr_data2 & 0x08) << 5) |
	  ((unsigned short) (cr_data3 & 0x20) << 4) |
	  ((unsigned short) (sr_data & 0x04) << 8);

    cr_data =  XGI_Pr->XGINEWUB_CRT1Table[index].CR[13];

    /* Vertical blank end */
    VBE = (cr_data & 0xff) |
          ((unsigned short) (sr_data & 0x10) << 4);
    temp = VBE - ((E - 1) & 511);
    B = (temp > 0) ? temp : (temp + 512);

    cr_data = XGI_Pr->XGINEWUB_CRT1Table[index].CR[11];

    /* Vertical retrace (=sync) end */
    VRE = (cr_data & 0x0f) | ((sr_data & 0x20) >> 1);
    temp = VRE - ((E + F - 1) & 31);
    C = (temp > 0) ? temp : (temp + 32);

    D = B - F - C;
      
    *upper_margin = D;
    *lower_margin = F;
    *vsync_len = C;

    if(XGI_Pr->RefIndex[RefreshRateTableIndex].Ext_InfoFlag & 0x8000)
       *sync &= ~FB_SYNC_VERT_HIGH_ACT;
    else
       *sync |= FB_SYNC_VERT_HIGH_ACT;

    if(XGI_Pr->RefIndex[RefreshRateTableIndex].Ext_InfoFlag & 0x4000)       
       *sync &= ~FB_SYNC_HOR_HIGH_ACT;
    else
       *sync |= FB_SYNC_HOR_HIGH_ACT;
		
    *vmode = FB_VMODE_NONINTERLACED;       
    if(XGI_Pr->RefIndex[RefreshRateTableIndex].Ext_InfoFlag & 0x0080)
       *vmode = FB_VMODE_INTERLACED;
    else {
      j = 0;
      while(XGI_Pr->EModeIDTable[j].Ext_ModeID != 0xff) {
          if(XGI_Pr->EModeIDTable[j].Ext_ModeID ==
	                  XGI_Pr->RefIndex[RefreshRateTableIndex].ModeID) {
              if(XGI_Pr->EModeIDTable[j].Ext_ModeFlag & DoubleScanMode) {
	      	  *vmode = FB_VMODE_DOUBLE;
              }
	      break;
          }
	  j++;
      }
    }       
          
    return 1;       
}			  

#endif



void XGIRegInit(VB_DEVICE_INFO *XGI_Pr, USHORT BaseAddr)
{
   XGI_Pr->P3c4 = BaseAddr + 0x14;
   XGI_Pr->P3d4 = BaseAddr + 0x24;
   XGI_Pr->P3c0 = BaseAddr + 0x10;
   XGI_Pr->P3ce = BaseAddr + 0x1e;
   XGI_Pr->P3c2 = BaseAddr + 0x12;
   XGI_Pr->P3ca = BaseAddr + 0x1a;
   XGI_Pr->P3c6 = BaseAddr + 0x16;
   XGI_Pr->P3c7 = BaseAddr + 0x17;
   XGI_Pr->P3c8 = BaseAddr + 0x18;
   XGI_Pr->P3c9 = BaseAddr + 0x19;
   XGI_Pr->P3da = BaseAddr + 0x2A;
   XGI_Pr->Part1Port = BaseAddr + XGI_CRT2_PORT_04;   /* Digital video interface registers (LCD) */
   XGI_Pr->Part2Port = BaseAddr + XGI_CRT2_PORT_10;   /* 301 TV Encoder registers */
   XGI_Pr->Part3Port = BaseAddr + XGI_CRT2_PORT_12;   /* 301 Macrovision registers */
   XGI_Pr->Part4Port = BaseAddr + XGI_CRT2_PORT_14;   /* 301 VGA2 (and LCD) registers */
   XGI_Pr->Part5Port = BaseAddr + XGI_CRT2_PORT_14+2; /* 301 palette address port registers */
  
}


void XGIfb_set_reg4(u16 port, unsigned long data)
{
	outl((u32) (data & 0xffffffff), port);
}

u32 XGIfb_get_reg3(u16 port)
{
	u32 data;

	data = inl(port);
	return (data);
}

/* ------------ Interface for init & mode switching code ------------- */

BOOLEAN
XGIfb_query_VGA_config_space(PXGI_HW_DEVICE_INFO pXGIhw_ext,
	unsigned long offset, unsigned long set, unsigned long *value)
{
	static struct pci_dev *pdev = NULL;
	static unsigned char init = 0, valid_pdev = 0;

	if (!set)
		DPRINTK("XGIfb: Get VGA offset 0x%lx\n", offset);
	else
		DPRINTK("XGIfb: Set offset 0x%lx to 0x%lx\n", offset, *value);

	if (!init) {
		init = TRUE;
		pdev = pci_find_device(PCI_VENDOR_ID_SI, xgi_video_info.chip_id, pdev);
		if (pdev)
			valid_pdev = TRUE;
	}

	if (!valid_pdev) {
		printk(KERN_DEBUG "XGIfb: Can't find XGI %d VGA device.\n",
				xgi_video_info.chip_id);
		return FALSE;
	}

	if (set == 0)
		pci_read_config_dword(pdev, offset, (u32 *)value);
	else
		pci_write_config_dword(pdev, offset, (u32)(*value));

	return TRUE;
}

/*BOOLEAN XGIfb_query_north_bridge_space(PXGI_HW_DEVICE_INFO pXGIhw_ext,
	unsigned long offset, unsigned long set, unsigned long *value)
{
	static struct pci_dev *pdev = NULL;
	static unsigned char init = 0, valid_pdev = 0;
	u16 nbridge_id = 0;

	if (!init) {
		init = TRUE;
		switch (xgi_video_info.chip) {
		case XGI_540:
			nbridge_id = PCI_DEVICE_ID_XG_540;
			break;
		case XGI_630:
			nbridge_id = PCI_DEVICE_ID_XG_630;
			break;
		case XGI_730:
			nbridge_id = PCI_DEVICE_ID_XG_730;
			break;
		case XGI_550:
			nbridge_id = PCI_DEVICE_ID_XG_550;
			break;
		case XGI_650:
			nbridge_id = PCI_DEVICE_ID_XG_650;
			break;
		case XGI_740:			
			nbridge_id = PCI_DEVICE_ID_XG_740;
			break;
		default:
			nbridge_id = 0;
			break;
		}

		pdev = pci_find_device(PCI_VENDOR_ID_SI, nbridge_id, pdev);
		if (pdev)
			valid_pdev = TRUE;
	}

	if (!valid_pdev) {
		printk(KERN_DEBUG "XGIfb: Can't find XGI %d North Bridge device.\n",
				nbridge_id);
		return FALSE;
	}

	if (set == 0)
		pci_read_config_dword(pdev, offset, (u32 *)value);
	else
		pci_write_config_dword(pdev, offset, (u32)(*value));

	return TRUE;
}
*/
/* ------------------ Internal helper routines ----------------- */

static void XGIfb_search_mode(const char *name)
{
	int i = 0, j = 0;

	if(name == NULL) {
	   printk(KERN_ERR "XGIfb: Internal error, using default mode.\n");
	   XGIfb_mode_idx = DEFAULT_MODE;
	   return;
	}
		
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)		
        if (!strcmp(name, XGIbios_mode[MODE_INDEX_NONE].name)) {
	   printk(KERN_ERR "XGIfb: Mode 'none' not supported anymore. Using default.\n");
	   XGIfb_mode_idx = DEFAULT_MODE;
	   return;
	}
#endif		

	while(XGIbios_mode[i].mode_no != 0) {
		if (!strcmp(name, XGIbios_mode[i].name)) {
			XGIfb_mode_idx = i;
			j = 1;
			break;
		}
		i++;
	}
	if(!j) printk(KERN_INFO "XGIfb: Invalid mode '%s'\n", name);
}

static void XGIfb_search_vesamode(unsigned int vesamode)
{
	int i = 0, j = 0;

	if(vesamode == 0) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
		XGIfb_mode_idx = MODE_INDEX_NONE;
#else
		printk(KERN_ERR "XGIfb: Mode 'none' not supported anymore. Using default.\n");
		XGIfb_mode_idx = DEFAULT_MODE;
#endif		
		return;
	}

	vesamode &= 0x1dff;  /* Clean VESA mode number from other flags */

	while(XGIbios_mode[i].mode_no != 0) {
		if( (XGIbios_mode[i].vesa_mode_no_1 == vesamode) ||
		    (XGIbios_mode[i].vesa_mode_no_2 == vesamode) ) {
			XGIfb_mode_idx = i;
			j = 1;
			break;
		}
		i++;
	}
	if(!j) printk(KERN_INFO "XGIfb: Invalid VESA mode 0x%x'\n", vesamode);
}

static int XGIfb_validate_mode(int myindex)
{
   u16 xres, yres;




    if(!(XGIbios_mode[myindex].chipset & MD_XGI315)) {
        return(-1);
    }


   switch (xgi_video_info.disp_state & DISPTYPE_DISP2) {
     case DISPTYPE_LCD:
	switch (XGIhw_ext.ulCRT2LCDType) {
	case LCD_640x480:
		xres =  640; yres =  480;  break;
	case LCD_800x600:
		xres =  800; yres =  600;  break;
        case LCD_1024x600:
		xres = 1024; yres =  600;  break;		
	case LCD_1024x768:
	 	xres = 1024; yres =  768;  break;
	case LCD_1152x768:
		xres = 1152; yres =  768;  break;		
	case LCD_1280x960:
	        xres = 1280; yres =  960;  break;		
	case LCD_1280x768:
		xres = 1280; yres =  768;  break;
	case LCD_1280x1024:
		xres = 1280; yres = 1024;  break;
	case LCD_1400x1050:
		xres = 1400; yres = 1050;  break;		
	case LCD_1600x1200:
		xres = 1600; yres = 1200;  break;
//	case LCD_320x480:				/* TW: FSTN */
//		xres =  320; yres =  480;  break;
	default:
	        xres =    0; yres =    0;  break;
	}
	if(XGIbios_mode[myindex].xres > xres) {
	        return(-1);
	}
        if(XGIbios_mode[myindex].yres > yres) {
	        return(-1);
	}
	if((XGIhw_ext.ulExternalChip == 0x01) ||   /* LVDS */
           (XGIhw_ext.ulExternalChip == 0x05))    /* LVDS+Chrontel */
	{	
	   switch (XGIbios_mode[myindex].xres) {
	   	case 512:
	       		if(XGIbios_mode[myindex].yres != 512) return -1;
			if(XGIhw_ext.ulCRT2LCDType == LCD_1024x600) return -1;
	       		break;
	   	case 640:
		       	if((XGIbios_mode[myindex].yres != 400) &&
	           	   (XGIbios_mode[myindex].yres != 480))
		          	return -1;
	       		break;
	   	case 800:
		       	if(XGIbios_mode[myindex].yres != 600) return -1;
	       		break;
	   	case 1024:
		       	if((XGIbios_mode[myindex].yres != 600) &&
	           	   (XGIbios_mode[myindex].yres != 768))
		          	return -1;
			if((XGIbios_mode[myindex].yres == 600) &&
			   (XGIhw_ext.ulCRT2LCDType != LCD_1024x600))
			   	return -1;
			break;
		case 1152:
			if((XGIbios_mode[myindex].yres) != 768) return -1;
			if(XGIhw_ext.ulCRT2LCDType != LCD_1152x768) return -1;
			break;
	   	case 1280:
		   	if((XGIbios_mode[myindex].yres != 768) &&
	           	   (XGIbios_mode[myindex].yres != 1024))
		          	return -1;
			if((XGIbios_mode[myindex].yres == 768) &&
			   (XGIhw_ext.ulCRT2LCDType != LCD_1280x768))
			   	return -1;				
			break;
	   	case 1400:
		   	if(XGIbios_mode[myindex].yres != 1050) return -1;
			break;
	   	case 1600:
		   	if(XGIbios_mode[myindex].yres != 1200) return -1;
			break;
	   	default:
		        return -1;		
	   }
	} else {
	   switch (XGIbios_mode[myindex].xres) {
	   	case 512:
	       		if(XGIbios_mode[myindex].yres != 512) return -1;
	       		break;
	   	case 640:
		       	if((XGIbios_mode[myindex].yres != 400) &&
	           	   (XGIbios_mode[myindex].yres != 480))
		          	return -1;
	       		break;
	   	case 800:
		       	if(XGIbios_mode[myindex].yres != 600) return -1;
	       		break;
	   	case 1024:
		       	if(XGIbios_mode[myindex].yres != 768) return -1;
			break;
	   	case 1280:
		   	if((XGIbios_mode[myindex].yres != 960) &&
	           	   (XGIbios_mode[myindex].yres != 1024))
		          	return -1;
			if(XGIbios_mode[myindex].yres == 960) {
			    if(XGIhw_ext.ulCRT2LCDType == LCD_1400x1050) 
			   	return -1;
			}
			break;
	   	case 1400:
		   	if(XGIbios_mode[myindex].yres != 1050) return -1;
			break;
	   	case 1600:
		   	if(XGIbios_mode[myindex].yres != 1200) return -1;
			break;
	   	default:
		        return -1;		
	   }
	}
	break;
     case DISPTYPE_TV:
	switch (XGIbios_mode[myindex].xres) {
	case 512:
	case 640:
	case 800:
		break;
	case 720:
		if (xgi_video_info.TV_type == TVMODE_NTSC) {
			if (XGIbios_mode[myindex].yres != 480) {
				return(-1);
			}
		} else if (xgi_video_info.TV_type == TVMODE_PAL) {
			if (XGIbios_mode[myindex].yres != 576) {
				return(-1);
			}
		}
		/* TW: LVDS/CHRONTEL does not support 720 */
		if (xgi_video_info.hasVB == HASVB_LVDS_CHRONTEL ||
					xgi_video_info.hasVB == HASVB_CHRONTEL) {
				return(-1);
		}
		break;
	case 1024:
		if (xgi_video_info.TV_type == TVMODE_NTSC) {
			if(XGIbios_mode[myindex].bpp == 32) {
			       return(-1);
			}
		}
		/* TW: LVDS/CHRONTEL only supports < 800 (1024 on 650/Ch7019)*/
		if (xgi_video_info.hasVB == HASVB_LVDS_CHRONTEL ||
					xgi_video_info.hasVB == HASVB_CHRONTEL) {
		    if(xgi_video_info.chip < XGI_315H) {
				return(-1);
		    }
		}
		break;
	default:
		return(-1);
	}
	break;
     case DISPTYPE_CRT2:	
        if(XGIbios_mode[myindex].xres > 1280) return -1;
	break;	
     }
     return(myindex);
}

static void XGIfb_search_crt2type(const char *name)
{
	int i = 0;

	if(name == NULL)
		return;

	while(XGI_crt2type[i].type_no != -1) {
		if (!strcmp(name, XGI_crt2type[i].name)) {
			XGIfb_crt2type = XGI_crt2type[i].type_no;
			XGIfb_tvplug = XGI_crt2type[i].tvplug_no;
			break;
		}
		i++;
	}
	if(XGIfb_crt2type < 0)
		printk(KERN_INFO "XGIfb: Invalid CRT2 type: %s\n", name);
}

static void XGIfb_search_queuemode(const char *name)
{
	int i = 0;

	if(name == NULL)
		return;

	while (XGI_queuemode[i].type_no != -1) {
		if (!strcmp(name, XGI_queuemode[i].name)) {
			XGIfb_queuemode = XGI_queuemode[i].type_no;
			break;
		}
		i++;
	}
	if (XGIfb_queuemode < 0)
		printk(KERN_INFO "XGIfb: Invalid queuemode type: %s\n", name);
}

static u8 XGIfb_search_refresh_rate(unsigned int rate)
{
	u16 xres, yres;
	int i = 0;

	xres = XGIbios_mode[XGIfb_mode_idx].xres;
	yres = XGIbios_mode[XGIfb_mode_idx].yres;

	XGIfb_rate_idx = 0;
	while ((XGIfb_vrate[i].idx != 0) && (XGIfb_vrate[i].xres <= xres)) {
		if ((XGIfb_vrate[i].xres == xres) && (XGIfb_vrate[i].yres == yres)) {
			if (XGIfb_vrate[i].refresh == rate) {
				XGIfb_rate_idx = XGIfb_vrate[i].idx;
				break;
			} else if (XGIfb_vrate[i].refresh > rate) {
				if ((XGIfb_vrate[i].refresh - rate) <= 3) {
					DPRINTK("XGIfb: Adjusting rate from %d up to %d\n",
						rate, XGIfb_vrate[i].refresh);
					XGIfb_rate_idx = XGIfb_vrate[i].idx;
					xgi_video_info.refresh_rate = XGIfb_vrate[i].refresh;
				} else if (((rate - XGIfb_vrate[i-1].refresh) <= 2)
						&& (XGIfb_vrate[i].idx != 1)) {
					DPRINTK("XGIfb: Adjusting rate from %d down to %d\n",
						rate, XGIfb_vrate[i-1].refresh);
					XGIfb_rate_idx = XGIfb_vrate[i-1].idx;
					xgi_video_info.refresh_rate = XGIfb_vrate[i-1].refresh;
				} 
				break;
			} else if((rate - XGIfb_vrate[i].refresh) <= 2) {
				DPRINTK("XGIfb: Adjusting rate from %d down to %d\n",
						rate, XGIfb_vrate[i].refresh);
	           		XGIfb_rate_idx = XGIfb_vrate[i].idx;
		   		break;
	       		}
		}
		i++;
	}
	if (XGIfb_rate_idx > 0) {
		return XGIfb_rate_idx;
	} else {
		printk(KERN_INFO
			"XGIfb: Unsupported rate %d for %dx%d\n", rate, xres, yres);
		return 0;
	}
}

static void XGIfb_search_tvstd(const char *name)
{
	int i = 0;

	if(name == NULL)
		return;

	while (XGI_tvtype[i].type_no != -1) {
		if (!strcmp(name, XGI_tvtype[i].name)) {
			XGIfb_tvmode = XGI_tvtype[i].type_no;
			break;
		}
		i++;
	}
}

static BOOLEAN XGIfb_bridgeisslave(void)
{
   unsigned char usScratchP1_00;

   if(xgi_video_info.hasVB == HASVB_NONE) return FALSE;

   inXGIIDXREG(XGIPART1,0x00,usScratchP1_00);
   if( (usScratchP1_00 & 0x50) == 0x10)  {
	   return TRUE;
   } else {
           return FALSE;
   }
}

static BOOLEAN XGIfbcheckvretracecrt1(void)
{
   unsigned char temp;

   inXGIIDXREG(XGICR,0x17,temp);
   if(!(temp & 0x80)) return FALSE;
   

   inXGIIDXREG(XGISR,0x1f,temp);
   if(temp & 0xc0) return FALSE;
   

   if(inXGIREG(XGIINPSTAT) & 0x08) return TRUE;
   else 			   return FALSE;
}

static BOOLEAN XGIfbcheckvretracecrt2(void)
{
   unsigned char temp;
   if(xgi_video_info.hasVB == HASVB_NONE) return FALSE;
   inXGIIDXREG(XGIPART1, 0x30, temp);
   if(temp & 0x02) return FALSE;
   else 	   return TRUE;
}

static BOOLEAN XGIfb_CheckVBRetrace(void) 
{
   if(xgi_video_info.disp_state & DISPTYPE_DISP2) {
      if(XGIfb_bridgeisslave()) {
         return(XGIfbcheckvretracecrt1());
      } else {
         return(XGIfbcheckvretracecrt2());
      }
   } 
   return(XGIfbcheckvretracecrt1());
}

/* ----------- FBDev related routines for all series ----------- */

static int XGIfb_do_set_var(struct fb_var_screeninfo *var, int isactive,
		      struct fb_info *info)
{
	
	unsigned int htotal = var->left_margin + var->xres + 
		var->right_margin + var->hsync_len;
	unsigned int vtotal = var->upper_margin + var->yres + 
		var->lower_margin + var->vsync_len;

	double drate = 0, hrate = 0;
	int found_mode = 0;
	int old_mode;
	unsigned char reg,reg1;
	
	DEBUGPRN("Inside do_set_var");
//        printk(KERN_DEBUG "XGIfb:var->yres=%d, var->upper_margin=%d, var->lower_margin=%d, var->vsync_len=%d\n", var->yres,var->upper_margin,var->lower_margin,var->vsync_len);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
	inXGIIDXREG(XGICR,0x34,reg);
	if(reg & 0x80) {
	   printk(KERN_INFO "XGIfb: Cannot change display mode, X server is active\n");
	   return -EBUSY;
	}
#endif	
        info->var.xres_virtual = var->xres_virtual;
        info->var.yres_virtual = var->yres_virtual;
        info->var.bits_per_pixel = var->bits_per_pixel;

	if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_NONINTERLACED) 
		vtotal <<= 1;
	else if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_DOUBLE)
		vtotal <<= 2;
	else if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED)
	{	
//		vtotal <<= 1;
//		var->yres <<= 1;
	}

	if(!htotal || !vtotal) {
		DPRINTK("XGIfb: Invalid 'var' information\n");
		return -EINVAL;
	}
//        printk(KERN_DEBUG "XGIfb: var->pixclock=%d, htotal=%d, vtotal=%d\n",
//                var->pixclock,htotal,vtotal);

	if(var->pixclock && htotal && vtotal) {
	   drate = 1E12 / var->pixclock;
	   hrate = drate / htotal;
	   xgi_video_info.refresh_rate = (unsigned int) (hrate / vtotal * 2 + 0.5);
	} else xgi_video_info.refresh_rate = 60;

	/* TW: Calculation wrong for 1024x600 - force it to 60Hz */
	if((var->xres == 1024) && (var->yres == 600))  xgi_video_info.refresh_rate = 60;

	printk(KERN_DEBUG "XGIfb: Change mode to %dx%dx%d-%dHz\n",
		var->xres,var->yres,var->bits_per_pixel,xgi_video_info.refresh_rate);

	old_mode = XGIfb_mode_idx;
	XGIfb_mode_idx = 0;

	while( (XGIbios_mode[XGIfb_mode_idx].mode_no != 0) &&
	       (XGIbios_mode[XGIfb_mode_idx].xres <= var->xres) ) {
		if( (XGIbios_mode[XGIfb_mode_idx].xres == var->xres) &&
		    (XGIbios_mode[XGIfb_mode_idx].yres == var->yres) &&
		    (XGIbios_mode[XGIfb_mode_idx].bpp == var->bits_per_pixel)) {
			XGIfb_mode_no = XGIbios_mode[XGIfb_mode_idx].mode_no;
			found_mode = 1;
			break;
		}
		XGIfb_mode_idx++;
	}

	if(found_mode)
		XGIfb_mode_idx = XGIfb_validate_mode(XGIfb_mode_idx);
	else
		XGIfb_mode_idx = -1;

       	if(XGIfb_mode_idx < 0) {
		printk(KERN_ERR "XGIfb: Mode %dx%dx%d not supported\n", var->xres,
		       var->yres, var->bits_per_pixel);
		XGIfb_mode_idx = old_mode;
		return -EINVAL;
	}

	if(XGIfb_search_refresh_rate(xgi_video_info.refresh_rate) == 0) {
		XGIfb_rate_idx = XGIbios_mode[XGIfb_mode_idx].rate_idx;
		xgi_video_info.refresh_rate = 60;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
                                                                                
	if(((var->activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_NOW) && isactive) {

#else
	if(isactive) {
#endif
		XGIfb_pre_setmode();

		if(XGISetModeNew( &XGIhw_ext, XGIfb_mode_no) == 0) {
			printk(KERN_ERR "XGIfb: Setting mode[0x%x] failed\n", XGIfb_mode_no);
			return -EINVAL;
		}
	info->fix.line_length = ((info->var.xres_virtual * info->var.bits_per_pixel)>>6);
//	info->fix.visual= FB_VISUAL_DIRECTCOLOR;
	outXGIIDXREG(XGISR,IND_XGI_PASSWORD,XGI_PASSWORD);
//        if(var->xres_virtual == 1024)
//	{
		outXGIIDXREG(XGICR,0x13,(info->fix.line_length & 0x00ff));
		outXGIIDXREG(XGISR,0x0E,(info->fix.line_length & 0xff00)>>8);
//	} 
		outXGIIDXREG(XGISR, IND_XGI_PASSWORD, XGI_PASSWORD);

		XGIfb_post_setmode();

		DPRINTK("XGIfb: Set new mode: %dx%dx%d-%d \n",
			XGIbios_mode[XGIfb_mode_idx].xres,
			XGIbios_mode[XGIfb_mode_idx].yres,
			XGIbios_mode[XGIfb_mode_idx].bpp,
			xgi_video_info.refresh_rate);

		xgi_video_info.video_bpp = XGIbios_mode[XGIfb_mode_idx].bpp;
		xgi_video_info.video_vwidth = info->var.xres_virtual;
		xgi_video_info.video_width = XGIbios_mode[XGIfb_mode_idx].xres;
		xgi_video_info.video_vheight = info->var.yres_virtual;
		xgi_video_info.video_height = XGIbios_mode[XGIfb_mode_idx].yres;
		xgi_video_info.org_x = xgi_video_info.org_y = 0;
		xgi_video_info.video_linelength = info->var.xres_virtual * (xgi_video_info.video_bpp >> 3);
		xgi_video_info.accel = 0;
		if(XGIfb_accel) {
		   xgi_video_info.accel = (var->accel_flags & FB_ACCELF_TEXT) ? -1 : 0;
		}
		switch(xgi_video_info.video_bpp) {
        	case 8:
            		xgi_video_info.DstColor = 0x0000;
	    		xgi_video_info.XGI310_AccelDepth = 0x00000000;
			xgi_video_info.video_cmap_len = 256;
            		break;
        	case 16:
            		xgi_video_info.DstColor = 0x8000;
            		xgi_video_info.XGI310_AccelDepth = 0x00010000;
			xgi_video_info.video_cmap_len = 16;
            		break;
        	case 32:
            		xgi_video_info.DstColor = 0xC000;
	    		xgi_video_info.XGI310_AccelDepth = 0x00020000;
			xgi_video_info.video_cmap_len = 16;
            		break;
		default:
			xgi_video_info.video_cmap_len = 16;
		        printk(KERN_ERR "XGIfb: Unsupported depth %d", xgi_video_info.video_bpp);
			xgi_video_info.accel = 0;
			break;
    		}
	}
	DEBUGPRN("End of do_set_var");
	return 0;
}

#ifdef XGIFB_PAN
static int XGIfb_pan_var(struct fb_var_screeninfo *var)
{
	unsigned int base;

//	printk("Inside pan_var");
	
	if (var->xoffset > (var->xres_virtual - var->xres)) {
//	        printk( "Pan: xo: %d xv %d xr %d\n",
//			var->xoffset, var->xres_virtual, var->xres);
		return -EINVAL;
	}
	if(var->yoffset > (var->yres_virtual - var->yres)) {
//		printk( "Pan: yo: %d yv %d yr %d\n",
//			var->yoffset, var->yres_virtual, var->yres);
		return -EINVAL;
	}
        base = var->yoffset * var->xres_virtual + var->xoffset;

        /* calculate base bpp dep. */
        switch(var->bits_per_pixel) {
        case 16:
        	base >>= 1;
        	break;
	case 32:
            	break;
	case 8:
        default:
        	base >>= 2;
            	break;
        }
	
	outXGIIDXREG(XGISR, IND_XGI_PASSWORD, XGI_PASSWORD);

        outXGIIDXREG(XGICR, 0x0D, base & 0xFF);
	outXGIIDXREG(XGICR, 0x0C, (base >> 8) & 0xFF);
	outXGIIDXREG(XGISR, 0x0D, (base >> 16) & 0xFF);
        outXGIIDXREG(XGISR, 0x37, (base >> 24) & 0x03);
	setXGIIDXREG(XGISR, 0x37, 0xDF, (base >> 21) & 0x04);

        if(xgi_video_info.disp_state & DISPTYPE_DISP2) {
		orXGIIDXREG(XGIPART1, XGIfb_CRT2_write_enable, 0x01);
        	outXGIIDXREG(XGIPART1, 0x06, (base & 0xFF));
        	outXGIIDXREG(XGIPART1, 0x05, ((base >> 8) & 0xFF));
        	outXGIIDXREG(XGIPART1, 0x04, ((base >> 16) & 0xFF));
		setXGIIDXREG(XGIPART1, 0x02, 0x7F, ((base >> 24) & 0x01) << 7);
        }
//	printk("End of pan_var");
	return 0;
}
#endif

static void XGIfb_bpp_to_var(struct fb_var_screeninfo *var)
{
	switch(var->bits_per_pixel) {
	   case 8:
	   	var->red.offset = var->green.offset = var->blue.offset = 0;
		var->red.length = var->green.length = var->blue.length = 6;
		xgi_video_info.video_cmap_len = 256;
		break;
	   case 16:
		var->red.offset = 11;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 0;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		xgi_video_info.video_cmap_len = 16;
		break;
	   case 32:
		var->red.offset = 16;
		var->red.length = 8;
		var->green.offset = 8;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		xgi_video_info.video_cmap_len = 16;
		break;
	}
}

void XGI_dispinfo(struct ap_data *rec)
{
	rec->minfo.bpp    = xgi_video_info.video_bpp;
	rec->minfo.xres   = xgi_video_info.video_width;
	rec->minfo.yres   = xgi_video_info.video_height;
	rec->minfo.v_xres = xgi_video_info.video_vwidth;
	rec->minfo.v_yres = xgi_video_info.video_vheight;
	rec->minfo.org_x  = xgi_video_info.org_x;
	rec->minfo.org_y  = xgi_video_info.org_y;
	rec->minfo.vrate  = xgi_video_info.refresh_rate;
	rec->iobase       = xgi_video_info.vga_base - 0x30;
	rec->mem_size     = xgi_video_info.video_size;
	rec->disp_state   = xgi_video_info.disp_state; 
	rec->version      = (VER_MAJOR << 24) | (VER_MINOR << 16) | VER_LEVEL; 
	rec->hasVB        = xgi_video_info.hasVB; 
	rec->TV_type      = xgi_video_info.TV_type; 
	rec->TV_plug      = xgi_video_info.TV_plug; 
	rec->chip         = xgi_video_info.chip;
}

/* ------------ FBDev related routines for 2.4 series ----------- */

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)

static void XGIfb_crtc_to_var(struct fb_var_screeninfo *var)
{
	u16 VRE, VBE, VRS, VBS, VDE, VT;
	u16 HRE, HBE, HRS, HBS, HDE, HT;
	u8  sr_data, cr_data, cr_data2, cr_data3, mr_data;
	int A, B, C, D, E, F, temp;
	double hrate, drate;

//	printk("\ncrt_to_var1:var->yres_virtual = %d",var->yres_virtual);
	DEBUGPRN("Inside crtc_to_var");
	inXGIIDXREG(XGISR, IND_XGI_COLOR_MODE, sr_data);

	if (sr_data & XGI_INTERLACED_MODE)
		var->vmode = FB_VMODE_INTERLACED;
	else
		var->vmode = FB_VMODE_NONINTERLACED;

	switch ((sr_data & 0x1C) >> 2) {
	   case XGI_8BPP_COLOR_MODE:
		var->bits_per_pixel = 8;
		break;
	   case XGI_16BPP_COLOR_MODE:
		var->bits_per_pixel = 16;
		break;
	   case XGI_32BPP_COLOR_MODE:
		var->bits_per_pixel = 32;
		break;
	}

	XGIfb_bpp_to_var(var);
	
	inXGIIDXREG(XGISR, 0x0A, sr_data);

        inXGIIDXREG(XGICR, 0x06, cr_data);

        inXGIIDXREG(XGICR, 0x07, cr_data2);

	VT = (cr_data & 0xFF) | ((u16) (cr_data2 & 0x01) << 8) |
	     ((u16) (cr_data2 & 0x20) << 4) | ((u16) (sr_data & 0x01) << 10);
	A = VT + 2;

	inXGIIDXREG(XGICR, 0x12, cr_data);

	VDE = (cr_data & 0xff) | ((u16) (cr_data2 & 0x02) << 7) |
	      ((u16) (cr_data2 & 0x40) << 3) | ((u16) (sr_data & 0x02) << 9);
	E = VDE + 1;

	inXGIIDXREG(XGICR, 0x10, cr_data);

	VRS = (cr_data & 0xff) | ((u16) (cr_data2 & 0x04) << 6) |
	      ((u16) (cr_data2 & 0x80) << 2) | ((u16) (sr_data & 0x08) << 7);
	F = VRS + 1 - E;

	inXGIIDXREG(XGICR, 0x15, cr_data);

	inXGIIDXREG(XGICR, 0x09, cr_data3);

	VBS = (cr_data & 0xff) | ((u16) (cr_data2 & 0x08) << 5) |
	      ((u16) (cr_data3 & 0x20) << 4) | ((u16) (sr_data & 0x04) << 8);

	inXGIIDXREG(XGICR, 0x16, cr_data);

	VBE = (cr_data & 0xff) | ((u16) (sr_data & 0x10) << 4);
	temp = VBE - ((E - 1) & 511);
	B = (temp > 0) ? temp : (temp + 512);

	inXGIIDXREG(XGICR, 0x11, cr_data);

	VRE = (cr_data & 0x0f) | ((sr_data & 0x20) >> 1);
	temp = VRE - ((E + F - 1) & 31);
	C = (temp > 0) ? temp : (temp + 32);

	D = B - F - C;
        var->yres = E;
#ifndef XGIFB_PAN
	var->yres_virtual = E;
#endif
	/* TW: We have to report the physical dimension to the console! */
	if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED) {
		var->yres <<= 1;
#ifndef XGIFB_PAN
		var->yres_virtual <<= 1;
#endif
	} else if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_DOUBLE) {
		var->yres >>= 1;
#ifndef XGIFB_PAN
		var->yres_virtual >>= 1;
#endif
	}

	var->upper_margin = D;
	var->lower_margin = F;
	var->vsync_len = C;

	inXGIIDXREG(XGISR, 0x0b, sr_data);

	inXGIIDXREG(XGICR, 0x00, cr_data);

	HT = (cr_data & 0xff) | ((u16) (sr_data & 0x03) << 8);
	A = HT + 5;

	inXGIIDXREG(XGICR, 0x01, cr_data);

	HDE = (cr_data & 0xff) | ((u16) (sr_data & 0x0C) << 6);
	E = HDE + 1;

	inXGIIDXREG(XGICR, 0x04, cr_data);

	HRS = (cr_data & 0xff) | ((u16) (sr_data & 0xC0) << 2);
	F = HRS - E - 3;

	inXGIIDXREG(XGICR, 0x02, cr_data);

	HBS = (cr_data & 0xff) | ((u16) (sr_data & 0x30) << 4);

	inXGIIDXREG(XGISR, 0x0c, sr_data);

	inXGIIDXREG(XGICR, 0x03, cr_data);

	inXGIIDXREG(XGICR, 0x05, cr_data2);

	HBE = (cr_data & 0x1f) | ((u16) (cr_data2 & 0x80) >> 2) |
	      ((u16) (sr_data & 0x03) << 6);
	HRE = (cr_data2 & 0x1f) | ((sr_data & 0x04) << 3);

	temp = HBE - ((E - 1) & 255);
	B = (temp > 0) ? temp : (temp + 256);

	temp = HRE - ((E + F + 3) & 63);
	C = (temp > 0) ? temp : (temp + 64);

	D = B - F - C;

	var->xres = E * 8;
	var->left_margin = D * 8;
	var->right_margin = F * 8;
	var->hsync_len = C * 8;

//	var->activate = FB_ACTIVATE_NOW;
	var->sync = 0;

	mr_data = inXGIREG(XGIMISCR);
	if (mr_data & 0x80)
		var->sync &= ~FB_SYNC_VERT_HIGH_ACT;
	else
		var->sync |= FB_SYNC_VERT_HIGH_ACT;

	if (mr_data & 0x40)
		var->sync &= ~FB_SYNC_HOR_HIGH_ACT;
	else
		var->sync |= FB_SYNC_HOR_HIGH_ACT;

	VT += 2;
	VT <<= 1;
	HT = (HT + 5) * 8;

	hrate = (double) xgi_video_info.refresh_rate * (double) VT / 2;
	drate = hrate * HT;
	var->pixclock = (u32) (1E12 / drate);

#ifdef XGIFB_PAN
	if(XGIfb_ypan) {
	   // var->yres_virtual = xgi_video_info.heapstart / (var->xres * (var->bits_per_pixel >> 3));
	    if(var->yres_virtual <= var->yres) {
	        var->yres_virtual = var->yres;
	    }
	    if(var->xres_virtual <= var->xres) {
	        var->xres_virtual = var->xres;
	    }
	} else
#endif
	   var->yres_virtual = var->yres;
//       printk("\ncrt_to_var2:var->yres_virtual = %d",var->yres_virtual);
}

static int XGI_getcolreg(unsigned regno, unsigned *red, unsigned *green, unsigned *blue,
			 unsigned *transp, struct fb_info *fb_info)
{
	if (regno >= xgi_video_info.video_cmap_len)
		return 1;

	*red = XGI_palette[regno].red;
	*green = XGI_palette[regno].green;
	*blue = XGI_palette[regno].blue;
	*transp = 0;
	return 0;
}

static int XGIfb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
                           unsigned transp, struct fb_info *fb_info)
{
	if (regno >= xgi_video_info.video_cmap_len)
		return 1;

	XGI_palette[regno].red = red;
	XGI_palette[regno].green = green;
	XGI_palette[regno].blue = blue;

	switch (xgi_video_info.video_bpp) {
#ifdef FBCON_HAS_CFB8
	case 8:
	        outXGIREG(XGIDACA, regno);
		outXGIREG(XGIDACD, (red >> 10));
		outXGIREG(XGIDACD, (green >> 10));
		outXGIREG(XGIDACD, (blue >> 10));
		if (xgi_video_info.disp_state & DISPTYPE_DISP2) {
		        outXGIREG(XGIDAC2A, regno);
			outXGIREG(XGIDAC2D, (red >> 8));
			outXGIREG(XGIDAC2D, (green >> 8));
			outXGIREG(XGIDAC2D, (blue >> 8));
		}
		break;
#endif
#ifdef FBCON_HAS_CFB16
	case 16:
		XGI_fbcon_cmap.cfb16[regno] =
		    ((red & 0xf800)) | ((green & 0xfc00) >> 5) | ((blue & 0xf800) >> 11);
		break;
#endif
#ifdef FBCON_HAS_CFB32
	case 32:
		red >>= 8;
		green >>= 8;
		blue >>= 8;
		XGI_fbcon_cmap.cfb32[regno] = (red << 16) | (green << 8) | (blue);
		break;
#endif
	}
	return 0;
}

static void XGIfb_set_disp(int con, struct fb_var_screeninfo *var,
                           struct fb_info *info)
{
	struct fb_fix_screeninfo fix;
	long   flags;
	struct display *display;
	struct display_switch *sw;

	if(con >= 0)
		display = &fb_display[con];
	else
		display = &XGI_disp;

	XGIfb_get_fix(&fix, con, 0);

	display->screen_base = xgi_video_info.video_vbase;
	display->visual = fix.visual;
	display->type = fix.type;
	display->type_aux = fix.type_aux;
	display->ypanstep = fix.ypanstep;
	display->ywrapstep = fix.ywrapstep;
	display->line_length = fix.line_length;
	display->next_line = fix.line_length;
	display->can_soft_blank = 0;
	display->inverse = XGIfb_inverse;
	display->var = *var;

	save_flags(flags);

	switch (xgi_video_info.video_bpp) {
#ifdef FBCON_HAS_CFB8
	   case 8:
#ifdef XGIFBACCEL
		sw = xgi_video_info.accel ? &fbcon_XGI8 : &fbcon_cfb8;
#else
		sw = &fbcon_cfb8;
#endif
		break;
#endif
#ifdef FBCON_HAS_CFB16
	   case 16:
#ifdef XGIFBACCEL
		sw = xgi_video_info.accel ? &fbcon_XGI16 : &fbcon_cfb16;
#else
		sw = &fbcon_cfb16;
#endif
		display->dispsw_data = XGI_fbcon_cmap.cfb16;
		break;
#endif
#ifdef FBCON_HAS_CFB32
	   case 32:
#ifdef XGIFBACCEL
		sw = xgi_video_info.accel ? &fbcon_XGI32 : &fbcon_cfb32;
#else
		sw = &fbcon_cfb32;
#endif
		display->dispsw_data = XGI_fbcon_cmap.cfb32;
		break;
#endif
	   default:
		sw = &fbcon_dummy;
		return;
	}
	memcpy(&XGIfb_sw, sw, sizeof(*sw));
	display->dispsw = &XGIfb_sw;
	restore_flags(flags);

#ifdef XGIFB_PAN
        if((xgi_video_info.accel) && (XGIfb_ypan)) {
  	    /* display->scrollmode = SCROLL_YPAN; - not defined */
	} else {
	    display->scrollmode = SCROLL_YREDRAW;
	    XGIfb_sw.bmove = fbcon_redraw_bmove;
	}
#else
	display->scrollmode = SCROLL_YREDRAW;
	XGIfb_sw.bmove = fbcon_redraw_bmove;
#endif
//    default_var.xres_virtual = var->xres_virtual; 
//	default_var.xres=var->xres;
//    default_var.yres_virtual = var->yres_virtual;
//	default_var.yres=var->yres;
    //memcpy(&display->var, &default_var, sizeof(struct fb_var_screeninfo));
//	printk("set_disp:var x=%d,y=%d,xv=%d,yv=%d\n",var->xres,var->yres,var->xres_virtual,var->yres_virtual);


}

static void XGIfb_do_install_cmap(int con, struct fb_info *info)
{
        if (con != currcon)
		return;

        if (fb_display[con].cmap.len)
		fb_set_cmap(&fb_display[con].cmap, 1, XGIfb_setcolreg, info);
        else
		fb_set_cmap(fb_default_cmap(xgi_video_info.video_cmap_len), 1,
			    XGIfb_setcolreg, info);
}


static int XGIfb_get_var(struct fb_var_screeninfo *var, int con,
			 struct fb_info *info)
{
	DEBUGPRN("inside get_var");
	if(con == -1)
		memcpy(var, &default_var, sizeof(struct fb_var_screeninfo));
	else
		*var = fb_display[con].var;

 	/* For FSTN, DSTN */
	if (var->xres == 320 && var->yres == 480)
		var->yres = 240;
		
	DEBUGPRN("end of get_var");
	return 0;
}

static int XGIfb_set_var(struct fb_var_screeninfo *var, int con,
			 struct fb_info *info)
{
	int err;
	unsigned int cols, rows;
	unsigned char reg;


    
    
//	fb_display[con].var.activate = FB_ACTIVATE_NOW;
        if(XGIfb_do_set_var(var, con == currcon, info)) {
		//XGIfb_crtc_to_var(var);
		return -EINVAL;
	}
//   if ((var->activate & FB_ACTIVATE_MASK) == FB_ACTIVATE_TEST) return 0;

	switch (var->activate & FB_ACTIVATE_MASK) {
	case FB_ACTIVATE_TEST:
		DPRINTK("EXIT - FB_ACTIVATE_TEST\n");
		return 0;
	case FB_ACTIVATE_NXTOPEN:	/* ?? */
	case FB_ACTIVATE_NOW:
		break;		/* continue */
	default:
		DPRINTK("EXIT - unknown activation type\n");
		return -EINVAL;	/* unknown */
	}

	XGIfb_crtc_to_var(var);
	XGIfb_set_disp(con, var, info);

	if(info->changevar)
		(*info->changevar) (con);

	if((err = fb_alloc_cmap(&fb_display[con].cmap, 0, 0)))
		return err;

	XGIfb_do_install_cmap(con, info);

	cols = XGIbios_mode[XGIfb_mode_idx].cols;
	rows = XGIbios_mode[XGIfb_mode_idx].rows;
	vc_resize_con(rows, cols, fb_display[con].conp->vc_num);

	DEBUGPRN("end of set_var");

	return 0;
}

static int XGIfb_get_cmap(struct fb_cmap *cmap, int kspc, int con,
			  struct fb_info *info)
{
	DEBUGPRN("inside get_cmap");
        if (con == currcon)
		return fb_get_cmap(cmap, kspc, XGI_getcolreg, info);

	else if (fb_display[con].cmap.len)
		fb_copy_cmap(&fb_display[con].cmap, cmap, kspc ? 0 : 2);
	else
		fb_copy_cmap(fb_default_cmap(xgi_video_info.video_cmap_len), cmap, kspc ? 0 : 2);

	DEBUGPRN("end of get_cmap");
	return 0;
}

static int XGIfb_set_cmap(struct fb_cmap *cmap, int kspc, int con,
			  struct fb_info *info)
{
	int err;

	DEBUGPRN("inside set_cmap");
	if (!fb_display[con].cmap.len) {
		err = fb_alloc_cmap(&fb_display[con].cmap, xgi_video_info.video_cmap_len, 0);
		if (err)
			return err;
	}
        
	if (con == currcon)
		return fb_set_cmap(cmap, kspc, XGIfb_setcolreg, info);

	else
		fb_copy_cmap(cmap, &fb_display[con].cmap, kspc ? 0 : 1);
	DEBUGPRN("end of set_cmap");
	return 0;
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#ifdef XGIFB_PAN
static int XGIfb_pan_display(struct fb_var_screeninfo *var, int con,
			     struct fb_info* info)
{
	int err;
	

	if (var->vmode & FB_VMODE_YWRAP) {

		if (var->yoffset < 0 || var->yoffset >= fb_display[con].var.yres_virtual || var->xoffset)
			return -EINVAL;

	} else {

		if (var->xoffset+fb_display[con].var.xres > fb_display[con].var.xres_virtual ||
		    var->yoffset+fb_display[con].var.yres > fb_display[con].var.yres_virtual)
			return -EINVAL;

	}

        if(con == currcon) 
	{  

	   if((err = XGIfb_pan_var(var)) < 0) return err;
	}

	fb_display[con].var.xoffset = var->xoffset;
	fb_display[con].var.yoffset = var->yoffset;
	if (var->vmode & FB_VMODE_YWRAP)
		fb_display[con].var.vmode |= FB_VMODE_YWRAP;
	else
		fb_display[con].var.vmode &= ~FB_VMODE_YWRAP;

	DEBUGPRN("end of pan_display");
	return 0;
}
#endif
#endif
static int XGIfb_mmap(struct fb_info *info, struct file *file,
		      struct vm_area_struct *vma)
{
	struct fb_var_screeninfo var;
	unsigned long start;
	unsigned long off;
	u32 len, mmio_off;

	DEBUGPRN("inside mmap");
	if(vma->vm_pgoff > (~0UL >> PAGE_SHIFT))  return -EINVAL;

	off = vma->vm_pgoff << PAGE_SHIFT;

	start = (unsigned long) xgi_video_info.video_base;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + xgi_video_info.video_size);
#if 0
	if (off >= len) {
		off -= len;
#endif
	/* By Jake Page: Treat mmap request with offset beyond heapstart
	 *               as request for mapping the mmio area 
	 */
	mmio_off = PAGE_ALIGN((start & ~PAGE_MASK) + xgi_video_info.heapstart);
	if(off >= mmio_off) {
		off -= mmio_off;		
		XGIfb_get_var(&var, currcon, info);
		if(var.accel_flags) return -EINVAL;

		start = (unsigned long) xgi_video_info.mmio_base;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + XGIfb_mmio_size);
	}

	start &= PAGE_MASK;
	if((vma->vm_end - vma->vm_start + off) > len)	return -EINVAL;

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;   /* by Jake Page; is that really needed? */

#if defined(__i386__) || defined(__x86_64__)
	if (boot_cpu_data.x86 > 3)
		pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
#endif
	if (io_remap_page_range(vma, vma->vm_start, off, vma->vm_end - vma->vm_start,vma->vm_page_prot))
		return -EAGAIN;

        DEBUGPRN("end of mmap");
	return 0;
}

static void XGI_get_glyph(struct fb_info *info, XGI_GLYINFO *gly)
{
	struct display *p = &fb_display[currcon];
	u16 c;
	u8 *cdat;
	int widthb;
	u8 *gbuf = gly->gmask;
	int size;

	DEBUGPRN("Inside get_glyph");
	gly->fontheight = fontheight(p);
	gly->fontwidth = fontwidth(p);
	widthb = (fontwidth(p) + 7) / 8;

	c = gly->ch & p->charmask;
	if (fontwidth(p) <= 8)
		cdat = p->fontdata + c * fontheight(p);
	else
		cdat = p->fontdata + (c * fontheight(p) << 1);

	size = fontheight(p) * widthb;
	memcpy(gbuf, cdat, size);
	gly->ngmask = size;
	DEBUGPRN("End of get_glyph");
}

static int XGIfb_update_var(int con, struct fb_info *info)
{
#ifdef XGIFB_PAN
        return(XGIfb_pan_var(&fb_display[con].var));
#else
	return 0;
#endif	
}

static int XGIfb_switch(int con, struct fb_info *info)
{
	int cols, rows;

        if(fb_display[currcon].cmap.len)
		fb_get_cmap(&fb_display[currcon].cmap, 1, XGI_getcolreg, info);

	fb_display[con].var.activate = FB_ACTIVATE_NOW;

	if(!memcmp(&fb_display[con].var, &fb_display[currcon].var,
	                           sizeof(struct fb_var_screeninfo))) {
		currcon = con;
		return 1;
	}

	currcon = con;

	XGIfb_do_set_var(&fb_display[con].var, 1, info);

	XGIfb_set_disp(con, &fb_display[con].var, info);

	XGIfb_do_install_cmap(con, info);

	cols = XGIbios_mode[XGIfb_mode_idx].cols;
	rows = XGIbios_mode[XGIfb_mode_idx].rows;
	vc_resize_con(rows, cols, fb_display[con].conp->vc_num);

	XGIfb_update_var(con, info);

	return 1;
}

static void XGIfb_blank(int blank, struct fb_info *info)
{
	u8 reg;

	inXGIIDXREG(XGICR, 0x17, reg);

	if(blank > 0)
		reg &= 0x7f;
	else
		reg |= 0x80;

        outXGIIDXREG(XGICR, 0x17, reg);		
	outXGIIDXREG(XGISR, 0x00, 0x01);    /* Synchronous Reset */
	outXGIIDXREG(XGISR, 0x00, 0x03);    /* End Reset */
//	printk(KERN_DEBUG "XGIfb_blank() called (%d)\n", blank);
}


static int XGIfb_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg, int con,
		       struct fb_info *info)
{
	DEBUGPRN("inside ioctl");
	switch (cmd) {
	   case FBIO_ALLOC:
		if (!capable(CAP_SYS_RAWIO))
			return -EPERM;
		XGI_malloc((struct XGI_memreq *) arg);
		break;
	   case FBIO_FREE:
		if (!capable(CAP_SYS_RAWIO))
			return -EPERM;
		XGI_free(*(unsigned long *) arg);
		break;
	   case FBIOGET_GLYPH:
                XGI_get_glyph(info,(XGI_GLYINFO *) arg);
		break;	
	   case FBIOGET_HWCINFO:
		{
			unsigned long *hwc_offset = (unsigned long *) arg;

			if (XGIfb_caps & HW_CURSOR_CAP)
				*hwc_offset = XGIfb_hwcursor_vbase -
				    (unsigned long) xgi_video_info.video_vbase;
			else
				*hwc_offset = 0;

			break;
		}
	   case FBIOPUT_MODEINFO:
		{
			struct mode_info *x = (struct mode_info *)arg;

			xgi_video_info.video_bpp        = x->bpp;
			xgi_video_info.video_width      = x->xres;
			xgi_video_info.video_height     = x->yres;
			xgi_video_info.video_vwidth     = x->v_xres;
			xgi_video_info.video_vheight    = x->v_yres;
			xgi_video_info.org_x            = x->org_x;
			xgi_video_info.org_y            = x->org_y;
			xgi_video_info.refresh_rate     = x->vrate;
			xgi_video_info.video_linelength = xgi_video_info.video_vwidth * (xgi_video_info.video_bpp >> 3);
			switch(xgi_video_info.video_bpp) {
        		case 8:
            			xgi_video_info.DstColor = 0x0000;
	    			xgi_video_info.XGI310_AccelDepth = 0x00000000;
				xgi_video_info.video_cmap_len = 256;
            			break;
        		case 16:
            			xgi_video_info.DstColor = 0x8000;
            			xgi_video_info.XGI310_AccelDepth = 0x00010000;
				xgi_video_info.video_cmap_len = 16;
            			break;
        		case 32:
            			xgi_video_info.DstColor = 0xC000;
	    			xgi_video_info.XGI310_AccelDepth = 0x00020000;
				xgi_video_info.video_cmap_len = 16;
            			break;
			default:
				xgi_video_info.video_cmap_len = 16;
		       	 	printk(KERN_ERR "XGIfb: Unsupported depth %d", xgi_video_info.video_bpp);
				xgi_video_info.accel = 0;
				break;
    			}

			break;
		}
	   case FBIOGET_DISPINFO:
		XGI_dispinfo((struct ap_data *)arg);
		break;
	   case XGIFB_GET_INFO:  /* TW: New for communication with X driver */
	        {
			XGIfb_info *x = (XGIfb_info *)arg;

			x->XGIfb_id = XGIFB_ID;
			x->XGIfb_version = VER_MAJOR;
			x->XGIfb_revision = VER_MINOR;
			x->XGIfb_patchlevel = VER_LEVEL;
			x->chip_id = xgi_video_info.chip_id;
			x->memory = xgi_video_info.video_size / 1024;
			x->heapstart = xgi_video_info.heapstart / 1024;
			x->fbvidmode = XGIfb_mode_no;
			x->XGIfb_caps = XGIfb_caps;
			x->XGIfb_tqlen = 512; /* yet unused */
			x->XGIfb_pcibus = xgi_video_info.pcibus;
			x->XGIfb_pcislot = xgi_video_info.pcislot;
			x->XGIfb_pcifunc = xgi_video_info.pcifunc;
			x->XGIfb_lcdpdc = XGIfb_detectedpdc;
			x->XGIfb_lcda = XGIfb_detectedlcda;
	                break;
		}
	   case XGIFB_GET_VBRSTATUS:
	        {
			unsigned long *vbrstatus = (unsigned long *) arg;
			if(XGIfb_CheckVBRetrace()) *vbrstatus = 1;
			else		           *vbrstatus = 0;
		}
	   default:
		return -EINVAL;
	}
	DEBUGPRN("end of ioctl");
	return 0;

}
#endif

/* ------------ FBDev related routines for 2.5 series ----------- */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)

static int XGIfb_open(struct fb_info *info, int user)
{
    return 0;
}

static int XGIfb_release(struct fb_info *info, int user)
{
    return 0;
}

static int XGIfb_get_cmap_len(const struct fb_var_screeninfo *var)
{
	int rc = 16;		

	switch(var->bits_per_pixel) {
	case 8:
		rc = 256;	
		break;
	case 16:
		rc = 16;	
		break;		
	case 32:
		rc = 16;
		break;	
	}
	return rc;
}

static int XGIfb_setcolreg(unsigned regno, unsigned red, unsigned green, unsigned blue,
                           unsigned transp, struct fb_info *info)
{
	if (regno >= XGIfb_get_cmap_len(&info->var))
		return 1;

	switch (info->var.bits_per_pixel) {
	case 8:
	        outXGIREG(XGIDACA, regno);
		outXGIREG(XGIDACD, (red >> 10));
		outXGIREG(XGIDACD, (green >> 10));
		outXGIREG(XGIDACD, (blue >> 10));
		if (xgi_video_info.disp_state & DISPTYPE_DISP2) {
		        outXGIREG(XGIDAC2A, regno);
			outXGIREG(XGIDAC2D, (red >> 8));
			outXGIREG(XGIDAC2D, (green >> 8));
			outXGIREG(XGIDAC2D, (blue >> 8));
		}
		break;
	case 16:
		((u32 *)(info->pseudo_palette))[regno] =
		    ((red & 0xf800)) | ((green & 0xfc00) >> 5) | ((blue & 0xf800) >> 11);
		break;
	case 32:
		red >>= 8;
		green >>= 8;
		blue >>= 8;
		((u32 *) (info->pseudo_palette))[regno] = 
			(red << 16) | (green << 8) | (blue);
		break;
	}
	return 0;
}

static int XGIfb_set_par(struct fb_info *info)
{
	int err;

//	printk("XGIfb: inside set_par\n");
        if((err = XGIfb_do_set_var(&info->var, 1, info)))
		return err;

	XGIfb_get_fix(&info->fix, info->currcon, info);

//	printk("XGIfb:end of set_par\n");
	return 0;
}

static int XGIfb_check_var(struct fb_var_screeninfo *var,
                           struct fb_info *info)
{
	unsigned int htotal =
		var->left_margin + var->xres + var->right_margin +
		var->hsync_len;
	unsigned int vtotal = 0;
	double drate = 0, hrate = 0;
	int found_mode = 0;
	int refresh_rate, search_idx;

	DEBUGPRN("Inside check_var");

	if((var->vmode & FB_VMODE_MASK) == FB_VMODE_NONINTERLACED) {
		vtotal = var->upper_margin + var->yres + var->lower_margin +
		         var->vsync_len;   
		vtotal <<= 1;
	} else if((var->vmode & FB_VMODE_MASK) == FB_VMODE_DOUBLE) {
		vtotal = var->upper_margin + var->yres + var->lower_margin +
		         var->vsync_len;   
		vtotal <<= 2;
	} else if((var->vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED) {
		vtotal = var->upper_margin + (var->yres/2) + var->lower_margin +
		         var->vsync_len;   
	} else 	vtotal = var->upper_margin + var->yres + var->lower_margin +
		         var->vsync_len;

	if(!(htotal) || !(vtotal)) {
		XGIFAIL("XGIfb: no valid timing data");
	}

	if((var->pixclock) && (htotal)) {
	   drate = 1E12 / var->pixclock;
	   hrate = drate / htotal;
	   refresh_rate = (unsigned int) (hrate / vtotal * 2 + 0.5);
	} else refresh_rate = 60;

	/* TW: Calculation wrong for 1024x600 - force it to 60Hz */
	if((var->xres == 1024) && (var->yres == 600)) refresh_rate = 60;

	search_idx = 0;
	while( (XGIbios_mode[search_idx].mode_no != 0) &&
	       (XGIbios_mode[search_idx].xres <= var->xres) ) {
		if( (XGIbios_mode[search_idx].xres == var->xres) &&
		    (XGIbios_mode[search_idx].yres == var->yres) &&
		    (XGIbios_mode[search_idx].bpp == var->bits_per_pixel)) {
		        if(XGIfb_validate_mode(search_idx) > 0) {
			   found_mode = 1;
			   break;
			}
		}
		search_idx++;
	}

	if(!found_mode) {
	
		printk(KERN_ERR "XGIfb: %dx%dx%d is no valid mode\n", 
			var->xres, var->yres, var->bits_per_pixel);
			
                search_idx = 0;
		while(XGIbios_mode[search_idx].mode_no != 0) {
		       
		   if( (var->xres <= XGIbios_mode[search_idx].xres) &&
		       (var->yres <= XGIbios_mode[search_idx].yres) && 
		       (var->bits_per_pixel == XGIbios_mode[search_idx].bpp) ) {
		          if(XGIfb_validate_mode(search_idx) > 0) {
			     found_mode = 1;
			     break;
			  }
		   }
		   search_idx++;
	        }			
		if(found_mode) {
			var->xres = XGIbios_mode[search_idx].xres;
		      	var->yres = XGIbios_mode[search_idx].yres;
		      	printk(KERN_DEBUG "XGIfb: Adapted to mode %dx%dx%d\n",
		   		var->xres, var->yres, var->bits_per_pixel);
		   
		} else {
		   	printk(KERN_ERR "XGIfb: Failed to find similar mode to %dx%dx%d\n", 
				var->xres, var->yres, var->bits_per_pixel);
		   	return -EINVAL;
		}
	}

	/* TW: TODO: Check the refresh rate */		
	
	/* Adapt RGB settings */
	XGIfb_bpp_to_var(var);	
	
	/* Sanity check for offsets */
	if (var->xoffset < 0)
		var->xoffset = 0;
	if (var->yoffset < 0)
		var->yoffset = 0;


	if(!XGIfb_ypan) {
		if(var->xres != var->xres_virtual)
			 var->xres_virtual = var->xres;
		if(var->yres != var->yres_virtual)
			var->yres_virtual = var->yres;
	}/* else {
	   // TW: Now patch yres_virtual if we use panning 
	   // May I do this? 
	   var->yres_virtual = xgi_video_info.heapstart / (var->xres * (var->bits_per_pixel >> 3));
	    if(var->yres_virtual <= var->yres) {
	    	// TW: Paranoia check 
	        var->yres_virtual = var->yres;
	    }
	}*/

	/* Truncate offsets to maximum if too high */
	if (var->xoffset > var->xres_virtual - var->xres)
		var->xoffset = var->xres_virtual - var->xres - 1;

	if (var->yoffset > var->yres_virtual - var->yres)
		var->yoffset = var->yres_virtual - var->yres - 1;
	
	/* Set everything else to 0 */
	var->red.msb_right = 
	    var->green.msb_right =
	    var->blue.msb_right =
	    var->transp.offset = var->transp.length = var->transp.msb_right = 0;		
		
	DEBUGPRN("end of check_var");
	return 0;
}
#if LINUX_VERSION_CODE >=  KERNEL_VERSION(2,5,0)
#ifdef XGIFB_PAN
static int XGIfb_pan_display( struct fb_var_screeninfo *var, 
				 struct fb_info* info)
{
	int err;
	
//	printk("\nInside pan_display:");
	
	if (var->xoffset > (var->xres_virtual - var->xres))
		return -EINVAL;
	if (var->yoffset > (var->yres_virtual - var->yres))
		return -EINVAL;

	if (var->vmode & FB_VMODE_YWRAP) {
		if (var->yoffset < 0
		    || var->yoffset >= info->var.yres_virtual
		    || var->xoffset) return -EINVAL;
	} else {
		if (var->xoffset + info->var.xres > info->var.xres_virtual ||
		    var->yoffset + info->var.yres > info->var.yres_virtual)
			return -EINVAL;
	}
    
	if((err = XGIfb_pan_var(var)) < 0) return err;

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;
	if (var->vmode & FB_VMODE_YWRAP)
		info->var.vmode |= FB_VMODE_YWRAP;
	else
		info->var.vmode &= ~FB_VMODE_YWRAP;

//	printk(" End of pan_display");
	return 0;
}
#endif
#endif
static int XGIfb_mmap(struct fb_info *info, struct file *file,
		      struct vm_area_struct *vma)
{
	unsigned long start;
	unsigned long off;
	u32 len, mmio_off;

	DEBUGPRN("inside mmap");
	if(vma->vm_pgoff > (~0UL >> PAGE_SHIFT))  return -EINVAL;

	off = vma->vm_pgoff << PAGE_SHIFT;

	start = (unsigned long) xgi_video_info.video_base;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + xgi_video_info.video_size);
#if 0
	if (off >= len) {
		off -= len;
#endif
	/* By Jake Page: Treat mmap request with offset beyond heapstart
	 *               as request for mapping the mmio area 
	 */
	mmio_off = PAGE_ALIGN((start & ~PAGE_MASK) + xgi_video_info.heapstart);
	if(off >= mmio_off) {
		off -= mmio_off;		
		if(info->var.accel_flags) return -EINVAL;

		start = (unsigned long) xgi_video_info.mmio_base;
		len = PAGE_ALIGN((start & ~PAGE_MASK) + XGIfb_mmio_size);
	}

	start &= PAGE_MASK;
	if((vma->vm_end - vma->vm_start + off) > len)	return -EINVAL;

	off += start;
	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;   /* by Jake Page; is that really needed? */

#if defined(__i386__) || defined(__x86_64__)
	if (boot_cpu_data.x86 > 3)
		pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
#endif
	if (io_remap_page_range(vma, vma->vm_start, off, vma->vm_end - vma->vm_start,
				vma->vm_page_prot))
		return -EAGAIN;

        DEBUGPRN("end of mmap");
	return 0;
}

static int XGIfb_blank(int blank, struct fb_info *info)
{
	u8 reg;

	inXGIIDXREG(XGICR, 0x17, reg);

	if(blank > 0)
		reg &= 0x7f;
	else
		reg |= 0x80;

        outXGIIDXREG(XGICR, 0x17, reg);		
	outXGIIDXREG(XGISR, 0x00, 0x01);    /* Synchronous Reset */
	outXGIIDXREG(XGISR, 0x00, 0x03);    /* End Reset */
        return(0);
}

static int XGIfb_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg, 
		       struct fb_info *info)
{
	DEBUGPRN("inside ioctl");
	switch (cmd) {
	   case FBIO_ALLOC:
		if (!capable(CAP_SYS_RAWIO))
			return -EPERM;
		XGI_malloc((struct XGI_memreq *) arg);
		break;
	   case FBIO_FREE:
		if (!capable(CAP_SYS_RAWIO))
			return -EPERM;
		XGI_free(*(unsigned long *) arg);
		break;
	   case FBIOGET_HWCINFO:
		{
			unsigned long *hwc_offset = (unsigned long *) arg;

			if (XGIfb_caps & HW_CURSOR_CAP)
				*hwc_offset = XGIfb_hwcursor_vbase -
				    (unsigned long) xgi_video_info.video_vbase;
			else
				*hwc_offset = 0;

			break;
		}
	   case FBIOPUT_MODEINFO:
		{
			struct mode_info *x = (struct mode_info *)arg;

			xgi_video_info.video_bpp        = x->bpp;
			xgi_video_info.video_width      = x->xres;
			xgi_video_info.video_height     = x->yres;
			xgi_video_info.video_vwidth     = x->v_xres;
			xgi_video_info.video_vheight    = x->v_yres;
			xgi_video_info.org_x            = x->org_x;
			xgi_video_info.org_y            = x->org_y;
			xgi_video_info.refresh_rate     = x->vrate;
			xgi_video_info.video_linelength = xgi_video_info.video_vwidth * (xgi_video_info.video_bpp >> 3);
			switch(xgi_video_info.video_bpp) {
        		case 8:
            			xgi_video_info.DstColor = 0x0000;
	    			xgi_video_info.XGI310_AccelDepth = 0x00000000;
				xgi_video_info.video_cmap_len = 256;
            			break;
        		case 16:
            			xgi_video_info.DstColor = 0x8000;
            			xgi_video_info.XGI310_AccelDepth = 0x00010000;
				xgi_video_info.video_cmap_len = 16;
            			break;
        		case 32:
            			xgi_video_info.DstColor = 0xC000;
	    			xgi_video_info.XGI310_AccelDepth = 0x00020000;
				xgi_video_info.video_cmap_len = 16;
            			break;
			default:
				xgi_video_info.video_cmap_len = 16;
		       	 	printk(KERN_ERR "XGIfb: Unsupported accel depth %d", xgi_video_info.video_bpp);
				xgi_video_info.accel = 0;
				break;
    			}

			break;
		}
	   case FBIOGET_DISPINFO:
		XGI_dispinfo((struct ap_data *)arg);
		break;
	   case XGIFB_GET_INFO:  /* TW: New for communication with X driver */
	        {
			XGIfb_info *x = (XGIfb_info *)arg;

			//x->XGIfb_id = XGIFB_ID;
			x->XGIfb_version = VER_MAJOR;
			x->XGIfb_revision = VER_MINOR;
			x->XGIfb_patchlevel = VER_LEVEL;
			x->chip_id = xgi_video_info.chip_id;
			x->memory = xgi_video_info.video_size / 1024;
			x->heapstart = xgi_video_info.heapstart / 1024;
			x->fbvidmode = XGIfb_mode_no;
			x->XGIfb_caps = XGIfb_caps;
			x->XGIfb_tqlen = 512; /* yet unused */
			x->XGIfb_pcibus = xgi_video_info.pcibus;
			x->XGIfb_pcislot = xgi_video_info.pcislot;
			x->XGIfb_pcifunc = xgi_video_info.pcifunc;
			x->XGIfb_lcdpdc = XGIfb_detectedpdc;
			x->XGIfb_lcda = XGIfb_detectedlcda;
	                break;
		}
	   case XGIFB_GET_VBRSTATUS:
	        {
			unsigned long *vbrstatus = (unsigned long *) arg;
			if(XGIfb_CheckVBRetrace()) *vbrstatus = 1;
			else		           *vbrstatus = 0;
		}
	   default:
		return -EINVAL;
	}
	DEBUGPRN("end of ioctl");
	return 0;

}

#endif

/* ----------- FBDev related routines for all series ---------- */

static int XGIfb_get_fix(struct fb_fix_screeninfo *fix, int con,
			 struct fb_info *info)
{
	DEBUGPRN("inside get_fix");
	memset(fix, 0, sizeof(struct fb_fix_screeninfo));

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
	strcpy(fix->id, XGI_fb_info.modename);
#else
	strcpy(fix->id, myid);
#endif	

	fix->smem_start = xgi_video_info.video_base;

	fix->smem_len = xgi_video_info.video_size;
	
        /* TW */
/*        if((!XGIfb_mem) || (XGIfb_mem > (xgi_video_info.video_size/1024))) {
	    if (xgi_video_info.video_size > 0x1000000) {
	        fix->smem_len = 0xD00000;
	    } else if (xgi_video_info.video_size > 0x800000)
		fix->smem_len = 0x800000;
	    else
		fix->smem_len = 0x400000;
        } else
		fix->smem_len = XGIfb_mem * 1024;
*/
	fix->type        = video_type;
	fix->type_aux    = 0;
	if(xgi_video_info.video_bpp == 8)
		fix->visual = FB_VISUAL_PSEUDOCOLOR;
	else
		fix->visual = FB_VISUAL_DIRECTCOLOR;
	fix->xpanstep    = 0;
#ifdef XGIFB_PAN
        if(XGIfb_ypan) 	 fix->ypanstep = 1;
#endif
	fix->ywrapstep   = 0;
	fix->line_length = xgi_video_info.video_linelength;
	fix->mmio_start  = xgi_video_info.mmio_base;
	fix->mmio_len    = XGIfb_mmio_size;
    if(xgi_video_info.chip >= XG40)
	   fix->accel    = FB_ACCEL_XGI_XABRE;
	else 
	   fix->accel    = FB_ACCEL_XGI_GLAMOUR_2;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)		
	fix->reserved[0] = xgi_video_info.video_size & 0xFFFF;
	fix->reserved[1] = (xgi_video_info.video_size >> 16) & 0xFFFF;
	fix->reserved[2] = XGIfb_caps;
#endif	

	DEBUGPRN("end of get_fix");
	return 0;
}

/* ----------------  fb_ops structures ----------------- */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
static struct fb_ops XGIfb_ops = {
	owner:		THIS_MODULE,
	fb_get_fix:	XGIfb_get_fix,
	fb_get_var:	XGIfb_get_var,
	fb_set_var:	XGIfb_set_var,
	fb_get_cmap:	XGIfb_get_cmap,
	fb_set_cmap:	XGIfb_set_cmap,
#ifdef XGIFB_PAN
        fb_pan_display:	XGIfb_pan_display,
#endif
	fb_ioctl:	XGIfb_ioctl,
//	fb_mmap:	XGIfb_mmap,
};
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
static struct fb_ops XGIfb_ops = {
	.owner        =	THIS_MODULE,
	.fb_open      = XGIfb_open,
	.fb_release   = XGIfb_release,
	.fb_check_var = XGIfb_check_var,
	.fb_set_par   = XGIfb_set_par,
	.fb_setcolreg = XGIfb_setcolreg,
#ifdef XGIFB_PAN
        .fb_pan_display = XGIfb_pan_display,
#endif	
        .fb_blank     = XGIfb_blank,
	.fb_fillrect  = fbcon_XGI_fillrect,
	.fb_copyarea  = fbcon_XGI_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_cursor    = soft_cursor,	
	.fb_sync      = fbcon_XGI_sync,
	.fb_ioctl     =	XGIfb_ioctl,
//	.fb_mmap      =	XGIfb_mmap,
};
#endif


/* ---------------- Chip generation dependent routines ---------------- */


/* for XGI 315/550/650/740/330 */

static int XGIfb_get_dram_size(void)
{
	
	u8  ChannelNum,tmp;
	u8  reg = 0;


	        inXGIIDXREG(XGISR, IND_XGI_DRAM_SIZE, reg);
		switch ((reg & XGI_DRAM_SIZE_MASK) >> 4) {
		   case XGI_DRAM_SIZE_1MB:
			xgi_video_info.video_size = 0x100000;
			break;
		   case XGI_DRAM_SIZE_2MB:
			xgi_video_info.video_size = 0x200000;
			break;
		   case XGI_DRAM_SIZE_4MB:
			xgi_video_info.video_size = 0x400000;
			break;
		   case XGI_DRAM_SIZE_8MB:
			xgi_video_info.video_size = 0x800000;
			break;
		   case XGI_DRAM_SIZE_16MB:
			xgi_video_info.video_size = 0x1000000;
			break;
		   case XGI_DRAM_SIZE_32MB:
			xgi_video_info.video_size = 0x2000000;
			break;
		   case XGI_DRAM_SIZE_64MB:
			xgi_video_info.video_size = 0x4000000;
			break;
		   case XGI_DRAM_SIZE_128MB:
			xgi_video_info.video_size = 0x8000000;
			break;
		   case XGI_DRAM_SIZE_256MB:
			xgi_video_info.video_size = 0x10000000;
			break;
		   default:
			return -1;
		}
		
		tmp = (reg & 0x0c) >> 2;
		switch(xgi_video_info.chip)
		{
		    case XG20:
		        ChannelNum = 1;
		        break;
		        
		    case XG42:
		        if(reg & 0x04)
		            ChannelNum = 2;
		        else
		            ChannelNum = 1;
		        break;
		        
		    case XG45:
		        if(tmp == 1)
                    ChannelNum = 2;		    
                else 
                if(tmp == 2)
                    ChannelNum = 3;
                else
                if(tmp == 3)
                    ChannelNum = 4;
                else
                    ChannelNum = 1;
		        break;

		    case XG40:
		    default:
                if(tmp == 2)
                    ChannelNum = 2;		    
                else 
                if(tmp == 3)
                    ChannelNum = 3;
                else
                    ChannelNum = 1;
		        break;
		}
		
		
		xgi_video_info.video_size = xgi_video_info.video_size * ChannelNum;

		return 0;

}

static void XGIfb_detect_VB(void)
{
	u8 cr32, temp=0;

	xgi_video_info.TV_plug = xgi_video_info.TV_type = 0;

        switch(xgi_video_info.hasVB) {
	  case HASVB_LVDS_CHRONTEL:
	  case HASVB_CHRONTEL:
	     break;
	  case HASVB_301:
	  case HASVB_302:
	   //  XGI_Sense30x();
	     break;
	}

	inXGIIDXREG(XGICR, IND_XGI_SCRATCH_REG_CR32, cr32);

	if ((cr32 & XGI_CRT1) && !XGIfb_crt1off)
		XGIfb_crt1off = 0;
	else {
		if (cr32 & 0x5F)   
			XGIfb_crt1off = 1;
		else
			XGIfb_crt1off = 0;
	}

	if (XGIfb_crt2type != -1)
		/* TW: Override with option */
		xgi_video_info.disp_state = XGIfb_crt2type;
	else if (cr32 & XGI_VB_TV)
		xgi_video_info.disp_state = DISPTYPE_TV;		
	else if (cr32 & XGI_VB_LCD)
		xgi_video_info.disp_state = DISPTYPE_LCD;		
	else if (cr32 & XGI_VB_CRT2)
		xgi_video_info.disp_state = DISPTYPE_CRT2;
	else
		xgi_video_info.disp_state = 0;

	if(XGIfb_tvplug != -1)
		/* PR/TW: Override with option */
	        xgi_video_info.TV_plug = XGIfb_tvplug;
	else if (cr32 & XGI_VB_HIVISION) {
		xgi_video_info.TV_type = TVMODE_HIVISION;
		xgi_video_info.TV_plug = TVPLUG_SVIDEO;
	}
	else if (cr32 & XGI_VB_SVIDEO)
		xgi_video_info.TV_plug = TVPLUG_SVIDEO;
	else if (cr32 & XGI_VB_COMPOSITE)
		xgi_video_info.TV_plug = TVPLUG_COMPOSITE;
	else if (cr32 & XGI_VB_SCART)
		xgi_video_info.TV_plug = TVPLUG_SCART;

	if(xgi_video_info.TV_type == 0) {
	    /* TW: PAL/NTSC changed for 650 */
	    if((xgi_video_info.chip <= XGI_315PRO) || (xgi_video_info.chip >= XGI_330)) {

                inXGIIDXREG(XGICR, 0x38, temp);
		if(temp & 0x10)
			xgi_video_info.TV_type = TVMODE_PAL;
		else
			xgi_video_info.TV_type = TVMODE_NTSC;

	    } else {

	        inXGIIDXREG(XGICR, 0x79, temp);
		if(temp & 0x20)
			xgi_video_info.TV_type = TVMODE_PAL;
		else
			xgi_video_info.TV_type = TVMODE_NTSC;
	    }
	}

	/* TW: Copy forceCRT1 option to CRT1off if option is given */
    	if (XGIfb_forcecrt1 != -1) {
    		if (XGIfb_forcecrt1) XGIfb_crt1off = 0;
		else   	             XGIfb_crt1off = 1;
    	}
}

static void XGIfb_get_VB_type(void)
{
	u8 reg;

	if (!XGIfb_has_VB()) {
	        inXGIIDXREG(XGICR, IND_XGI_SCRATCH_REG_CR37, reg);
		switch ((reg & XGI_EXTERNAL_CHIP_MASK) >> 1) {
	 	   case XGI310_EXTERNAL_CHIP_LVDS:
			xgi_video_info.hasVB = HASVB_LVDS;
			break;
		   case XGI310_EXTERNAL_CHIP_LVDS_CHRONTEL:
			xgi_video_info.hasVB = HASVB_LVDS_CHRONTEL;
			break;
		   default:
			break;
		}
	}
}


static int XGIfb_has_VB(void)
{
	u8 vb_chipid;

	inXGIIDXREG(XGIPART4, 0x00, vb_chipid);
	switch (vb_chipid) {
	   case 0x01:
		xgi_video_info.hasVB = HASVB_301;
		break;
	   case 0x02:
		xgi_video_info.hasVB = HASVB_302;
		break;
	   default:
		xgi_video_info.hasVB = HASVB_NONE;
		return FALSE;
	}
	return TRUE;
}



/* ------------------ Sensing routines ------------------ */

/* TW: Determine and detect attached devices on XGI30x */
int
XGIDoSense(int tempbl, int tempbh, int tempcl, int tempch)
{
    int temp,i;

    outXGIIDXREG(XGIPART4,0x11,tempbl);
    temp = tempbh | tempcl;
    setXGIIDXREG(XGIPART4,0x10,0xe0,temp);
    for(i=0; i<10; i++) XGI_LongWait(&XGI_Pr);
    tempch &= 0x7f;
    inXGIIDXREG(XGIPART4,0x03,temp);
    temp ^= 0x0e;
    temp &= tempch;
    return(temp);
}

void
XGI_Sense30x(void)
{
  u8 backupP4_0d;
  u8 testsvhs_tempbl, testsvhs_tempbh;
  u8 testsvhs_tempcl, testsvhs_tempch;
  u8 testcvbs_tempbl, testcvbs_tempbh;
  u8 testcvbs_tempcl, testcvbs_tempch;
  u8 testvga2_tempbl, testvga2_tempbh;
  u8 testvga2_tempcl, testvga2_tempch;
  int myflag, result;

  inXGIIDXREG(XGIPART4,0x0d,backupP4_0d);
  outXGIIDXREG(XGIPART4,0x0d,(backupP4_0d | 0x04));



	testvga2_tempbh = 0x00; testvga2_tempbl = 0xd1;
        testsvhs_tempbh = 0x00; testsvhs_tempbl = 0xb9;
	testcvbs_tempbh = 0x00; testcvbs_tempbl = 0xb3;
	if((XGIhw_ext.ujVBChipID != VB_CHIP_301) &&
	   (XGIhw_ext.ujVBChipID != VB_CHIP_302)) {
	      testvga2_tempbh = 0x01; testvga2_tempbl = 0x90;
	      testsvhs_tempbh = 0x01; testsvhs_tempbl = 0x6b;
	      testcvbs_tempbh = 0x01; testcvbs_tempbl = 0x74;
	      if(XGIhw_ext.ujVBChipID == VB_CHIP_301LV ||
	         XGIhw_ext.ujVBChipID == VB_CHIP_302LV) {
	         testvga2_tempbh = 0x00; testvga2_tempbl = 0x00;
	         testsvhs_tempbh = 0x02; testsvhs_tempbl = 0x00;
	         testcvbs_tempbh = 0x01; testcvbs_tempbl = 0x00;
	      }
	}
	if(XGIhw_ext.ujVBChipID != VB_CHIP_301LV &&
	   XGIhw_ext.ujVBChipID != VB_CHIP_302LV) {
	   inXGIIDXREG(XGIPART4,0x01,myflag);
	   if(myflag & 0x04) {
	      testvga2_tempbh = 0x00; testvga2_tempbl = 0xfd;
	      testsvhs_tempbh = 0x00; testsvhs_tempbl = 0xdd;
	      testcvbs_tempbh = 0x00; testcvbs_tempbl = 0xee;
	   }
	}
	if((XGIhw_ext.ujVBChipID == VB_CHIP_301LV) ||
	   (XGIhw_ext.ujVBChipID == VB_CHIP_302LV) ) {
	   testvga2_tempbh = 0x00; testvga2_tempbl = 0x00;
	   testvga2_tempch = 0x00; testvga2_tempcl = 0x00;
	   testsvhs_tempch = 0x04; testsvhs_tempcl = 0x08;
	   testcvbs_tempch = 0x08; testcvbs_tempcl = 0x08;
	} else {
	   testvga2_tempch = 0x0e; testvga2_tempcl = 0x08;
	   testsvhs_tempch = 0x06; testsvhs_tempcl = 0x04;
	   testcvbs_tempch = 0x08; testcvbs_tempcl = 0x04;
	}


    if(testvga2_tempch || testvga2_tempcl || testvga2_tempbh || testvga2_tempbl) {
        result = XGIDoSense(testvga2_tempbl, testvga2_tempbh,
                            testvga2_tempcl, testvga2_tempch);
 	if(result) {
        	printk(KERN_INFO "XGIfb: Detected secondary VGA connection\n");
		orXGIIDXREG(XGICR, 0x32, 0x10);
	}
    }
    
    result = XGIDoSense(testsvhs_tempbl, testsvhs_tempbh,
                        testsvhs_tempcl, testsvhs_tempch);
    if(result) {
        printk(KERN_INFO "XGIfb: Detected TV connected to SVHS output\n");
        /* TW: So we can be sure that there IS a SVHS output */
	xgi_video_info.TV_plug = TVPLUG_SVIDEO;
	orXGIIDXREG(XGICR, 0x32, 0x02);
    }

    if(!result) {
        result = XGIDoSense(testcvbs_tempbl, testcvbs_tempbh,
	                    testcvbs_tempcl, testcvbs_tempch);
	if(result) {
	    printk(KERN_INFO "XGIfb: Detected TV connected to CVBS output\n");
	    /* TW: So we can be sure that there IS a CVBS output */
	    xgi_video_info.TV_plug = TVPLUG_COMPOSITE;
	    orXGIIDXREG(XGICR, 0x32, 0x01);
	}
    }
    XGIDoSense(0, 0, 0, 0);

    outXGIIDXREG(XGIPART4,0x0d,backupP4_0d);
}



/* ------------------------ Heap routines -------------------------- */

static int XGIfb_heap_init(void)
{
	XGI_OH *poh;
	u8 temp=0;

	int            agp_enabled = 1;
	u32            agp_size;
	unsigned long *cmdq_baseport = 0;
	unsigned long *read_port = 0;
	unsigned long *write_port = 0;
	XGI_CMDTYPE    cmd_type;
#ifndef AGPOFF
	struct agp_kern_info  *agp_info;
	struct agp_memory     *agp;
	u32            agp_phys;
#endif

/* TW: The heap start is either set manually using the "mem" parameter, or
 *     defaults as follows:
 *     -) If more than 16MB videoRAM available, let our heap start at 12MB.
 *     -) If more than  8MB videoRAM available, let our heap start at  8MB.
 *     -) If 4MB or less is available, let it start at 4MB.
 *     This is for avoiding a clash with X driver which uses the beginning
 *     of the videoRAM. To limit size of X framebuffer, use Option MaxXFBMem
 *     in XF86Config-4.
 *     The heap start can also be specified by parameter "mem" when starting the XGIfb
 *     driver. XGIfb mem=1024 lets heap starts at 1MB, etc.
 */
     if ((!XGIfb_mem) || (XGIfb_mem > (xgi_video_info.video_size/1024))) {
        if (xgi_video_info.video_size > 0x1000000) {
	        xgi_video_info.heapstart = 0xD00000;
	} else if (xgi_video_info.video_size > 0x800000) {
	        xgi_video_info.heapstart = 0x800000;
	} else {
		xgi_video_info.heapstart = 0x400000;
	}
     } else {
           xgi_video_info.heapstart = XGIfb_mem * 1024;
     }
     XGIfb_heap_start =
	       (unsigned long) (xgi_video_info.video_vbase + xgi_video_info.heapstart);
     printk(KERN_INFO "XGIfb: Memory heap starting at %dK\n",
     					(int)(xgi_video_info.heapstart / 1024));

     XGIfb_heap_end = (unsigned long) xgi_video_info.video_vbase + xgi_video_info.video_size;
     XGIfb_heap_size = XGIfb_heap_end - XGIfb_heap_start;


 
        /* TW: Now initialize the 310 series' command queue mode.
	 * On 310/325, there are three queue modes available which
	 * are chosen by setting bits 7:5 in SR26:
	 * 1. MMIO queue mode (bit 5, 0x20). The hardware will keep
	 *    track of the queue, the FIFO, command parsing and so
	 *    on. This is the one comparable to the 300 series.
	 * 2. VRAM queue mode (bit 6, 0x40). In this case, one will
	 *    have to do queue management himself. Register 0x85c4 will
	 *    hold the location of the next free queue slot, 0x85c8
	 *    is the "queue read pointer" whose way of working is
	 *    unknown to me. Anyway, this mode would require a
	 *    translation of the MMIO commands to some kind of
	 *    accelerator assembly and writing these commands
	 *    to the memory location pointed to by 0x85c4.
	 *    We will not use this, as nobody knows how this
	 *    "assembly" works, and as it would require a complete
	 *    re-write of the accelerator code.
	 * 3. AGP queue mode (bit 7, 0x80). Works as 2., but keeps the
	 *    queue in AGP memory space.
	 *
	 * SR26 bit 4 is called "Bypass H/W queue".
	 * SR26 bit 1 is called "Enable Command Queue Auto Correction"
	 * SR26 bit 0 resets the queue
	 * Size of queue memory is encoded in bits 3:2 like this:
	 *    00  (0x00)  512K
	 *    01  (0x04)  1M
	 *    10  (0x08)  2M
	 *    11  (0x0C)  4M
	 * The queue location is to be written to 0x85C0.
	 *
         */
	cmdq_baseport = (unsigned long *)(xgi_video_info.mmio_vbase + MMIO_QUEUE_PHYBASE);
	write_port    = (unsigned long *)(xgi_video_info.mmio_vbase + MMIO_QUEUE_WRITEPORT);
	read_port     = (unsigned long *)(xgi_video_info.mmio_vbase + MMIO_QUEUE_READPORT);

	DPRINTK("AGP base: 0x%p, read: 0x%p, write: 0x%p\n", cmdq_baseport, read_port, write_port);

	agp_size  = COMMAND_QUEUE_AREA_SIZE;

#ifndef AGPOFF
	if (XGIfb_queuemode == AGP_CMD_QUEUE) {
		agp_info = vmalloc(sizeof(*agp_info));
		memset((void*)agp_info, 0x00, sizeof(*agp_info));
		agp_copy_info(agp_info);

		agp_backend_acquire();

		agp = agp_allocate_memory(COMMAND_QUEUE_AREA_SIZE/PAGE_SIZE,
					  AGP_NORMAL_MEMORY);
		if (agp == NULL) {
			DPRINTK("XGIfb: Allocating AGP buffer failed.\n");
			agp_enabled = 0;
		} else {
			if (agp_bind_memory(agp, agp->pg_start) != 0) {
				DPRINTK("XGIfb: AGP: Failed to bind memory\n");
				/* TODO: Free AGP memory here */
				agp_enabled = 0;
			} else {
				agp_enable(0);
			}
		}
	}
#else
	agp_enabled = 0;
#endif

	/* TW: Now select the queue mode */

	if ((agp_enabled) && (XGIfb_queuemode == AGP_CMD_QUEUE)) {
		cmd_type = AGP_CMD_QUEUE;
		printk(KERN_INFO "XGIfb: Using AGP queue mode\n");
/*	} else if (XGIfb_heap_size >= COMMAND_QUEUE_AREA_SIZE)  */
        } else if (XGIfb_queuemode == VM_CMD_QUEUE) {
		cmd_type = VM_CMD_QUEUE;
		printk(KERN_INFO "XGIfb: Using VRAM queue mode\n");
	} else {
		printk(KERN_INFO "XGIfb: Using MMIO queue mode\n");
		cmd_type = MMIO_CMD;
	}

	switch (agp_size) {
	   case 0x80000:
		temp = XGI_CMD_QUEUE_SIZE_512k;
		break;
	   case 0x100000:
		temp = XGI_CMD_QUEUE_SIZE_1M;
		break;
	   case 0x200000:
		temp = XGI_CMD_QUEUE_SIZE_2M;
		break;
	   case 0x400000:
		temp = XGI_CMD_QUEUE_SIZE_4M;
		break;
	}

	switch (cmd_type) {
	   case AGP_CMD_QUEUE:
#ifndef AGPOFF
		DPRINTK("XGIfb: AGP buffer base = 0x%lx, offset = 0x%x, size = %dK\n",
			agp_info->aper_base, agp->physical, agp_size/1024);

		agp_phys = agp_info->aper_base + agp->physical;

		outXGIIDXREG(XGICR,  IND_XGI_AGP_IO_PAD, 0);
		outXGIIDXREG(XGICR,  IND_XGI_AGP_IO_PAD, XGI_AGP_2X);

                outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_THRESHOLD, COMMAND_QUEUE_THRESHOLD);

		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_SET, XGI_CMD_QUEUE_RESET);

		*write_port = *read_port;

		temp |= XGI_AGP_CMDQUEUE_ENABLE;
		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_SET, temp);

		*cmdq_baseport = agp_phys;

		XGIfb_caps |= AGP_CMD_QUEUE_CAP;
#endif
		break;

	   case VM_CMD_QUEUE:
		XGIfb_heap_end -= COMMAND_QUEUE_AREA_SIZE;
		XGIfb_heap_size -= COMMAND_QUEUE_AREA_SIZE;

		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_THRESHOLD, COMMAND_QUEUE_THRESHOLD);

		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_SET, XGI_CMD_QUEUE_RESET);

		*write_port = *read_port;

		temp |= XGI_VRAM_CMDQUEUE_ENABLE;
		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_SET, temp);

		*cmdq_baseport = xgi_video_info.video_size - COMMAND_QUEUE_AREA_SIZE;

		XGIfb_caps |= VM_CMD_QUEUE_CAP;

		DPRINTK("XGIfb: VM Cmd Queue offset = 0x%lx, size is %dK\n",
			*cmdq_baseport, COMMAND_QUEUE_AREA_SIZE/1024);
		break;

	   default:  /* MMIO */
	   	/* TW: This previously only wrote XGI_MMIO_CMD_ENABLE
		 * to IND_XGI_CMDQUEUE_SET. I doubt that this is
		 * enough. Reserve memory in any way.
		 */
	   	XGIfb_heap_end -= COMMAND_QUEUE_AREA_SIZE;
		XGIfb_heap_size -= COMMAND_QUEUE_AREA_SIZE;

		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_THRESHOLD, COMMAND_QUEUE_THRESHOLD);
		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_SET, XGI_CMD_QUEUE_RESET);

		*write_port = *read_port;

		/* TW: Set Auto_Correction bit */
		temp |= (XGI_MMIO_CMD_ENABLE | XGI_CMD_AUTO_CORR);
		outXGIIDXREG(XGISR, IND_XGI_CMDQUEUE_SET, temp);

		*cmdq_baseport = xgi_video_info.video_size - COMMAND_QUEUE_AREA_SIZE;

		XGIfb_caps |= MMIO_CMD_QUEUE_CAP;

		DPRINTK("XGIfb: MMIO Cmd Queue offset = 0x%lx, size is %dK\n",
			*cmdq_baseport, COMMAND_QUEUE_AREA_SIZE/1024);
		break;
	}




     /* TW: Now reserve memory for the HWCursor. It is always located at the very
            top of the videoRAM, right below the TB memory area (if used). */
     if (XGIfb_heap_size >= XGIfb_hwcursor_size) {
		XGIfb_heap_end -= XGIfb_hwcursor_size;
		XGIfb_heap_size -= XGIfb_hwcursor_size;
		XGIfb_hwcursor_vbase = XGIfb_heap_end;

		XGIfb_caps |= HW_CURSOR_CAP;

		DPRINTK("XGIfb: Hardware Cursor start at 0x%lx, size is %dK\n",
			XGIfb_heap_end, XGIfb_hwcursor_size/1024);
     }

     XGIfb_heap.poha_chain = NULL;
     XGIfb_heap.poh_freelist = NULL;

     poh = XGIfb_poh_new_node();

     if(poh == NULL)  return 1;
	
     poh->poh_next = &XGIfb_heap.oh_free;
     poh->poh_prev = &XGIfb_heap.oh_free;
     poh->size = XGIfb_heap_end - XGIfb_heap_start + 1;
     poh->offset = XGIfb_heap_start - (unsigned long) xgi_video_info.video_vbase;

     DPRINTK("XGIfb: Heap start:0x%p, end:0x%p, len=%dk\n",
		(char *) XGIfb_heap_start, (char *) XGIfb_heap_end,
		(unsigned int) poh->size / 1024);

     DPRINTK("XGIfb: First Node offset:0x%x, size:%dk\n",
		(unsigned int) poh->offset, (unsigned int) poh->size / 1024);

     XGIfb_heap.oh_free.poh_next = poh;
     XGIfb_heap.oh_free.poh_prev = poh;
     XGIfb_heap.oh_free.size = 0;
     XGIfb_heap.max_freesize = poh->size;

     XGIfb_heap.oh_used.poh_next = &XGIfb_heap.oh_used;
     XGIfb_heap.oh_used.poh_prev = &XGIfb_heap.oh_used;
     XGIfb_heap.oh_used.size = SENTINEL;

     return 0;
}

static XGI_OH *XGIfb_poh_new_node(void)
{
	int           i;
	unsigned long cOhs;
	XGI_OHALLOC   *poha;
	XGI_OH        *poh;

	if (XGIfb_heap.poh_freelist == NULL) {
		poha = kmalloc(OH_ALLOC_SIZE, GFP_KERNEL);
		if(!poha) return NULL;

		poha->poha_next = XGIfb_heap.poha_chain;
		XGIfb_heap.poha_chain = poha;

		cOhs = (OH_ALLOC_SIZE - sizeof(XGI_OHALLOC)) / sizeof(XGI_OH) + 1;

		poh = &poha->aoh[0];
		for (i = cOhs - 1; i != 0; i--) {
			poh->poh_next = poh + 1;
			poh = poh + 1;
		}

		poh->poh_next = NULL;
		XGIfb_heap.poh_freelist = &poha->aoh[0];
	}

	poh = XGIfb_heap.poh_freelist;
	XGIfb_heap.poh_freelist = poh->poh_next;

	return (poh);
}

static XGI_OH *XGIfb_poh_allocate(unsigned long size)
{
	XGI_OH *pohThis;
	XGI_OH *pohRoot;
	int     bAllocated = 0;

	if (size > XGIfb_heap.max_freesize) {
		DPRINTK("XGIfb: Can't allocate %dk size on offscreen\n",
			(unsigned int) size / 1024);
		return (NULL);
	}

	pohThis = XGIfb_heap.oh_free.poh_next;

	while (pohThis != &XGIfb_heap.oh_free) {
		if (size <= pohThis->size) {
			bAllocated = 1;
			break;
		}
		pohThis = pohThis->poh_next;
	}

	if (!bAllocated) {
		DPRINTK("XGIfb: Can't allocate %dk size on offscreen\n",
			(unsigned int) size / 1024);
		return (NULL);
	}

	if (size == pohThis->size) {
		pohRoot = pohThis;
		XGIfb_delete_node(pohThis);
	} else {
		pohRoot = XGIfb_poh_new_node();

		if (pohRoot == NULL) {
			return (NULL);
		}

		pohRoot->offset = pohThis->offset;
		pohRoot->size = size;

		pohThis->offset += size;
		pohThis->size -= size;
	}

	XGIfb_heap.max_freesize -= size;

	pohThis = &XGIfb_heap.oh_used;
	XGIfb_insert_node(pohThis, pohRoot);

	return (pohRoot);
}

static void XGIfb_delete_node(XGI_OH *poh)
{
	XGI_OH *poh_prev;
	XGI_OH *poh_next;

	poh_prev = poh->poh_prev;
	poh_next = poh->poh_next;

	poh_prev->poh_next = poh_next;
	poh_next->poh_prev = poh_prev;

}

static void XGIfb_insert_node(XGI_OH *pohList, XGI_OH *poh)
{
	XGI_OH *pohTemp;

	pohTemp = pohList->poh_next;

	pohList->poh_next = poh;
	pohTemp->poh_prev = poh;

	poh->poh_prev = pohList;
	poh->poh_next = pohTemp;
}

static XGI_OH *XGIfb_poh_free(unsigned long base)
{
	XGI_OH *pohThis;
	XGI_OH *poh_freed;
	XGI_OH *poh_prev;
	XGI_OH *poh_next;
	unsigned long ulUpper;
	unsigned long ulLower;
	int foundNode = 0;

	poh_freed = XGIfb_heap.oh_used.poh_next;

	while(poh_freed != &XGIfb_heap.oh_used) {
		if(poh_freed->offset == base) {
			foundNode = 1;
			break;
		}

		poh_freed = poh_freed->poh_next;
	}

	if (!foundNode)  return (NULL);

	XGIfb_heap.max_freesize += poh_freed->size;

	poh_prev = poh_next = NULL;
	ulUpper = poh_freed->offset + poh_freed->size;
	ulLower = poh_freed->offset;

	pohThis = XGIfb_heap.oh_free.poh_next;

	while (pohThis != &XGIfb_heap.oh_free) {
		if (pohThis->offset == ulUpper) {
			poh_next = pohThis;
		}
			else if ((pohThis->offset + pohThis->size) ==
				 ulLower) {
			poh_prev = pohThis;
		}
		pohThis = pohThis->poh_next;
	}

	XGIfb_delete_node(poh_freed);

	if (poh_prev && poh_next) {
		poh_prev->size += (poh_freed->size + poh_next->size);
		XGIfb_delete_node(poh_next);
		XGIfb_free_node(poh_freed);
		XGIfb_free_node(poh_next);
		return (poh_prev);
	}

	if (poh_prev) {
		poh_prev->size += poh_freed->size;
		XGIfb_free_node(poh_freed);
		return (poh_prev);
	}

	if (poh_next) {
		poh_next->size += poh_freed->size;
		poh_next->offset = poh_freed->offset;
		XGIfb_free_node(poh_freed);
		return (poh_next);
	}

	XGIfb_insert_node(&XGIfb_heap.oh_free, poh_freed);

	return (poh_freed);
}

static void XGIfb_free_node(XGI_OH *poh)
{
	if(poh == NULL) return;

	poh->poh_next = XGIfb_heap.poh_freelist;
	XGIfb_heap.poh_freelist = poh;

}

void XGI_malloc(struct XGI_memreq *req)
{
	XGI_OH *poh;

	poh = XGIfb_poh_allocate(req->size);

	if(poh == NULL) {
		req->offset = 0;
		req->size = 0;
		DPRINTK("XGIfb: Video RAM allocation failed\n");
	} else {
		DPRINTK("XGIfb: Video RAM allocation succeeded: 0x%p\n",
			(char *) (poh->offset + (unsigned long) xgi_video_info.video_vbase));

		req->offset = poh->offset;
		req->size = poh->size;
	}

}

void XGI_free(unsigned long base)
{
	XGI_OH *poh;

	poh = XGIfb_poh_free(base);

	if(poh == NULL) {
		DPRINTK("XGIfb: XGIfb_poh_free() failed at base 0x%x\n",
			(unsigned int) base);
	}
}

/* --------------------- SetMode routines ------------------------- */

static void XGIfb_pre_setmode(void)
{
	u8 cr30 = 0, cr31 = 0;

	inXGIIDXREG(XGICR, 0x31, cr31);
	cr31 &= ~0x60;

	switch (xgi_video_info.disp_state & DISPTYPE_DISP2) {
	   case DISPTYPE_CRT2:
		cr30 = (XGI_VB_OUTPUT_CRT2 | XGI_SIMULTANEOUS_VIEW_ENABLE);
		cr31 |= XGI_DRIVER_MODE;
		break;
	   case DISPTYPE_LCD:
		cr30  = (XGI_VB_OUTPUT_LCD | XGI_SIMULTANEOUS_VIEW_ENABLE);
		cr31 |= XGI_DRIVER_MODE;
		break;
	   case DISPTYPE_TV:
		if (xgi_video_info.TV_type == TVMODE_HIVISION)
			cr30 = (XGI_VB_OUTPUT_HIVISION | XGI_SIMULTANEOUS_VIEW_ENABLE);
		else if (xgi_video_info.TV_plug == TVPLUG_SVIDEO)
			cr30 = (XGI_VB_OUTPUT_SVIDEO | XGI_SIMULTANEOUS_VIEW_ENABLE);
		else if (xgi_video_info.TV_plug == TVPLUG_COMPOSITE)
			cr30 = (XGI_VB_OUTPUT_COMPOSITE | XGI_SIMULTANEOUS_VIEW_ENABLE);
		else if (xgi_video_info.TV_plug == TVPLUG_SCART)
			cr30 = (XGI_VB_OUTPUT_SCART | XGI_SIMULTANEOUS_VIEW_ENABLE);
		cr31 |= XGI_DRIVER_MODE;

	        if (XGIfb_tvmode == 1 || xgi_video_info.TV_type == TVMODE_PAL)
			cr31 |= 0x01;
                else
                        cr31 &= ~0x01;
		break;
	   default:	/* disable CRT2 */
		cr30 = 0x00;
		cr31 |= (XGI_DRIVER_MODE | XGI_VB_OUTPUT_DISABLE);
	}

	outXGIIDXREG(XGICR, IND_XGI_SCRATCH_REG_CR30, cr30);
	outXGIIDXREG(XGICR, IND_XGI_SCRATCH_REG_CR31, cr31);

        outXGIIDXREG(XGICR, IND_XGI_SCRATCH_REG_CR33, (XGIfb_rate_idx & 0x0F));

	if(xgi_video_info.accel) XGIfb_syncaccel();

	
}

static void XGIfb_post_setmode(void)
{
	u8 reg;
	BOOLEAN doit = TRUE;
#if 0	/* TW: Wrong: Is not in MMIO space, but in RAM */
	/* Backup mode number to MMIO space */
	if(xgi_video_info.mmio_vbase) {
	  *(volatile u8 *)(((u8*)xgi_video_info.mmio_vbase) + 0x449) = (unsigned char)XGIfb_mode_no;
	}
#endif	
/*	outXGIIDXREG(XGISR,IND_XGI_PASSWORD,XGI_PASSWORD);
	outXGIIDXREG(XGICR,0x13,0x00);
	setXGIIDXREG(XGISR,0x0E,0xF0,0x01);
*test**/
	if (xgi_video_info.video_bpp == 8) {
		/* TW: We can't switch off CRT1 on LVDS/Chrontel in 8bpp Modes */
		if ((xgi_video_info.hasVB == HASVB_LVDS) || (xgi_video_info.hasVB == HASVB_LVDS_CHRONTEL)) {
			doit = FALSE;
		}
		/* TW: We can't switch off CRT1 on 301B-DH in 8bpp Modes if using LCD */
		if  (xgi_video_info.disp_state & DISPTYPE_LCD)  {
	        	doit = FALSE;
	        }
	}

	/* TW: We can't switch off CRT1 if bridge is in slave mode */
	if(xgi_video_info.hasVB != HASVB_NONE) {
		inXGIIDXREG(XGIPART1, 0x00, reg);


		if((reg & 0x50) == 0x10) {
			doit = FALSE;
		}

	} else XGIfb_crt1off = 0;

	inXGIIDXREG(XGICR, 0x17, reg);
	if((XGIfb_crt1off) && (doit))
		reg &= ~0x80;
	else 	      
		reg |= 0x80;
	outXGIIDXREG(XGICR, 0x17, reg);

        andXGIIDXREG(XGISR, IND_XGI_RAMDAC_CONTROL, ~0x04);

	if((xgi_video_info.disp_state & DISPTYPE_TV) && (xgi_video_info.hasVB == HASVB_301)) {

	   inXGIIDXREG(XGIPART4, 0x01, reg);

	   if(reg < 0xB0) {        	/* Set filter for XGI301 */

		switch (xgi_video_info.video_width) {
		   case 320:
			filter_tb = (xgi_video_info.TV_type == TVMODE_NTSC) ? 4 : 12;
			break;
		   case 640:
			filter_tb = (xgi_video_info.TV_type == TVMODE_NTSC) ? 5 : 13;
			break;
		   case 720:
			filter_tb = (xgi_video_info.TV_type == TVMODE_NTSC) ? 6 : 14;
			break;
		   case 800:
			filter_tb = (xgi_video_info.TV_type == TVMODE_NTSC) ? 7 : 15;
			break;
		   default:
			filter = -1;
			break;
		}

		orXGIIDXREG(XGIPART1, XGIfb_CRT2_write_enable, 0x01);

		if(xgi_video_info.TV_type == TVMODE_NTSC) {

		        andXGIIDXREG(XGIPART2, 0x3a, 0x1f);

			if (xgi_video_info.TV_plug == TVPLUG_SVIDEO) {

			        andXGIIDXREG(XGIPART2, 0x30, 0xdf);

			} else if (xgi_video_info.TV_plug == TVPLUG_COMPOSITE) {

			        orXGIIDXREG(XGIPART2, 0x30, 0x20);

				switch (xgi_video_info.video_width) {
				case 640:
				        outXGIIDXREG(XGIPART2, 0x35, 0xEB);
					outXGIIDXREG(XGIPART2, 0x36, 0x04);
					outXGIIDXREG(XGIPART2, 0x37, 0x25);
					outXGIIDXREG(XGIPART2, 0x38, 0x18);
					break;
				case 720:
					outXGIIDXREG(XGIPART2, 0x35, 0xEE);
					outXGIIDXREG(XGIPART2, 0x36, 0x0C);
					outXGIIDXREG(XGIPART2, 0x37, 0x22);
					outXGIIDXREG(XGIPART2, 0x38, 0x08);
					break;
				case 800:
					outXGIIDXREG(XGIPART2, 0x35, 0xEB);
					outXGIIDXREG(XGIPART2, 0x36, 0x15);
					outXGIIDXREG(XGIPART2, 0x37, 0x25);
					outXGIIDXREG(XGIPART2, 0x38, 0xF6);
					break;
				}
			}

		} else if(xgi_video_info.TV_type == TVMODE_PAL) {

			andXGIIDXREG(XGIPART2, 0x3A, 0x1F);

			if (xgi_video_info.TV_plug == TVPLUG_SVIDEO) {

			        andXGIIDXREG(XGIPART2, 0x30, 0xDF);

			} else if (xgi_video_info.TV_plug == TVPLUG_COMPOSITE) {

			        orXGIIDXREG(XGIPART2, 0x30, 0x20);

				switch (xgi_video_info.video_width) {
				case 640:
					outXGIIDXREG(XGIPART2, 0x35, 0xF1);
					outXGIIDXREG(XGIPART2, 0x36, 0xF7);
					outXGIIDXREG(XGIPART2, 0x37, 0x1F);
					outXGIIDXREG(XGIPART2, 0x38, 0x32);
					break;
				case 720:
					outXGIIDXREG(XGIPART2, 0x35, 0xF3);
					outXGIIDXREG(XGIPART2, 0x36, 0x00);
					outXGIIDXREG(XGIPART2, 0x37, 0x1D);
					outXGIIDXREG(XGIPART2, 0x38, 0x20);
					break;
				case 800:
					outXGIIDXREG(XGIPART2, 0x35, 0xFC);
					outXGIIDXREG(XGIPART2, 0x36, 0xFB);
					outXGIIDXREG(XGIPART2, 0x37, 0x14);
					outXGIIDXREG(XGIPART2, 0x38, 0x2A);
					break;
				}
			}
		}

		if ((filter >= 0) && (filter <=7)) {
			DPRINTK("FilterTable[%d]-%d: %02x %02x %02x %02x\n", filter_tb, filter, 
				XGI_TV_filter[filter_tb].filter[filter][0],
				XGI_TV_filter[filter_tb].filter[filter][1],
				XGI_TV_filter[filter_tb].filter[filter][2],
				XGI_TV_filter[filter_tb].filter[filter][3]
			);
			outXGIIDXREG(XGIPART2, 0x35, (XGI_TV_filter[filter_tb].filter[filter][0]));
			outXGIIDXREG(XGIPART2, 0x36, (XGI_TV_filter[filter_tb].filter[filter][1]));
			outXGIIDXREG(XGIPART2, 0x37, (XGI_TV_filter[filter_tb].filter[filter][2]));
			outXGIIDXREG(XGIPART2, 0x38, (XGI_TV_filter[filter_tb].filter[filter][3]));
		}

	     }
	  
	}

}

#ifndef MODULE
int XGIfb_setup(char *options)
{
	char *this_opt;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	XGI_fb_info.fontname[0] = '\0';
#endif	

	xgi_video_info.refresh_rate = 0;

        printk(KERN_INFO "XGIfb: Options %s\n", options);

	if (!options || !*options)
		return 0;

	while((this_opt = strsep(&options, ",")) != NULL) {

		if (!*this_opt)	continue;

		if (!strncmp(this_opt, "mode:", 5)) {
			XGIfb_search_mode(this_opt + 5);
		} else if (!strncmp(this_opt, "vesa:", 5)) {
			XGIfb_search_vesamode(simple_strtoul(this_opt + 5, NULL, 0));
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)			
		} else if (!strcmp(this_opt, "inverse")) {
			XGIfb_inverse = 1;
			/* fb_invert_cmaps(); */
		} else if (!strncmp(this_opt, "font:", 5)) {
			strcpy(XGI_fb_info.fontname, this_opt + 5);
#endif			
		} else if (!strncmp(this_opt, "mode:", 5)) {
			XGIfb_search_mode(this_opt + 5);
		} else if (!strncmp(this_opt, "vesa:", 5)) {
			XGIfb_search_vesamode(simple_strtoul(this_opt + 5, NULL, 0));
		} else if (!strncmp(this_opt, "vrate:", 6)) {
			xgi_video_info.refresh_rate = simple_strtoul(this_opt + 6, NULL, 0);
		} else if (!strncmp(this_opt, "rate:", 5)) {
			xgi_video_info.refresh_rate = simple_strtoul(this_opt + 5, NULL, 0);
		} else if (!strncmp(this_opt, "off", 3)) {
			XGIfb_off = 1;
		} else if (!strncmp(this_opt, "crt1off", 7)) {
			XGIfb_crt1off = 1;
		} else if (!strncmp(this_opt, "filter:", 7)) {
			filter = (int)simple_strtoul(this_opt + 7, NULL, 0);
		} else if (!strncmp(this_opt, "forcecrt2type:", 14)) {
			XGIfb_search_crt2type(this_opt + 14);
		} else if (!strncmp(this_opt, "forcecrt1:", 10)) {
			XGIfb_forcecrt1 = (int)simple_strtoul(this_opt + 10, NULL, 0);
                } else if (!strncmp(this_opt, "tvmode:",7)) {
		        XGIfb_search_tvstd(this_opt + 7);
                } else if (!strncmp(this_opt, "tvstandard:",11)) {
			XGIfb_search_tvstd(this_opt + 7);
                } else if (!strncmp(this_opt, "mem:",4)) {
		        XGIfb_mem = simple_strtoul(this_opt + 4, NULL, 0);
                } else if (!strncmp(this_opt, "dstn", 4)) {
			enable_dstn = 1;
			/* TW: DSTN overrules forcecrt2type */
			XGIfb_crt2type = DISPTYPE_LCD;
		} else if (!strncmp(this_opt, "queuemode:", 10)) {
			XGIfb_search_queuemode(this_opt + 10);
		} else if (!strncmp(this_opt, "pdc:", 4)) {
		        XGIfb_pdc = simple_strtoul(this_opt + 4, NULL, 0);
		        if(XGIfb_pdc & ~0x3c) {
			   printk(KERN_INFO "XGIfb: Illegal pdc parameter\n");
			   XGIfb_pdc = 0;
		        }
		} else if (!strncmp(this_opt, "noaccel", 7)) {
			XGIfb_accel = 0;
		} else if (!strncmp(this_opt, "noypan", 6)) {
		        XGIfb_ypan = 0;
		} else if (!strncmp(this_opt, "userom:", 7)) {
			XGIfb_userom = (int)simple_strtoul(this_opt + 7, NULL, 0);
		} else if (!strncmp(this_opt, "useoem:", 7)) {
			XGIfb_useoem = (int)simple_strtoul(this_opt + 7, NULL, 0);
		} else {
			printk(KERN_INFO "XGIfb: Invalid option %s\n", this_opt);
		}

		/* TW: Acceleration only with MMIO mode */
		if((XGIfb_queuemode != -1) && (XGIfb_queuemode != MMIO_CMD)) {
			XGIfb_ypan = 0;
			XGIfb_accel = 0;
		}
		/* TW: Panning only with acceleration */
		if(XGIfb_accel == 0) XGIfb_ypan = 0;

	}
	return 0;
}
#endif

static char *XGI_find_rom(void)
{
#if defined(__i386__)
        u32  segstart;
        unsigned char *rom_base;
        unsigned char *rom;
        int  stage;
        int  i;
        char XGI_rom_sig[] = "XGI Technology, Inc.";
        
        char *XGI_sig[5] = {
          "Volari GPU", "XG41","XG42", "XG45", "Volari Z7"
        };
	
	unsigned short XGI_nums[5] = {
	    XG40, XG41, XG42, XG45, XG20
	};

        for(segstart=0x000c0000; segstart<0x000f0000; segstart+=0x00001000) {

                stage = 1;

                rom_base = (char *)ioremap(segstart, 0x1000);

                if ((*rom_base == 0x55) && (((*(rom_base + 1)) & 0xff) == 0xaa))
                   stage = 2;

                if (stage != 2) {
                   iounmap(rom_base);
                   continue;
                }


		rom = rom_base + (unsigned short)(*(rom_base + 0x12) | (*(rom_base + 0x13) << 8));
                if(strncmp(XGI_rom_sig, rom, strlen(XGI_rom_sig)) == 0) {
                    stage = 3;
		}
                if(stage != 3) {
                    iounmap(rom_base);
                    continue;
                }

		rom = rom_base + (unsigned short)(*(rom_base + 0x14) | (*(rom_base + 0x15) << 8));
        for(i = 0;(i < 5) && (stage != 4); i++) {
            if(strncmp(XGI_sig[i], rom, strlen(XGI_sig[i])) == 0) {
		        if(XGI_nums[i] == xgi_video_info.chip) {
                    stage = 4;
                    break;
			    }
            }
        }
		

        if(stage != 4) {
            iounmap(rom_base);
            continue;
        }

        return rom_base;
        }
#endif
        return NULL;
}



int __init XGIfb_init(void)
{
	struct pci_dev *pdev = NULL;
	struct board *b;
	int pdev_valid = 0;
	u32 reg32;
	u16 reg16;
	u8  reg, reg1;

	/* outb(0x77, 0x80); */  /* What is this? */

#if 0 	
	/* for DOC VB */
	XGIfb_set_reg4(0xcf8,0x800000e0);
	reg32 = XGIfb_get_reg3(0xcfc);
	reg32 = reg32 | 0x00001000;
	XGIfb_set_reg4(0xcfc,reg32);
	}
#endif

	if (XGIfb_off)
		return -ENXIO;

//	if (enable_dstn)
//		XGI_SetEnableDstn(&XGI_Pr);
		
	XGIfb_registered = 0;

	memset(&XGI_fb_info, 0, sizeof(XGI_fb_info));
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
	memset(&XGI_disp, 0, sizeof(XGI_disp));
#endif	

	while ((pdev = pci_find_device(PCI_ANY_ID, PCI_ANY_ID, pdev)) != NULL) {
		for (b = XGIdev_list; b->vendor; b++) {
			if ((b->vendor == pdev->vendor)
			    && (b->device == pdev->device)) {
				pdev_valid = 1;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,5,0)				
				strcpy(XGI_fb_info.modename, b->name);
#else				
				strcpy(myid, b->name);
#endif				
				xgi_video_info.chip_id = pdev->device;
				pci_read_config_byte(pdev, PCI_REVISION_ID,
				                     &xgi_video_info.revision_id);
				pci_read_config_word(pdev, PCI_COMMAND, &reg16);
				XGIhw_ext.jChipRevision = xgi_video_info.revision_id;
				XGIvga_enabled = reg16 & 0x01;
				xgi_video_info.pcibus = pdev->bus->number;
				xgi_video_info.pcislot = PCI_SLOT(pdev->devfn);
				xgi_video_info.pcifunc = PCI_FUNC(pdev->devfn);
				xgi_video_info.subsysvendor = pdev->subsystem_vendor;
				xgi_video_info.subsysdevice = pdev->subsystem_device;
				break;
			}
		}

		if (pdev_valid)
			break;
	}

	if (!pdev_valid)
		return -ENODEV;

	switch (xgi_video_info.chip_id) {


	   case PCI_DEVICE_ID_XG_20:
		xgi_video_info.chip = XG20;
		XGIfb_hwcursor_size = HW_CURSOR_AREA_SIZE_315 * 2;
		XGIfb_CRT2_write_enable = IND_XGI_CRT2_WRITE_ENABLE_315;
		break;
	    case PCI_DEVICE_ID_XG_40:
		xgi_video_info.chip = XG40;
		XGIfb_hwcursor_size = HW_CURSOR_AREA_SIZE_315 * 2;
		XGIfb_CRT2_write_enable = IND_XGI_CRT2_WRITE_ENABLE_315;
		break;
	    case PCI_DEVICE_ID_XG_41:
		xgi_video_info.chip = XG41;
		XGIfb_hwcursor_size = HW_CURSOR_AREA_SIZE_315 * 2;
		XGIfb_CRT2_write_enable = IND_XGI_CRT2_WRITE_ENABLE_315;
		break;
	    case PCI_DEVICE_ID_XG_42:
		xgi_video_info.chip = XG42;
		XGIfb_hwcursor_size = HW_CURSOR_AREA_SIZE_315 * 2;
		XGIfb_CRT2_write_enable = IND_XGI_CRT2_WRITE_ENABLE_315;
		break;
	    case PCI_DEVICE_ID_XG_45:
		xgi_video_info.chip = XG45;
		XGIfb_hwcursor_size = HW_CURSOR_AREA_SIZE_315 * 2;
		XGIfb_CRT2_write_enable = IND_XGI_CRT2_WRITE_ENABLE_315;
		break;


           default:
	        return -ENODEV;
	}
	XGIhw_ext.jChipType = xgi_video_info.chip;

	
	xgi_video_info.video_base = pci_resource_start(pdev, 0);
	xgi_video_info.mmio_base = pci_resource_start(pdev, 1);
	XGIhw_ext.pjIOAddress = (unsigned short) xgi_video_info.vga_base =
	    (unsigned short) XGI_Pr.RelIO  = pci_resource_start(pdev, 2) + 0x30;

	XGIfb_mmio_size =  pci_resource_len(pdev, 1);

	if(!XGIvga_enabled) {
		if (pci_enable_device(pdev))   return -EIO;
	}

	
	XGIRegInit(&XGI_Pr, (USHORT)XGIhw_ext.pjIOAddress);


        outXGIIDXREG(XGISR, IND_XGI_PASSWORD, XGI_PASSWORD);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)		
#ifdef MODULE	
	inXGIIDXREG(XGICR,0x34,reg);
	if(reg & 0x80) {
	   if((XGIbios_mode[XGIfb_mode_idx].mode_no) != 0xFF) {
	      printk(KERN_INFO "XGIfb: Cannot initialize display mode, X server is active\n");
	      return -EBUSY;
	   }
	}
#endif	
#endif

	
		switch (xgi_video_info.chip) {
		   case XG40:
		   case XG41:
		   case XG42:
		   case XG45:
		   case XG20:
			XGIhw_ext.bIntegratedMMEnabled = TRUE;
			break;

		   default:
			break;
		}
	

	XGIhw_ext.pDevice = NULL;
	if(XGIfb_userom) {
	    XGIhw_ext.pjVirtualRomBase = XGI_find_rom();
	    if(XGIhw_ext.pjVirtualRomBase) {
		printk(KERN_INFO "XGIfb: Video ROM found and mapped to %p\n",
		        XGIhw_ext.pjVirtualRomBase);

	    } else {

	        printk(KERN_INFO "XGIfb: Video ROM not found\n");
	    }
	} else {
	    XGIhw_ext.pjVirtualRomBase = NULL;

	    printk(KERN_INFO "XGIfb: Video ROM usage disabled\n");
	}
	XGIhw_ext.pjCustomizedROMImage = NULL;
	XGIhw_ext.bSkipDramSizing = 0;
	XGIhw_ext.pQueryVGAConfigSpace = &XGIfb_query_VGA_config_space;
//	XGIhw_ext.pQueryNorthBridgeSpace = &XGIfb_query_north_bridge_space;
	strcpy(XGIhw_ext.szVBIOSVer, "0.84");

	/* TW: Mode numbers for 1280x960 are different for 300 and 310/325 series */


	XGIhw_ext.pSR = vmalloc(sizeof(XGI_DSReg) * SR_BUFFER_SIZE);
	if (XGIhw_ext.pSR == NULL) {
		printk(KERN_ERR "XGIfb: Fatal error: Allocating SRReg space failed.\n");
		return -ENODEV;
	}
	XGIhw_ext.pSR[0].jIdx = XGIhw_ext.pSR[0].jVal = 0xFF;

	XGIhw_ext.pCR = vmalloc(sizeof(XGI_DSReg) * CR_BUFFER_SIZE);
	if (XGIhw_ext.pCR == NULL) {
	        vfree(XGIhw_ext.pSR);
		printk(KERN_ERR "XGIfb: Fatal error: Allocating CRReg space failed.\n");
		return -ENODEV;
	}
	XGIhw_ext.pCR[0].jIdx = XGIhw_ext.pCR[0].jVal = 0xFF;



	
		if (!XGIvga_enabled) {
			/* Mapping Max FB Size for 315 Init */
			XGIhw_ext.pjVideoMemoryAddress 
				= ioremap(xgi_video_info.video_base, 0x8000000);
			if((XGIfb_mode_idx < 0) || ((XGIbios_mode[XGIfb_mode_idx].mode_no) != 0xFF)) { 
#ifdef LINUXBIOS
			        /* TW: XGIInit is now for LINUXBIOS only */
				XGIInit(&XGI_Pr, &XGIhw_ext);
#endif

				outXGIIDXREG(XGISR, IND_XGI_PASSWORD, XGI_PASSWORD);

				XGIhw_ext.bSkipDramSizing = TRUE;
				XGIhw_ext.pSR[0].jIdx = 0x13;
				XGIhw_ext.pSR[1].jIdx = 0x14;
				XGIhw_ext.pSR[2].jIdx = 0xFF;
				inXGIIDXREG(XGISR, 0x13, XGIhw_ext.pSR[0].jVal);
				inXGIIDXREG(XGISR, 0x14, XGIhw_ext.pSR[1].jVal);
				XGIhw_ext.pSR[2].jVal = 0xFF;
			}
		}
#ifdef LINUXBIOS
		else {
			XGIhw_ext.pjVideoMemoryAddress
				= ioremap(xgi_video_info.video_base, 0x8000000);
			if((XGIfb_mode_idx < 0) || ((XGIbios_mode[XGIfb_mode_idx].mode_no) != 0xFF)) { 

				XGIInit(&XGI_Pr, &XGIhw_ext);

				outXGIIDXREG(XGISR, IND_XGI_PASSWORD, XGI_PASSWORD);

				XGIhw_ext.bSkipDramSizing = TRUE;
				XGIhw_ext.pSR[0].jIdx = 0x13;
				XGIhw_ext.pSR[1].jIdx = 0x14;
				XGIhw_ext.pSR[2].jIdx = 0xFF;
				inXGIIDXREG(XGISR, 0x13, XGIhw_ext.pSR[0].jVal);
				inXGIIDXREG(XGISR, 0x14, XGIhw_ext.pSR[1].jVal);
				XGIhw_ext.pSR[2].jVal = 0xFF;
			}
		}
#endif
		if (XGIfb_get_dram_size()) {
			vfree(XGIhw_ext.pSR);
			vfree(XGIhw_ext.pCR);
			printk(KERN_INFO "XGIfb: Fatal error: Unable to determine RAM size.\n");
			return -ENODEV;
		}
	


	if((XGIfb_mode_idx < 0) || ((XGIbios_mode[XGIfb_mode_idx].mode_no) != 0xFF)) { 

	        /* Enable PCI_LINEAR_ADDRESSING and MMIO_ENABLE  */
	        orXGIIDXREG(XGISR, IND_XGI_PCI_ADDRESS_SET, (XGI_PCI_ADDR_ENABLE | XGI_MEM_MAP_IO_ENABLE));

                /* Enable 2D accelerator engine */
	        orXGIIDXREG(XGISR, IND_XGI_MODULE_ENABLE, XGI_ENABLE_2D);

	}

	XGIhw_ext.ulVideoMemorySize = xgi_video_info.video_size;



	if (!request_mem_region(xgi_video_info.video_base, xgi_video_info.video_size, "XGIfb FB")) {
		printk(KERN_ERR "XGIfb: Fatal error: Unable to reserve frame buffer memory\n");
		printk(KERN_ERR "XGIfb: Is there another framebuffer driver active?\n");
		vfree(XGIhw_ext.pSR);
		vfree(XGIhw_ext.pCR);
		return -ENODEV;
	}

	if (!request_mem_region(xgi_video_info.mmio_base, XGIfb_mmio_size, "XGIfb MMIO")) {
		printk(KERN_ERR "XGIfb: Fatal error: Unable to reserve MMIO region\n");
		release_mem_region(xgi_video_info.video_base, xgi_video_info.video_size);
		vfree(XGIhw_ext.pSR);
		vfree(XGIhw_ext.pCR);
		return -ENODEV;
	}

	xgi_video_info.video_vbase = XGIhw_ext.pjVideoMemoryAddress = 
		ioremap(xgi_video_info.video_base, xgi_video_info.video_size);
	xgi_video_info.mmio_vbase = ioremap(xgi_video_info.mmio_base, XGIfb_mmio_size);

	printk(KERN_INFO "XGIfb: Framebuffer at 0x%lx, mapped to 0x%p, size %dk\n",
	       xgi_video_info.video_base, xgi_video_info.video_vbase,
	       xgi_video_info.video_size / 1024);

	printk(KERN_INFO "XGIfb: MMIO at 0x%lx, mapped to 0x%p, size %ldk\n",
	       xgi_video_info.mmio_base, xgi_video_info.mmio_vbase,
	       XGIfb_mmio_size / 1024);

	if(XGIfb_heap_init()) {
		printk(KERN_WARNING "XGIfb: Failed to initialize offscreen memory heap\n");
	}

	xgi_video_info.mtrr = (unsigned int) 0;

	if((XGIfb_mode_idx < 0) || ((XGIbios_mode[XGIfb_mode_idx].mode_no) != 0xFF)) { 

        if(xgi_video_info.chip == XG20)
            xgi_video_info.hasVB = HASVB_NONE;            
        else
		    XGIfb_get_VB_type();

		XGIhw_ext.ujVBChipID = VB_CHIP_UNKNOWN;

		XGIhw_ext.ulExternalChip = 0;

		switch (xgi_video_info.hasVB) {

		case HASVB_301:
		        inXGIIDXREG(XGIPART4, 0x01, reg);
			if (reg >= 0xE0) {
				XGIhw_ext.ujVBChipID = VB_CHIP_302LV;
				printk(KERN_INFO "XGIfb: XGI302LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if (reg >= 0xD0) {
				XGIhw_ext.ujVBChipID = VB_CHIP_301LV;
				printk(KERN_INFO "XGIfb: XGI301LV bridge detected (revision 0x%02x)\n",reg);
	  		}/* else if (reg >= 0xB0) {
				XGIhw_ext.ujVBChipID = VB_CHIP_301B;
				inXGIIDXREG(XGIPART4,0x23,reg1);
				
			}*/ else {
				XGIhw_ext.ujVBChipID = VB_CHIP_301;
				printk(KERN_INFO "XGIfb: XGI301 bridge detected\n");
			}
			break;
		case HASVB_302:
		        inXGIIDXREG(XGIPART4, 0x01, reg);
			if (reg >= 0xE0) {
				XGIhw_ext.ujVBChipID = VB_CHIP_302LV;
				printk(KERN_INFO "XGIfb: XGI302LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if (reg >= 0xD0) {
				XGIhw_ext.ujVBChipID = VB_CHIP_301LV;
				printk(KERN_INFO "XGIfb: XGI302LV bridge detected (revision 0x%02x)\n",reg);
	  		} else if (reg >= 0xB0) {
				inXGIIDXREG(XGIPART4,0x23,reg1);

			        XGIhw_ext.ujVBChipID = VB_CHIP_302B;

			} else {
			        XGIhw_ext.ujVBChipID = VB_CHIP_302;
				printk(KERN_INFO "XGIfb: XGI302 bridge detected\n");
			}
			break;
		case HASVB_LVDS:
			XGIhw_ext.ulExternalChip = 0x1;
			printk(KERN_INFO "XGIfb: LVDS transmitter detected\n");
			break;
/*
		case HASVB_TRUMPION:
			XGIhw_ext.ulExternalChip = 0x2;
			printk(KERN_INFO "XGIfb: Trumpion Zurac LVDS scaler detected\n");
			break;
		case HASVB_CHRONTEL:
			XGIhw_ext.ulExternalChip = 0x4;
			printk(KERN_INFO "XGIfb: Chrontel TV encoder detected\n");
			break;
		case HASVB_LVDS_CHRONTEL:
			XGIhw_ext.ulExternalChip = 0x5;
			printk(KERN_INFO "XGIfb: LVDS transmitter and Chrontel TV encoder detected\n");
			break;
*/
		default:
			printk(KERN_INFO "XGIfb: No or unknown bridge type detected\n");
			break;
		}

		if (xgi_video_info.hasVB != HASVB_NONE) {
		    XGIfb_detect_VB();
        }

		if (xgi_video_info.disp_state & DISPTYPE_DISP2) {
			if (XGIfb_crt1off)
				xgi_video_info.disp_state |= DISPMODE_SINGLE;
			else
				xgi_video_info.disp_state |= (DISPMODE_MIRROR | DISPTYPE_CRT1);
		} else {
			xgi_video_info.disp_state = DISPMODE_SINGLE | DISPTYPE_CRT1;
		}

		if (xgi_video_info.disp_state & DISPTYPE_LCD) {
		    if (!enable_dstn) {
		        inXGIIDXREG(XGICR, IND_XGI_LCD_PANEL, reg);
			    reg &= 0x0f;
			    XGIhw_ext.ulCRT2LCDType = XGI310paneltype[reg];
			
		    } else {
		        // TW: FSTN/DSTN 
			XGIhw_ext.ulCRT2LCDType = LCD_320x480;
		    }
		}
		
		XGIfb_detectedpdc = 0;

	        XGIfb_detectedlcda = 0xff;
#ifndef LINUXBIOS

                /* TW: Try to find about LCDA */
		
        if((XGIhw_ext.ujVBChipID == VB_CHIP_302B) ||
	       (XGIhw_ext.ujVBChipID == VB_CHIP_301LV) ||
	       (XGIhw_ext.ujVBChipID == VB_CHIP_302LV)) 
	    {
	       int tmp;
	       inXGIIDXREG(XGICR,0x34,tmp);
	       if(tmp <= 0x13) 
	       {	
	          /* Currently on LCDA? (Some BIOSes leave CR38) */
	          inXGIIDXREG(XGICR,0x38,tmp);
		      if((tmp & 0x03) == 0x03) 
		      {
//		          XGI_Pr.XGI_UseLCDA = TRUE;
		      }else
		      {
		     /* Currently on LCDA? (Some newer BIOSes set D0 in CR35) */
		         inXGIIDXREG(XGICR,0x35,tmp);
		         if(tmp & 0x01) 
		         {
//		              XGI_Pr.XGI_UseLCDA = TRUE;
		           }else
		           {
		               inXGIIDXREG(XGICR,0x30,tmp);
		               if(tmp & 0x20) 
		               {
		                   inXGIIDXREG(XGIPART1,0x13,tmp);
			               if(tmp & 0x04) 
			               {
//			                XGI_Pr.XGI_UseLCDA = TRUE;
			               }
		               }
		           }
		        }
	         } 

        }
		

#endif

		if (XGIfb_mode_idx >= 0)
			XGIfb_mode_idx = XGIfb_validate_mode(XGIfb_mode_idx);

		if (XGIfb_mode_idx < 0) {
			switch (xgi_video_info.disp_state & DISPTYPE_DISP2) {
			   case DISPTYPE_LCD:
				XGIfb_mode_idx = DEFAULT_LCDMODE;
				break;
			   case DISPTYPE_TV:
				XGIfb_mode_idx = DEFAULT_TVMODE;
				break;
			   default:
				XGIfb_mode_idx = DEFAULT_MODE;
				break;
			}
		}

		XGIfb_mode_no = XGIbios_mode[XGIfb_mode_idx].mode_no;

		if (xgi_video_info.refresh_rate != 0)
			XGIfb_search_refresh_rate(xgi_video_info.refresh_rate);

		if (XGIfb_rate_idx == 0) {
			XGIfb_rate_idx = XGIbios_mode[XGIfb_mode_idx].rate_idx;
			xgi_video_info.refresh_rate = 60;
		}

		xgi_video_info.video_bpp = XGIbios_mode[XGIfb_mode_idx].bpp;
		xgi_video_info.video_vwidth = xgi_video_info.video_width = XGIbios_mode[XGIfb_mode_idx].xres;
		xgi_video_info.video_vheight = xgi_video_info.video_height = XGIbios_mode[XGIfb_mode_idx].yres;
		xgi_video_info.org_x = xgi_video_info.org_y = 0;
		xgi_video_info.video_linelength = xgi_video_info.video_width * (xgi_video_info.video_bpp >> 3);
		switch(xgi_video_info.video_bpp) {
        	case 8:
            		xgi_video_info.DstColor = 0x0000;
	    		xgi_video_info.XGI310_AccelDepth = 0x00000000;
			xgi_video_info.video_cmap_len = 256;
            		break;
        	case 16:
            		xgi_video_info.DstColor = 0x8000;
            		xgi_video_info.XGI310_AccelDepth = 0x00010000;
			xgi_video_info.video_cmap_len = 16;
            		break;
        	case 32:
            		xgi_video_info.DstColor = 0xC000;
	    		xgi_video_info.XGI310_AccelDepth = 0x00020000;
			xgi_video_info.video_cmap_len = 16;
            		break;
		default:
			xgi_video_info.video_cmap_len = 16;
		        printk(KERN_INFO "XGIfb: Unsupported depth %d", xgi_video_info.video_bpp);
			break;
    		}
		
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	

		/* ---------------- For 2.4: Now switch the mode ------------------ */		
		
		printk(KERN_INFO "XGIfb: Mode is %dx%dx%d (%dHz)\n",
	       		xgi_video_info.video_width, xgi_video_info.video_height, xgi_video_info.video_bpp,
			xgi_video_info.refresh_rate);

		XGIfb_pre_setmode();

		if (XGISetModeNew(&XGIhw_ext, XGIfb_mode_no) == 0) {
			printk(KERN_ERR "XGIfb: Setting mode[0x%x] failed, using default mode\n", 
				XGIfb_mode_no);
			return -1;
		}

		outXGIIDXREG(XGISR, IND_XGI_PASSWORD, XGI_PASSWORD);

		XGIfb_post_setmode();

		XGIfb_crtc_to_var(&default_var);
		
#else		/* --------- For 2.5: Setup a somewhat sane default var ------------ */

		printk(KERN_INFO "XGIfb: Default mode is %dx%dx%d (%dHz)\n",
	       		xgi_video_info.video_width, xgi_video_info.video_height, xgi_video_info.video_bpp,
			xgi_video_info.refresh_rate);
			
		default_var.xres = default_var.xres_virtual = xgi_video_info.video_width;
		default_var.yres = default_var.yres_virtual = xgi_video_info.video_height;
		default_var.bits_per_pixel = xgi_video_info.video_bpp;
		
		XGIfb_bpp_to_var(&default_var);
		
		default_var.pixclock = (u32) (1E12 /
				XGIfb_mode_rate_to_dclock(&XGI_Pr, &XGIhw_ext,
						XGIfb_mode_no, XGIfb_rate_idx));
						
		if(XGIfb_mode_rate_to_ddata(&XGI_Pr, &XGIhw_ext,
			 XGIfb_mode_no, XGIfb_rate_idx,
			 &default_var.left_margin, &default_var.right_margin, 
			 &default_var.upper_margin, &default_var.lower_margin,
			 &default_var.hsync_len, &default_var.vsync_len,
			 &default_var.sync, &default_var.vmode)) {
			 
		   if((default_var.vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED) {
		      default_var.yres <<= 1;
		      default_var.yres_virtual <<= 1;
		   } else if((default_var.vmode	& FB_VMODE_MASK) == FB_VMODE_DOUBLE) {
		      default_var.pixclock >>= 1;
		      default_var.yres >>= 1;
		      default_var.yres_virtual >>= 1;
		   }
		   
	        }
#ifdef XGIFB_PAN
		if(XGIfb_ypan) {
	    		default_var.yres_virtual = 
				xgi_video_info.heapstart / (default_var.xres * (default_var.bits_per_pixel >> 3));
	    		if(default_var.yres_virtual <= default_var.yres) {
	        		default_var.yres_virtual = default_var.yres;
	    		}
		} 
#endif
		
#endif

		xgi_video_info.accel = 0;
		if(XGIfb_accel) {
		   xgi_video_info.accel = -1;
		   default_var.accel_flags |= FB_ACCELF_TEXT;
		   XGIfb_initaccel();
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)		/* ---- 2.4 series init ---- */
		XGI_fb_info.node = -1;
		XGI_fb_info.flags = FBINFO_FLAG_DEFAULT;
		XGI_fb_info.blank = &XGIfb_blank;
		XGI_fb_info.fbops = &XGIfb_ops;
		XGI_fb_info.switch_con = &XGIfb_switch;
		XGI_fb_info.updatevar = &XGIfb_update_var;
		XGI_fb_info.changevar = NULL;
		XGI_fb_info.disp = &XGI_disp;
			
		XGIfb_set_disp(-1, &default_var, &XGI_fb_info);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)		/* ---- 2.5 series init ---- */
		XGI_fb_info.flags = FBINFO_FLAG_DEFAULT;
		XGI_fb_info.var = default_var;
		XGI_fb_info.fix = XGIfb_fix;
		XGI_fb_info.par = &xgi_video_info;
		XGI_fb_info.screen_base = xgi_video_info.video_vbase;
		XGI_fb_info.fbops = &XGIfb_ops;
		XGIfb_get_fix(&XGI_fb_info.fix, -1, &XGI_fb_info);
		XGI_fb_info.pseudo_palette = pseudo_palette;
		
		fb_alloc_cmap(&XGI_fb_info.cmap, 256 , 0);
#endif

#ifdef CONFIG_MTRR
		xgi_video_info.mtrr = mtrr_add((unsigned int) xgi_video_info.video_base,
				(unsigned int) xgi_video_info.video_size,
				MTRR_TYPE_WRCOMB, 1);
		if(xgi_video_info.mtrr) {
			printk(KERN_INFO "XGIfb: Added MTRRs\n");
		}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		vc_resize_con(1, 1, 0);
#endif

		DEBUGPRN("Before calling register_framebuffer");
		
		if(register_framebuffer(&XGI_fb_info) < 0)
			return -EINVAL;
			
		XGIfb_registered = 1;			

		printk(KERN_INFO "XGIfb: Installed XGIFB_GET_INFO ioctl (%x)\n", XGIFB_GET_INFO);
		
		printk(KERN_INFO "XGIfb: 2D acceleration is %s, scrolling mode %s\n",
		     XGIfb_accel ? "enabled" : "disabled",
		     XGIfb_ypan  ? "ypan" : "redraw");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
		printk(KERN_INFO "fb%d: %s frame buffer device, Version %d.%d.%02d\n",
	       		GET_FB_IDX(XGI_fb_info.node), XGI_fb_info.modename, VER_MAJOR, VER_MINOR,
	       		VER_LEVEL);		     
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		printk(KERN_INFO "fb%d: %s frame buffer device, Version %d.%d.%02d\n",
	       		XGI_fb_info.node, myid, VER_MAJOR, VER_MINOR, VER_LEVEL);			     
#endif

	}	/* TW: if mode = "none" */
	return 0;
}


#ifdef MODULE

static char         *mode = NULL;
static int          vesa = -1;
static unsigned int rate = 0;
static unsigned int crt1off = 1;
static unsigned int mem = 0;
static unsigned int dstn = 0;
static char         *forcecrt2type = NULL;
static int          forcecrt1 = -1;
static char         *queuemode = NULL;
static int          pdc = 0;
static int          noaccel = -1;
static int          noypan  = -1;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
static int          inverse = 0;
#endif
static int          userom = 1;
static int          useoem = -1;
static char         *tvstandard = NULL;

MODULE_DESCRIPTION(DRIVER_DESC );
MODULE_LICENSE("GPL");
MODULE_AUTHOR("XGI; Thomas Winischhofer <thomas@winischhofer.net>; Various others");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
MODULE_PARM(mode, "s");
MODULE_PARM_DESC(mode,
       "\nSelects the desired display mode in the format [X]x[Y]x[Depth], eg.\n"
         "800x600x16 (default: none if XGIfb is a module; this leaves the\n"
	 "console untouched and the driver will only do the video memory\n"
	 "management for eg. DRM/DRI; 800x600x8 if XGIfb is in the kernel)");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)	 
MODULE_PARM(mode, "s");
MODULE_PARM_DESC(mode,
       "\nSelects the desired default display mode in the format [X]x[Y]x[Depth],\n"
         "eg. 1024x768x16 (default: 800x600x8)");
#endif	 

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
MODULE_PARM(vesa, "i");
MODULE_PARM_DESC(vesa,
       "\nSelects the desired display mode by VESA defined mode number, eg. 0x117\n"
         "(default: 0x0000 if XGIfb is a module; this leaves the console untouched\n"
	 "and the driver will only do the video memory management for eg. DRM/DRI;\n"
	 "0x0103 if XGIfb is in the kernel)");
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)	 
MODULE_PARM(vesa, "i");
MODULE_PARM_DESC(vesa,
       "\nSelects the desired default display mode by VESA defined mode number, eg.\n"
         "0x117 (default: 0x0103)");
#endif	 

MODULE_PARM(rate, "i");
MODULE_PARM_DESC(rate,
	"\nSelects the desired vertical refresh rate for CRT1 (external VGA) in Hz.\n"
	"(default: 60)");

MODULE_PARM(crt1off,   "i");
MODULE_PARM_DESC(crt1off,
	"(Deprecated, please use forcecrt1)");

MODULE_PARM(filter, "i");
MODULE_PARM_DESC(filter,
	"\nSelects TV flicker filter type (only for systems with a XGI301 video bridge).\n"
	  "(Possible values 0-7, default: [no filter])");

MODULE_PARM(dstn,   "i");
MODULE_PARM_DESC(dstn,
	"\nSelects DSTN/FSTN display mode for XGI550. This sets CRT2 type to LCD and\n"
	  "overrides forcecrt2type setting. (1=ON, 0=OFF) (default: 0)");

MODULE_PARM(queuemode,   "s");
MODULE_PARM_DESC(queuemode,
	"\nSelects the queue mode on 315/550/650/740/330. Possible choices are AGP, VRAM or\n"
  	  "MMIO. AGP is only available if the kernel has AGP support. The queue mode is\n"
	  "important to programs using the 2D/3D accelerator of the XGI chip. The modes\n"
	  "require a totally different way of programming the engines. If any mode than\n"
	  "MMIO is selected, XGIfb will disable its own 2D acceleration. On\n"
	  "300/540/630/730, this option is ignored. (default: MMIO)");

/* TW: "Import" the options from the X driver */
MODULE_PARM(mem,    "i");
MODULE_PARM_DESC(mem,
	"\nDetermines the beginning of the video memory heap in KB. This heap is used\n"
	  "for video RAM management for eg. DRM/DRI. The default depends on the amount\n"
	  "of video RAM available. If 8MB of video RAM or less is available, the heap\n"
	  "starts at 4096KB, if between 8 and 16MB are available at 8192KB, otherwise\n"
	  "at 12288KB. The value is to be specified without 'KB' and should match\n"
	  "the MaxXFBMem setting for XFree 4.x (x>=2).");

MODULE_PARM(forcecrt2type, "s");
MODULE_PARM_DESC(forcecrt2type,
	"\nIf this option is omitted, the driver autodetects CRT2 output devices, such as\n"
	  "LCD, TV or secondary VGA. With this option, this autodetection can be\n"
	  "overridden. Possible parameters are LCD, TV, VGA or NONE. NONE disables CRT2.\n"
	  "On systems with a 301(B/LV) bridge, parameters SVIDEO, COMPOSITE or SCART can be\n"
	  "used instead of TV to override the TV detection. (default: [autodetected])");

MODULE_PARM(forcecrt1, "i");
MODULE_PARM_DESC(forcecrt1,
	"\nNormally, the driver autodetects whether or not CRT1 (external VGA) is \n"
	  "connected. With this option, the detection can be overridden (1=CRT1 ON,\n"
	  " 0=CRT1 off) (default: [autodetected])");

MODULE_PARM(pdc, "i");
MODULE_PARM_DESC(pdc,
        "\n(300 series only) This is for manually selecting the LCD panel delay\n"
	  "compensation. The driver should detect this correctly in most cases; however,\n"
	  "sometimes this is not possible. If you see 'small waves' on the LCD, try\n"
	  "setting this to 4, 32 or 24. If the problem perXGIts, try other values\n"
	  "between 4 and 60 in steps of 4. (default: [autodetected])");

MODULE_PARM(noaccel, "i");
MODULE_PARM_DESC(noaccel,
        "\nIf set to anything other than 0, 2D acceleration and y-panning will be\n"
	"disabled. (default: 0)");

MODULE_PARM(noypan, "i");
MODULE_PARM_DESC(noypan,
        "\nIf set to anything other than 0, y-panning will be disabled and scrolling\n"
	"will be performed by redrawing the screen. This required 2D acceleration, so\n"
	"if the option noaccel is set, y-panning will be disabled. (default: 0)");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
MODULE_PARM(inverse, "i");
MODULE_PARM_DESC(inverse,
        "\nSetting this to anything but 0 should invert the display colors, but this\n"
	"does not seem to work. (default: 0)");
#endif	

MODULE_PARM(userom, "i");
MODULE_PARM_DESC(userom,
        "\nSetting this to 0 keeps XGIfb from using the video BIOS data which is needed\n"
	"for some LCD and TV setup. (default: 1)");

MODULE_PARM(useoem, "i");
MODULE_PARM_DESC(useoem,
        "\nSetting this to 0 keeps XGIfb from using its internel OEM data for some LCD\n"
	"panels and TV connector types. (default: auto)");

MODULE_PARM(tvstandard, "s");
MODULE_PARM_DESC(tvstandard,
	"\nThis allows overriding the BIOS default for the TV standard. Valid choices are\n"
	"pal and ntsc. (default: auto)");

int init_module(void)
{
	int err;
	
	if(mode)
		XGIfb_search_mode(mode);
	else if(vesa != -1)
		XGIfb_search_vesamode(vesa);
	else  
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)	
		/* For 2.4, set mode=none if no mode is given  */
		XGIfb_mode_idx = MODE_INDEX_NONE;
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
		/* For 2.5, we don't need this "mode=none" stuff anymore */	
		XGIfb_mode_idx = DEFAULT_MODE;
#endif

	xgi_video_info.refresh_rate = rate;

	if(forcecrt2type)
		XGIfb_search_crt2type(forcecrt2type);

	if(tvstandard)
		XGIfb_search_tvstd(tvstandard);

	if(crt1off == 0)
		XGIfb_crt1off = 1;
	else
		XGIfb_crt1off = 0;

	XGIfb_forcecrt1 = forcecrt1;
	if(forcecrt1 == 1)
		XGIfb_crt1off = 0;
	else if(forcecrt1 == 0)
		XGIfb_crt1off = 1;

	if(noaccel == 1)      XGIfb_accel = 0;
	else if(noaccel == 0) XGIfb_accel = 1;

	if(noypan == 1)       XGIfb_ypan = 0;
	else if(noypan == 0)  XGIfb_ypan = 1;

	/* TW: Panning only with acceleration */
	if(XGIfb_accel == 0)  XGIfb_ypan = 0;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
	if(inverse)           XGIfb_inverse = 1;
#endif

	if(mem)		      XGIfb_mem = mem;

	XGIfb_userom = userom;

	XGIfb_useoem = useoem;

	enable_dstn = dstn;

	/* TW: DSTN overrules forcecrt2type */
	if (enable_dstn)      XGIfb_crt2type = DISPTYPE_LCD;

	if (queuemode)        XGIfb_search_queuemode(queuemode);
	
	/* TW: If other queuemode than MMIO, disable 2D accel and ypan */
	if((XGIfb_queuemode != -1) && (XGIfb_queuemode != MMIO_CMD)) {
	        XGIfb_accel = 0;
		XGIfb_ypan = 0;
	}

        if(pdc) {
	   if(!(pdc & ~0x3c)) {
	        XGIfb_pdc = pdc & 0x3c;
	   }
	}

	if((err = XGIfb_init()) < 0) return err;

	return 0;
}

void cleanup_module(void)
{
	/* TW: Release mem regions */
	release_mem_region(xgi_video_info.video_base, xgi_video_info.video_size);
	release_mem_region(xgi_video_info.mmio_base, XGIfb_mmio_size);
	
#ifdef CONFIG_MTRR
	/* TW: Release MTRR region */
	if(xgi_video_info.mtrr) {
		mtrr_del(xgi_video_info.mtrr,
		      (unsigned int)xgi_video_info.video_base,
	              (unsigned int)xgi_video_info.video_size);
	}
#endif

	/* Unregister the framebuffer */
	if(XGIfb_registered) {
		unregister_framebuffer(&XGI_fb_info);
	}
	
	if(XGIhw_ext.pSR) vfree(XGIhw_ext.pSR);
	if(XGIhw_ext.pCR) vfree(XGIhw_ext.pCR);
	
	/* TODO: Restore the initial mode */
	
	printk(KERN_INFO "XGIfb: Module unloaded\n");
}




#endif

EXPORT_SYMBOL(XGI_malloc);
EXPORT_SYMBOL(XGI_free);
EXPORT_SYMBOL(XGI_dispinfo);
EXPORT_SYMBOL(xgi_video_info);
                                                                                           
