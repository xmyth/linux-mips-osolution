#include "osdef.h"


#ifdef LINUX_XF86
#include "xf86.h"
#include "xf86PciInfo.h"
#include "xgi.h"
#include "xgi_regs.h"
#include "vb_i2c.h"
#endif

#ifdef LINUX_KERNEL
#include <asm/io.h>
#include <linux/types.h>
#include <linux/version.h>
#include "XGIfb.h"
#include "vb_i2c.h"
/*#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
#include <video/XGIfb.h>
#else
#include <linux/XGIfb.h>
#endif*/
#endif

#ifdef WIN2000
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>
#include <ntddvdeo.h>
#include <video.h>

#include "xgiv.h"
#include "dd_i2c.h"
#include "tools.h"
#endif

#include "vb_def.h"
#include "vgatypes.h"
#include "vb_struct.h"
#include "vb_util.h"


extern void   XGI_ModCRT1Regs( USHORT ModeNo, USHORT ModeIdIndex, USHORT RefreshRateTableIndex, PXGI_HW_DEVICE_INFO HwDeviceExtension,PVB_DEVICE_INFO  pVBInfo );
extern void   XGI_SetCRT2ECLK( USHORT ModeNo, USHORT ModeIdIndex, USHORT RefreshRateTableIndex, PVB_DEVICE_INFO  pVBInfo);
extern USHORT XGI_GetResInfo ( USHORT ModeNo, USHORT ModeIdIndex, PVB_DEVICE_INFO pVBInfo);
extern void*  XGI_GetTVPtr(USHORT BX, USHORT ModeNo,USHORT ModeIdIndex,USHORT RefreshRateTableIndex, PVB_DEVICE_INFO pVBInfo);

/* CH7007 New function */
/* 2007/05/03 for CH7007 code */
typedef struct _XGI330_CH7007TVRegDataStruct
{
 UCHAR Reg[8];
} XGI330_CH7007TVRegDataStruct;

