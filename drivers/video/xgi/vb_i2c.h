#ifndef _VBI2C_
#define _VBI2C_
#include "vgatypes.h"
#ifdef LINUX_KERNEL
#include <linux/delay.h> /* udelay */
#endif 
#ifndef u32
#define u32 unsigned long
#define u8 unsigned long
#endif
#ifndef DelayUS
#define DelayUS(p) udelay(p)
#endif
#define IND_CR48_GPIO_REG_I   0x48
#define IND_CR4A_GPIO_REG_III 0x4A

typedef struct _I2CControl {
        ULONG Command;          /*  I2C_COMMAND_* */
        u32   dwCookie;         /* Context identifier returned on Open */
        u8    Data;             /* Data to write, or returned UCHAR */
        u8    Reserved[3];      /* Filler */
        ULONG Flags;            /*  I2C_FLAGS_* */
        ULONG Status;           /* I2C_STATUS_*  */
        ULONG ClockRate;        /* Bus clockrate in Hz. */
} I2CControl, *PI2CControl;

typedef struct _I2CContext
{
    u32 dwI2CPortAcquired;            /* port busy between start and stop */
    u32 dwCookie;                  /* cookie image for this instance */
    u32 dwCurCookie;                  /* cookie of current I2C channel owner */
} I2C_CONTEXT, *PI2C_CONTEXT;


typedef struct _EDID_V1_  {
    ULONG       ulHeader0;
    ULONG       ulHeader1;
    struct {
            UCHAR       LoUCHAR;
            UCHAR       HiUCHAR;
    } IDManufactureName;
    USHORT      IDProductCode;
    ULONG       IDSerialNumber;
    UCHAR       WeekOfManufacture;
    UCHAR       YearOfManufacture;
    UCHAR       bEDIDVersion;           /* should be 1 */
    UCHAR       bEDIDRevision;          /* should be 0~3 */
    UCHAR       bVideoInput;
    UCHAR       bMaxHzImageSize;        /* cm */
    UCHAR       bMaxVtImageSize;        /* cm */
    UCHAR       bGamma;
    UCHAR       bFeatureSupport;
    UCHAR       bColorCharacteristics[10];
    UCHAR       bEstablishedTiming[3];
    USHORT      usStandardTiming[8];
    union  {
        struct {
            USHORT  usPixelClock;
            UCHAR   bHzActive;
            UCHAR   bHzBlank;
            UCHAR   bHzActiveBlank;
            UCHAR   bVtActive;
            UCHAR   bVtBlank;
            UCHAR   bVtActiveBlank;
            UCHAR   bHSyncOffset;
            UCHAR   bHSyncWidth;
            UCHAR   bVSyncOffsetWidth;
            UCHAR   bHzVtSyncOffsetWidth;
            UCHAR   bHzImageSize;           /* mm */
            UCHAR   bVtImageSize;           /* mm */
            UCHAR   bHzVtImageSize;         /* mm */
            UCHAR   bHzBorder;
            UCHAR   bVtBorder;
            UCHAR   bFlags;
        } DetailedTiming;
        struct {
            USHORT      usReserved0;
            UCHAR       bReserved1;
            UCHAR       bTag;
            UCHAR       bReserved2;
            UCHAR       bData[13];
        } MonitorDescriptor;
    } Descriptor[4];
    UCHAR       bExtensionFlag;
    UCHAR       bChecksum;
} EDID_V1, *PEDID_V1;

typedef struct _XGI_I2C_CONTROL
{
     ULONG                   I2CDelay;        /* 100ns units*/
} XGI_I2C_CONTROL, *PXGI_I2C_CONTROL;

/**/
extern  char I2CAccessBuffer(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl I2CCntl, ULONG DevAddr, ULONG Offset, PUCHAR pBuffer, ULONG ulSize);
/**/
extern  char vGetEDIDExtensionBlocks(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2C, PUCHAR pjBuffer, ULONG ulBufferSize);
extern  char vGetEnhancedEDIDBlock(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2C, ULONG ulBlockID, ULONG ulBlockTag, PUCHAR pjBuffer, ULONG ulBufferSize);


extern  char I2COpen (PXGI_HW_DEVICE_INFO  pHWDE,ULONG ulI2CEnable, ULONG ulChannelID, PI2CControl pI2CControl);
extern  char I2CAccess(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl);
extern  BOOLEAN I2CNull( PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
extern  BOOLEAN I2CRead(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl);
extern  BOOLEAN I2CWrite(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
extern  BOOLEAN ResetI2C(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
extern  BOOLEAN I2CRead(PXGI_HW_DEVICE_INFO pHWDE,PI2CControl pI2CControl);
extern  BOOLEAN I2CWrite(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
extern  BOOLEAN ResetI2C(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);

#endif