UCHAR CH7007TVRegNo = 8;
UCHAR CH7007TVIndex[]={0x00,0x04,0x06,0x07,0x0A,0x0B,0x21,0x0E};
UCHAR CH7007TVReg_UNTSC[][8]=
{/*Index :000h,004h,006h,007h,00Ah,00Bh,021h,00Eh */
{0x4A,0x04,0x00,0x0B,0x48,0x00,0x01,0x0B}, /* 00 640x200,640x400 */
{0x4A,0x04,0x00,0x0B,0x48,0x00,0x01,0x0B}, /* 01 640x350  */
{0x4A,0x04,0x00,0x0B,0x48,0x00,0x01,0x0B}, /* 02 720x400  */
{0x4A,0x04,0x00,0x0B,0x39,0x00,0x01,0x0B}, /* 03 720x350  */
{0x6A,0x04,0x00,0x0B,0x2D,0x00,0x01,0x0B}, /* 04 640x480  */
{0x8D,0x04,0x00,0x0B,0x40,0x00,0x01,0x0B}, /* 05 800x600  */ /*07/05/28 index:0A -> data:4E->40*/
{0xEE,0x04,0x00,0x0B,0x39,0x00,0x01,0x0B}  /* 06 1024x768 */
};
UCHAR CH7007TVReg_ONTSC[][8]=
{/*Index :000h,004h,006h,007h,00Ah,00Bh,021h,00Eh */
{0x49,0x04,0x00,0x0B,0x34,0x00,0x01,0x0B}, /* 00 640x200,640x400 */
{0x49,0x04,0x00,0x0B,0x34,0x00,0x01,0x0B}, /* 01 640x350  */
{0x49,0x04,0x00,0x0B,0x34,0x00,0x01,0x0B}, /* 02 720x400  */
{0x49,0x04,0x00,0x0B,0x34,0x00,0x01,0x0B}, /* 03 720x350  */
{0x69,0x04,0x00,0x0B,0x28,0x00,0x01,0x0B}, /* 04 640x480  */
{0x8C,0x04,0x00,0x0B,0x40,0x00,0x01,0x0B}, /* 05 800x600  */
{0xED,0x04,0x00,0x0B,0x39,0x00,0x01,0x0B}  /* 06 1024x768 */
};
UCHAR CH7007TVReg_UPAL[][8]=
{/*Index :000h,004h,006h,007h,00Ah,00Bh,021h,00Eh */
{0x41,0x04,0x00,0x0B,0x50,0x00,0x01,0x0B}, /* 00 640x200,640x400 */
{0x41,0x04,0x00,0x0B,0x50,0x00,0x01,0x0B}, /* 01 640x350  */
{0x41,0x04,0x00,0x0B,0x50,0x00,0x01,0x0B}, /* 02 720x400  */
{0x41,0x04,0x00,0x0B,0x39,0x08,0x01,0x0B}, /* 03 720x350  */
{0x61,0x04,0x00,0x0B,0x39,0x08,0x01,0x0B}, /* 04 640x480  */
{0x84,0x04,0x00,0x0B,0x4A,0x08,0x01,0x0B}, /* 05 800x600  */ /*[Billy]07/06/11 index Ah : data : 5Ch -> 4Ah ,B:10h -> 08h*/
{0xE5,0x04,0x00,0x0B,0x39,0x00,0x01,0x0B}  /* 06 1024x768 */
};
UCHAR CH7007TVReg_OPAL[][8]=
{/*Index :000h,004h,006h,007h,00Ah,00Bh,021h,00Eh */
{0x41,0x04,0x00,0x0B,0x4A,0x00,0x01,0x0B}, /* 00 640x200,640x400 */
{0x41,0x04,0x00,0x0B,0x4A,0x00,0x01,0x0B}, /* 01 640x350  */
{0x41,0x04,0x00,0x0B,0x4A,0x00,0x01,0x0B}, /* 02 720x400  */
{0x41,0x04,0x00,0x0B,0x36,0x08,0x01,0x0B}, /* 03 720x350  */
{0x61,0x04,0x00,0x0B,0x36,0x04,0x01,0x0B}, /* 04 640x480  */
{0x83,0x04,0x00,0x0B,0x4A,0x00,0x01,0x0B}, /* 05 800x600  */
{0xE4,0x04,0x00,0x0B,0x39,0x00,0x01,0x0B}  /* 06 1024x768 */
};

UCHAR CH7007TVVCLKUNTSC[][2]=
{
                {0x60,0x36},  /* 30.2    ; 00 (320x200,320x400,640x200,640x400) */
                {0x60,0x36},  /* 30.2    ; 01 (320x350,640x350) */
                {0x60,0x36},  /* 30.2    ; 02 (360x400,720x400) */
                {0x60,0x36},  /* 30.2    ; 03 (720x350) */
                {0x40,0x4A},  /* 28.19   ; 04 (320x240,640x480) */
                {0x7E,0x32},  /* 47.832  ; 05 (400x300,800x600) */
                {0x4D,0x10}   /* 65.7    ; 06 (512x384,1024x768) */
};

UCHAR CH7007TVVCLKONTSC[][2]=
{
                {0x97,0x2C},  /* 26.2    ; 00 (320x200,320x400,640x200,640x400) */
                {0x97,0x2C},  /* 26.2    ; 01 (320x350,640x350) */
                {0x97,0x2C},  /* 26.2    ; 02 (360x400,720x400) */
                {0x97,0x2C},  /* 26.2    ; 03 (720x350) */
                {0x44,0xE4},  /* 24.6    ; 04 (320x240,640x480) */
                {0x9F,0x46},  /* 43.6    ; 05 (400x300,800x600) */
                {0x65,0x18}   /* 58.4    ; 06 (512x384,1024x768) */
};

UCHAR CH7007TVVCLKUPAL[][2]=
{
                {0x8A,0x24},  /* 31.5    ; 00 (320x200,320x400,640x200,640x400) */
                {0x8A,0x24},  /* 31.5    ; 01 (320x350,640x350) */
                {0x8A,0x24},  /* 31.5    ; 02 (360x400,720x400) */
                {0x8A,0x24},  /* 31.5    ; 03 (720x350) */
                {0x97,0x2C},  /* 26.2    ; 04 (320x240,640x480) */
                {0x52,0x4A},  /* 36      ; 05 (400x300,800x600) */
                {0x3A,0x62}   /* 70      ; 06 (512x384,1024x768) */
};

UCHAR CH7007TVVCLKOPAL[][2]=
{
                {0x8A,0x24},  /* 31.5    ; 00 (320x200,320x400,640x200,640x400) */
                {0x8A,0x24},  /* 31.5    ; 01 (320x350,640x350) */
                {0x8A,0x24},  /* 31.5    ; 02 (360x400,720x400) */
                {0x8A,0x24},  /* 31.5    ; 03 (720x350) */
                {0x97,0x2C},  /* 26.2    ; 04 (320x240,640x480) */
                {0xA1,0x4A},  /* 29.5    ; 05 (400x300,800x600) */
                {0x25,0x42}   /* 61.25   ; 06 (512x384,1024x768) */
};

XGI_CH7007TV_TimingHStruct CH7007TVCRT1UNTSC_H[] =
{/* Index :CR00,CR02,CR03,CR04,CR05,SR0B,SR0C,SR0E(7-5,7-5),SR2E,SR2F */
 {0x64,0x4F,0x88,0x5B,0x9F,0x00,0x01,0x00,0x56,0x64}, /* 00 (640x200,640x400) */
 {0x64,0x4F,0x88,0x5B,0x9F,0x00,0x01,0x00,0x56,0x64}, /* 01 (640x350) */ 
 {0x64,0x4F,0x88,0x5B,0x9F,0x00,0x01,0x00,0x56,0x64}, /* 02 (720x400) */
 {0x64,0x4F,0x88,0x5B,0x9F,0x00,0x01,0x00,0x56,0x64}, /* 03 (720x350) */
 {0x5D,0x4F,0x81,0x57,0x99,0x00,0x01,0x00,0x52,0x4C}, /* 04 (640x480) */
 {0x80,0x63,0x84,0x73,0x16,0x00,0x06,0x21,0x6E,0xC4}  /* 05 (800x600) */
};

XGI_CH7007TV_TimingHStruct CH7007TVCRT1ONTSC_H[] =
{/* Index :CR00,CR02,CR03,CR04,CR05,SR0B,SR0C,SR0E(7-5,7-5),SR2E,SR2F */
 {0x64,0x4F,0x88,0x5D,0x9E,0x00,0x01,0x00,0x58,0x64}, /* 00 (640x200,640x400) */
 {0x64,0x4F,0x88,0x5D,0x9E,0x00,0x01,0x00,0x58,0x64}, /* 01 (640x350) */
 {0x64,0x4F,0x88,0x5D,0x9E,0x00,0x01,0x00,0x56,0x64}, /* 02 (720x400) */
 {0x64,0x4F,0x88,0x5D,0x9E,0x00,0x01,0x00,0x58,0x68}, /* 03 (720x350) */
 {0x5D,0x4F,0x81,0x56,0x9F,0x00,0x01,0x00,0x50,0x68}, /* 04 (640x480) */
 {0x7D,0x63,0x81,0x71,0x15,0x00,0x06,0x01,0x6C,0xC0}  /* 05 (800x600) */
};

XGI_CH7007TV_TimingHStruct CH7007TVCRT1UPAL_H[] =
{/* CR00,CR02,CR03,CR04,CR05,SR0B,SR0C,SR0E(7-5,7-5),SR2E,SR2F */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5D,0x9C}, /* 00 (640x200,640x400) */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5D,0x9C}, /* 01 (640x350) */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5D,0x9C}, /* 02 (720x400) */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5F,0x90}, /* 03 (720x350) */
 {0x64,0x4F,0x88,0x56,0x84,0x00,0x05,0x00,0x51,0x7C}, /* 04 (640x480) */
 /*{0x70,0x63,0x94,0x70,0x8E,0x00,0x05,0x01,0x65,0xA4}*/  /* 05 (800x600) */ 
 {0x70,0x63,0x94,0x66,0x92,0x00,0x05,0x01,0x64,0xA0}  /* 05 (800x600) */ 
};

XGI_CH7007TV_TimingHStruct CH7007TVCRT1OPAL_H[] =
{/* CR00,CR02,CR03,CR04,CR05,SR0B,SR0C,SR0E(7-5,7-5),SR2E,SR2F */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5D,0x9C}, /* 00 (640x200,640x400) */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5D,0x9C}, /* 01 (640x350) */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5D,0x9C}, /* 02 (720x400) */
 {0x79,0x4F,0x9D,0x62,0x8C,0x00,0x05,0x00,0x5F,0x90}, /* 03 (720x350) */
 {0x64,0x4F,0x88,0x58,0x82,0x00,0x05,0x00,0x53,0x74}, /* 04 (640x480) */
 {0x73,0x63,0x97,0x6C,0x90,0x00,0x05,0x01,0x67,0xAC}  /* 05 (800x600) */   
};

XGI_CH7007TV_TimingVStruct CH7007TVCRT1UNTSC_V[] =
{/* CR06,CR07,CR10,CR11,CR15,CR16,SR0A & CR09(5,7),SR33,SR34,SR3F */
 {0x56,0x3E,0xE8,0x84,0x8F,0x57,0x20,0x01,0xF4,0x54}, /* 00 (640x200,640x400) */
 {0x56,0x3E,0xD0,0x88,0x5D,0x57,0x00,0x01,0xE8,0x64}, /* 01 (640x350) */
 {0x56,0x3E,0xE8,0x84,0x8F,0x57,0x20,0x01,0xF4,0x54}, /* 02 (720x400) */
 {0x56,0x3E,0xD0,0x88,0x5D,0x57,0x00,0x01,0xE8,0x64}, /* 03 (720x350) */
 {0x56,0xBA,0x0B,0x83,0xDF,0x57,0x00,0x00,0x06,0x51}, /* 04 (640x480) */
 {0xEC,0xF0,0x7F,0x8B,0x57,0xED,0xA0,0x00,0x40,0x31}, /* 05 (800x600) */
};

XGI_CH7007TV_TimingVStruct CH7007TVCRT1ONTSC_V[] =
{/* CR06,CR07,CR10,CR11,CR15,CR16,SR0A & CR09(5,7),SR33,SR34,SR3F */
 {0x0B,0x3E,0xC0,0x84,0x8F,0x0C,0x20,0x01,0xE0,0x14}, /* 00 (640x200,640x400) */
 {0x0B,0x3E,0xA7,0x8B,0x5D,0x0C,0x00,0x00,0xD4,0x30}, /* 01 (640x350) */
 {0x0B,0x3E,0xB7,0x83,0x8F,0x0C,0xA0,0x00,0xDC,0x10}, /* 02 (720x400) */
 {0x0B,0x3E,0xA7,0x8B,0x5D,0x0C,0x00,0x00,0xD4,0x30}, /* 03 (720x350) */
 {0x0B,0x3E,0xE8,0x84,0xDF,0x0C,0x00,0x01,0xF2,0x34}, /* 04 (640x480) */
 {0xBA,0xF0,0x77,0x87,0x57,0xBB,0x80,0x00,0x38,0x01}  /* 05 (800x600) */
};

XGI_CH7007TV_TimingVStruct CH7007TVCRT1UPAL_V[] =
{/* CR06,CR07,CR10,CR11,CR15,CR16,SR0A & CR09(5,7),SR33,SR34,SR3F */
 {0x6F,0x3E,0xF9,0x85,0x8F,0x70,0x20,0x00,0xFD,0x18}, /* 00 (640x200,640x400) */
 {0x6F,0x3E,0xCB,0x81,0x5D,0x8F,0x00,0x00,0xE6,0x48}, /* 01 (640x350) */
 {0x6F,0x3E,0xF9,0x85,0x8F,0x70,0x20,0x00,0xFD,0x18}, /* 02 (720x400) */
 {0x6F,0x3E,0xCB,0x81,0x5D,0x8F,0x00,0x00,0xE6,0x48}, /* 03 (720x350) */
 {0x6F,0xBA,0x0F,0x8D,0xDF,0x70,0x00,0x00,0x08,0x79}, /* 04 (640x480) */
 /*{0x42,0xF1,0xA1,0x85,0x57,0x43,0xB0,0x00,0x49,0x79}*/ /* 05 (800x600) */
 {0x42,0xE5,0x1F,0x8C,0x57,0x43,0x90,0x00,0x49,0x79}  /* 05 (800x600) */ /* 07/06/11 Billy */ 
};

XGI_CH7007TV_TimingVStruct CH7007TVCRT1OPAL_V[] =
{/* CR06,CR07,CR10,CR11,CR15,CR16,SR0A & CR09(5,7),SR33,SR34,SR3F */
 {0x6F,0x3E,0xF9,0x85,0x8F,0x70,0x20,0x00,0xFD,0x18}, /* 00 (640x200,640x400) */
 {0x6F,0x3E,0xCB,0x81,0x5D,0x8F,0x00,0x00,0xE6,0x48}, /* 01 (640x350) */
 {0x6F,0x3E,0xF9,0x85,0x8F,0x70,0x20,0x00,0xFD,0x18}, /* 02 (720x400) */
 {0x6F,0x3E,0xCB,0x81,0x5D,0x8F,0x00,0x00,0xE6,0x48}, /* 03 (720x350) */
 {0x6F,0xBA,0x0F,0x8D,0xDF,0x70,0x00,0x00,0x08,0x79}, /* 04 (640x480) */
 {0xEC,0xF0,0x87,0x8C,0x57,0xED,0xA0,0x00,0x44,0x35}  /* 05 (800x600) */
};

/* ---------------------------------------------------------------------- */
/*   Function : SetCH7007TVReg */
/*   Purpose  : Setting TV general registers */
/*   Input    : */
/*   Return   : */
/* ---------------------------------------------------------------------- */
VP_STATUS SetCH7007TVReg(PHW_DEVICE_EXTENSION pHWDE, USHORT ModeNo , USHORT ModeIdIndex , USHORT RefreshRateTableIndex, PVB_DEVICE_INFO  pVBInfo)
{
    UCHAR  temp,tempbx,i;
    UCHAR  CH7007index;
    I2CControl  I2C;
    VP_STATUS   status;
    XGI330_CH7007TVRegDataStruct  *TVPtr = NULL ;

    tempbx = 6 ;
    TVPtr = ( XGI330_CH7007TVRegDataStruct * )XGI_GetTVPtr( tempbx , ModeNo , ModeIdIndex , RefreshRateTableIndex, pVBInfo ) ;

    if (I2COpen(pHWDE, 1, I2C_DVI, &I2C) != NO_ERROR)
    {

        VideoDebugPrint((0, "SetCH7007TVReg : call I2COpen fail !!!\n"));       
        return ERROR_INVALID_PARAMETER;
    }

    /* Get Register index ,and set Register data */
    for(i=0; i<CH7007TVRegNo; i++)
    { 
        CH7007index = CH7007TVIndex[i];     
        temp = TVPtr->Reg[i];                      /* Set CH7007 Data  */ 
        I2C.Command = I2C_COMMAND_WRITE;
        status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, CH7007index, &temp, 1);
        if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
        {
           VideoDebugPrint((0, "SetCH7007TVReg : call I2CAccessBuffer(0xEA) fail !!!\n"));
           return ERROR_INVALID_PARAMETER;
        }
    }

    if (I2COpen(pHWDE, 0, I2C_DVI, &I2C) != NO_ERROR)
    {
        return ERROR_INVALID_PARAMETER;
    }
    return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
/*   Function : TurnOffCH7007 */
/*   Purpose  : Turn Off CH7007 (TV) */
/*   Input    : */
/*   Return   : */
/* ---------------------------------------------------------------------- */
VP_STATUS TurnOffCH7007(PHW_DEVICE_EXTENSION pHWDE)
{
    UCHAR  temp;
    I2CControl  I2C;
    VP_STATUS   status;

    if (I2COpen(pHWDE, 1, I2C_DVI, &I2C) != NO_ERROR)
    {
        VideoDebugPrint((0, "TurnOffCH7007 : call I2COpen fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    temp = 0x0C;
    I2C.Command = I2C_COMMAND_WRITE;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x0E, &temp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "TurnOffCH7007 : call I2CAccessBuffer(0x0E) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }    

    if (I2COpen(pHWDE, 0, I2C_DVI, &I2C) != NO_ERROR)
    {
        return ERROR_INVALID_PARAMETER;
    }
    return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
/*   Function : TurnOnCH7007 */
/*   Purpose  : Turn On CH7007 (TV) */
/*   Input    : */
/*   Return   : */
/* ---------------------------------------------------------------------- */
VP_STATUS TurnOnCH7007(PHW_DEVICE_EXTENSION pHWDE)
{
    UCHAR  temp;
    I2CControl  I2C;
    VP_STATUS   status;

    if (I2COpen(pHWDE, 1, I2C_DVI, &I2C) != NO_ERROR)
    {
        VideoDebugPrint((0, "TurnOnCH7007 : call I2COpen fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    temp = 0x0B;
    I2C.Command = I2C_COMMAND_WRITE;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x0E, &temp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "TurnOnCH7007 : call I2CAccessBuffer(0x0E) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }    

#ifdef  DEBUG_CH7007
int i;
ErrorF("\n dump all ch7007 regs:\n");
ErrorF("0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F\n");
   I2C.Command = I2C_COMMAND_READ;
    for (i = 0; i < 0x40; i++)
    {
        status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, i, &temp, 1);
        if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
        {
            VideoDebugPrint((0, "TurnOffCH7007 : call I2CAccessBuffer(0x0E) fail !!!\n"));
            return ERROR_INVALID_PARAMETER;
        }
        ErrorF(" %3x ", temp);
        if(i % 16 == 15)
            ErrorF("\n");
    }
#endif

    if (I2COpen(pHWDE, 0, I2C_DVI, &I2C) != NO_ERROR)
    {
        return ERROR_INVALID_PARAMETER;
    }
    return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
/*   Function : SetCH7007Regs */
/*   Purpose  : Setting CH7007 general registers */
/*   Input    : */
/*   Return   : */
/* ---------------------------------------------------------------------- */
void SetCH7007Regs(PXGI_HW_DEVICE_INFO HwDeviceExtension, USHORT ModeNo, USHORT ModeIdIndex, USHORT RefreshRateTableIndex, PVB_DEVICE_INFO pVBInfo )
{
    if( !(pVBInfo->VBInfo & SetCRT2ToTV) )
    {
        VideoDebugPrint((0, "SetCH7007Regs: TurnOffCH7007...\n"));
       TurnOffCH7007(HwDeviceExtension->pDevice) ; 

        return;
    }

    XGI_SetCRT2ECLK( ModeNo , ModeIdIndex , RefreshRateTableIndex, pVBInfo ) ;
    if( (pVBInfo->VBInfo & SetCRT2ToTV) )
    {
        VideoDebugPrint((0, "SetCH7007Regs: XGI_ModCRT1Regs...\n"));
        XGI_ModCRT1Regs( ModeNo, ModeIdIndex, RefreshRateTableIndex, HwDeviceExtension, pVBInfo ) ;
        VideoDebugPrint((0, "SetCH7007Regs: SetCH7007TVReg...\n"));
        SetCH7007TVReg( HwDeviceExtension->pDevice, ModeNo, ModeIdIndex, RefreshRateTableIndex, pVBInfo) ;
        VideoDebugPrint((0, "SetCH7007Regs: SetCH7007TVReg done\n"));
    }
    VideoDebugPrint((0, "SetCH7007Regs: done\n"));

}

/* ---------------------------------------------------------------------- */
/*   Function : SenseCHTV */
/*   Purpose  : Sense CH7007 Status */
/*   Input    : */
/*   Return   : */
/* ---------------------------------------------------------------------- */
UCHAR SenseCHTV(PHW_DEVICE_EXTENSION pHWDE)
{
    UCHAR  bltemp ;
    UCHAR  temp ;
    UCHAR  ahtemp = 0 ;

    I2CControl  I2C ;
    VP_STATUS   status ;
 
    if (I2COpen(pHWDE, 1, I2C_DVI, &I2C) != NO_ERROR)
    {
        VideoDebugPrint((0, "TurnOnCH7007 : call I2COpen fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    /* bltemp = GetCH7007( 0x000E ); */        /* Get CHTV default Power Status */
    I2C.Command = I2C_COMMAND_READ;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x0E, &bltemp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "SenseCHTV : call I2CAccessBuffer(0x0E) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    /* SetCH7007( 0x0B0E ); */                 /* Set CH7007 to normal mode */
    temp = 0x0B ;
    I2C.Command = I2C_COMMAND_WRITE;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x0E, &temp, 1);
    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "SenseCHTV : call I2CAccessBuffer(0x0E) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }
  
    DelayUS(500);                              /* DDC2Delay(); */

    /* SetCH7007( 0x0110 ); */                 /* Set Sense bit to 1 */
    temp = 0x01;
    I2C.Command = I2C_COMMAND_WRITE;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x10, &temp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "SenseCHTV : call I2CAccessBuffer(0x10) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }   

    DelayUS(500);                              /* DDC2Delay(); */

    /* SetCH7007( 0x0010 ); */                 /* Set Sense bit to 0 */
    temp = 0x00;
    I2C.Command = I2C_COMMAND_WRITE;

    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x10, &temp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "SenseCHTV : call I2CAccessBuffer(0x10) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    DelayUS(500);                              /* DDC2Delay(); */
  
    /* temp = GetCH7007( 0x0010 ); */          /* Get Sense Status */  
    I2C.Command = I2C_COMMAND_READ;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x10, &temp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "SenseCHTV : call I2CAccessBuffer(0x10) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if(temp == 0x08)                           /* Return the CHTV Sense Status */
    { 
       ahtemp = SVIDEOSense ;
    }       

    /* SetCH7007( bxtemp ); */                 /* Restore CHTV Power Default */
    I2C.Command = I2C_COMMAND_WRITE;
    status = I2CAccessBuffer(pHWDE, &I2C, 0xEA, 0x0E, &bltemp, 1);

    if ((status != NO_ERROR) || (I2C.Status != I2C_STATUS_NOERROR))
    {
        VideoDebugPrint((0, "SenseCHTV : call I2CAccessBuffer(0x0E) fail !!!\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if (I2COpen(pHWDE, 0, I2C_DVI, &I2C) != NO_ERROR)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ahtemp = SVIDEOSense ;  /* currently sense not working */
    return ahtemp;
}
/* ---------------------------------------------------------------------- */
/*   Function : SetTVInfo */
/*   Purpose  : */
/*   Input    : */
/*   Return   : */
/* ---------------------------------------------------------------------- */
void SetTVInfo(PXGI_HW_DEVICE_INFO HwDeviceExtension, PVB_DEVICE_INFO pVBInfo)
{
    UCHAR ahtemp ;

    ahtemp = XGINew_GetReg1( pVBInfo->P3d4 , 0x38 ) ; 
     
    if ( *pVBInfo->pSoftSetting & TVSoftSetting )
    {
      ahtemp |= (TVSoftSetting) ;
    }
    
    ahtemp &= 0x01 ;   
    XGINew_SetReg1(pVBInfo->P3d4, 0x38, ahtemp);    

}

/* ---------------------------------------------------------------------- */
/*   Function : IsCH7007TVMode */
/*   Purpose  : Check CH7007 TV Mode */
/*   Input    : */
/*   Return   : 0 : not support */
/* ---------------------------------------------------------------------- */
#if 0
BOOLEAN IsCH7007TVMode(PVB_DEVICE_INFO pVBInfo)
{
    UCHAR temp ;
    PUCHAR  volatile pVideoMemory = ( PUCHAR )pVBInfo->ROMAddr ;

    if ( ( pVideoMemory[ 0x65 ] & 0x02 ) )	/* For XG21 CH7007, VBCapability in SISINIT.asm 65 */
    {
        temp = XGINew_GetReg1( pVBInfo->P3d4 , 0x30 ) ;

        if (temp & 0x08)                        /* S-Video   */
        { 
            return 1 ;
        }      
        else if (temp & 0x04)                   /* Composite */
        {
            return 1 ;
        } 
        else
        {
            return 0 ;
        }
    }
    else
    {
        return 0 ;
    }

}
#endif
/* ---------------------------------------------------------------------- */
/*   Function : XGI_XG21CheckCH7007TVMode */
/*   Purpose  : Check CH7007 TV Mode */
/*   Input    : */
/*   Return   : FALSE : not support */
/* ---------------------------------------------------------------------- */
BOOLEAN XGI_XG21CheckCH7007TVMode(USHORT ModeNo,USHORT ModeIdIndex, PVB_DEVICE_INFO pVBInfo )
{
    USHORT xres ,
           yres ,
           colordepth ,
           modeflag ,
           resindex ;

    USHORT CH7007TVMAXH = 800 ;
    USHORT CH7007TVMAXV = 600 ;
    
        resindex = XGI_GetResInfo( ModeNo , ModeIdIndex, pVBInfo ) ;
        if ( ModeNo <= 0x13 )
        {
            xres = pVBInfo->StResInfo[ resindex ].HTotal ;
            yres = pVBInfo->StResInfo[ resindex ].VTotal ;
            modeflag = pVBInfo->SModeIDTable[ModeIdIndex].St_ModeFlag;    /* si+St_ResInfo */
        }
        else
        {
            xres = pVBInfo->ModeResInfo[ resindex ].HTotal ;              /* xres->ax */
            yres = pVBInfo->ModeResInfo[ resindex ].VTotal ;              /* yres->bx */
            modeflag = pVBInfo->EModeIDTable[ ModeIdIndex].Ext_ModeFlag ; /* si+St_ModeFlag */
        }

        if ( !( modeflag & Charx8Dot ) )
        {
            xres /= 9;
            xres *= 8;
        }

        if ( ModeNo > 0x13 )
        {
            if ( ( ModeNo>0x13 ) && ( modeflag & HalfDCLK ) )
            {
              xres *=  2 ;
            }
            if ( ( ModeNo>0x13 ) && ( modeflag & DoubleScanMode ) )
            {
              yres *=  2 ;
            }
        }
        if ( xres > CH7007TVMAXH )
        {
           return FALSE;
        }

        if ( yres > CH7007TVMAXV )
        {
           return FALSE;
        }
    

    return TRUE;
}
