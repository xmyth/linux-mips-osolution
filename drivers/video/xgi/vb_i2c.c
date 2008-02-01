#include "osdef.h"
#include "vgatypes.h"
#include "vb_util.h"
#include "vb_def.h"

#ifdef WIN2000
#include <dderror.h>
#include <devioctl.h>
#include <miniport.h>
#include <ntddvdeo.h>
#include <video.h>
#include "xgiv.h"
#include "dd_i2c.h"
#include "tools.h"
#endif /* WIN2000 */

#ifdef LINUX_XF86
#include "xf86.h"
#include "xf86PciInfo.h"
#include "xgi.h"
#include "xgi_regs.h"
#include "vb_i2c.h"
#endif

#ifdef LINUX_KERNEL
#include <linux/version.h>
#include <asm/io.h>
#include <linux/types.h>
#include "vb_i2c.h"
#include <linux/delay.h> /* udelay */
#include "XGIfb.h"

#endif



/*char I2CAccessBuffer(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl I2CCntl, ULONG DevAddr, ULONG Offset, PUCHAR pBuffer, ULONG ulSize); */
char vGetEDIDExtensionBlocks(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2C, PUCHAR pjBuffer, ULONG ulBufferSize);
char vGetEnhancedEDIDBlock(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2C, ULONG ulBlockID, ULONG ulBlockTag, PUCHAR pjBuffer, ULONG ulBufferSize);

char I2COpen (PXGI_HW_DEVICE_INFO  pHWDE,ULONG ulI2CEnable, ULONG ulChannelID, PI2CControl pI2CControl);
char I2CAccess(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl);
BOOLEAN I2CNull( PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
BOOLEAN I2CRead(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl);
BOOLEAN I2CWrite(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
BOOLEAN ResetI2C(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
BOOLEAN I2CRead(PXGI_HW_DEVICE_INFO pHWDE,PI2CControl pI2CControl);
BOOLEAN I2CWrite(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
BOOLEAN ResetI2C(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl);
BOOLEAN Ack (PXGI_HW_DEVICE_INFO pHWDE,  bool fPut);
BOOLEAN NoAck(PXGI_HW_DEVICE_INFO pHWDE);
BOOLEAN Start( PXGI_HW_DEVICE_INFO pHWDE);
BOOLEAN Stop(PXGI_HW_DEVICE_INFO pHWDE);
BOOLEAN WriteUCHARI2C(PXGI_HW_DEVICE_INFO pHWDE,  UCHAR cData);
BOOLEAN ReadUCHARI2C(PXGI_HW_DEVICE_INFO pHWDE,  PUCHAR pBuffer);
UCHAR ReverseUCHAR(UCHAR data);
VOID vWriteClockLineDVI(PXGI_HW_DEVICE_INFO pHWDE, UCHAR data);
VOID vWriteDataLineDVI(PXGI_HW_DEVICE_INFO pHWDE, UCHAR data);
BOOLEAN bReadClockLineDVI(PXGI_HW_DEVICE_INFO pHWDE);
BOOLEAN bReadDataLineDVI(PXGI_HW_DEVICE_INFO pHWDE);
BOOLEAN bEDIDCheckSum(PUCHAR pjEDIDBuf,ULONG ulBufSize);
/*
void DelayUS(ULONG MicroSeconds)
{
	udelay(MicroSeconds);
}
*/

typedef enum _I2C_ACCESS_CMD
{
    WRITE = 0,
    READ
} I2C_ACCESS_CMD;

/* For XG21 */

#define ENABLE_GPIOA          0x01
#define ENABLE_GPIOB          0x02
#define ENABLE_GPIOC          0x04
VOID
EnableGPIOA(
PUCHAR pjIOPort, I2C_ACCESS_CMD CmdType)
{
    UCHAR ujCR4A = XGINew_GetReg1(pjIOPort, IND_CR4A_GPIO_REG_III);

    if (CmdType == WRITE)
    {
        ujCR4A &= ~ENABLE_GPIOA;
    }
    else
    {
        ujCR4A |= ENABLE_GPIOA;
    }

    XGINew_SetReg1(pjIOPort, IND_CR4A_GPIO_REG_III, ujCR4A);
}

VOID
EnableGPIOB(
PUCHAR pjIOPort, I2C_ACCESS_CMD CmdType)
{
    UCHAR ujCR4A = XGINew_GetReg1(pjIOPort, IND_CR4A_GPIO_REG_III);

    if (CmdType == WRITE)
    {
        ujCR4A &= ~ENABLE_GPIOB;
    }
    else
    {
        ujCR4A |= ENABLE_GPIOB;
    }

    XGINew_SetReg1(pjIOPort, IND_CR4A_GPIO_REG_III, ujCR4A);
}

VOID
EnableGPIOC(
PUCHAR pjIOPort, I2C_ACCESS_CMD CmdType)
{
    UCHAR ujCR4A = XGINew_GetReg1(pjIOPort, IND_CR4A_GPIO_REG_III);

    if (CmdType == WRITE)
    {
        ujCR4A &= ~ENABLE_GPIOC;
    }
    else
    {
        ujCR4A |= ENABLE_GPIOC;
    }

    XGINew_SetReg1(pjIOPort, IND_CR4A_GPIO_REG_III, ujCR4A);
}




/**
*  Function: getGPIORWTranser()
*
*  Description: This function is used based on Z9. Because of wrongly wired deployment by HW
*               the CR4A and CR48 for GPIO pins have reverse sequence. For example,
*               D[7:0] for read function is ordered as GPIOA to GPIOH, but D[7:0] for read
*               is as GPIOH~GPIOA
*/
UCHAR
getGPIORWTransfer(
UCHAR ujDate)
{
    UCHAR  ujRet = 0;
    UCHAR  i = 0;

    for (i=0; i<8; i++)
	{
    	ujRet = ujRet << 1;
		ujRet |= GETBITS(ujDate >> i, 0:0);
    }

	return ujRet;
}



char I2CAccessBuffer(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl, ULONG ulDevAddr,
    ULONG ulOffset, PUCHAR pBuffer,ULONG ulSize)
{

    I2CControl  I2C;
    ULONG       i;

    if ((ulSize == 0) || (pBuffer == NULL))  {
        return -1;
    }
    if (ulDevAddr & 1)  {
        return -1;
    }
    if ((ulDevAddr > 0xFF) || (ulOffset > 0xFF))  {
        return -1;
    }

    I2C.Command = pI2CControl->Command;
    I2C.dwCookie = pI2CControl->dwCookie;
    I2C.Data = pI2CControl->Data;
    I2C. Flags = pI2CControl->Flags;
    I2C.Status = pI2CControl->Status;
    I2C.ClockRate = pI2CControl->ClockRate;
    switch (pI2CControl->Command)  {
    case I2C_COMMAND_READ:
        /* Reset I2C Bus */
        I2C.Command = I2C_COMMAND_RESET;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Write Device Address */
        I2C.Command = I2C_COMMAND_WRITE;
        I2C.Flags = I2C_FLAGS_START | I2C_FLAGS_ACK;
        I2C.Data = (UCHAR)ulDevAddr;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Write Register Offset */
        I2C.Command = I2C_COMMAND_WRITE;
        I2C.Flags = I2C_FLAGS_ACK | I2C_FLAGS_STOP;
        I2C.Data = (UCHAR)ulOffset;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Write Device Read Address */
        I2C.Command = I2C_COMMAND_WRITE;
        I2C.Flags = I2C_FLAGS_START | I2C_FLAGS_ACK;
        I2C.Data = (UCHAR)ulDevAddr + 1;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Read Data */
        for (i=0; i< ulSize; i++)  {
            I2C.Command = I2C_COMMAND_READ;
            I2C.Flags = I2C_FLAGS_ACK;
            if (i == ulSize - 1)  {     /* Read Last UCHAR */
                I2C.Flags |= I2C_FLAGS_STOP;
            }
            I2CAccess(pHWDE, &I2C);
            if (I2C.Status != I2C_STATUS_NOERROR)  {
                pI2CControl->Status = I2C.Status;
                break;
            }
            *pBuffer = I2C.Data;
            pBuffer++;
        }
        pI2CControl->Status = I2C.Status;
        break;

    case I2C_COMMAND_WRITE:
        /* Reset I2C Bus */
        I2C.Command = I2C_COMMAND_RESET;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Write Device Address */
        I2C.Command = I2C_COMMAND_WRITE;
        I2C.Flags = I2C_FLAGS_START | I2C_FLAGS_ACK;
        I2C.Data = (UCHAR)ulDevAddr;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Write Register Offset */
        I2C.Command = I2C_COMMAND_WRITE;
        I2C.Flags = I2C_FLAGS_ACK;
        I2C.Data = (UCHAR)ulOffset;
        I2CAccess(pHWDE, &I2C);
        if (I2C.Status != I2C_STATUS_NOERROR)  {
            pI2CControl->Status = I2C.Status;
            break;
        }

        /* Write Data */
        for (i=0; i< ulSize; i++)  {
            I2C.Command = I2C_COMMAND_WRITE;
            I2C.Flags = I2C_FLAGS_ACK;
            if (i == ulSize - 1)  {     /* Read Last UCHAR */
                I2C.Flags |= I2C_FLAGS_STOP;
            }
            I2C.Data = *pBuffer;
            I2CAccess(pHWDE, &I2C);
            if (I2C.Status != I2C_STATUS_NOERROR)  {
                pI2CControl->Status = I2C.Status;
                break;
            }
            pBuffer++;
        }
        pI2CControl->Status = I2C.Status;
        break;
    }

    if (pI2CControl->Status == I2C_STATUS_NOERROR)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}


/*************************************************************************
// char vGetEDIDExtensionBlocks
//
// Routine Description:
//      Reads the extension part behind a 128KB block of EDID which is in 1.3
//      format. The extension part can be distributed into 128KB-blocks.
//
// Arguments:
//      pHWDE           -   Hardware extension object pointer
//      pI2C            -   I2C pointer
//      pjBuffer        -   EDID Buffer
//      ulBufferSize    -   Buffer size
//
//
// Return Value:
//      Status of returning EDID.
//      NO_ERROR                -   success
//      ERROR_INVALID_PARAMETER -   failed
//
****************************************************************************/
char vGetEDIDExtensionBlocks(
    PXGI_HW_DEVICE_INFO  pHWDE,
    PI2CControl pI2C,
    PUCHAR      pjBuffer,
    ULONG       ulBufferSize)
{
    char   status;
    ULONG       ulBlockTag;
    ULONG       i;
    PUCHAR      pBlockMap;


    if ((ulBufferSize < 128) || (pjBuffer == NULL))
    {
        return -1;
    }

    pI2C->Command = I2C_COMMAND_READ;
    status = I2CAccessBuffer(pHWDE, pI2C, 0xA0, 128, pjBuffer, 128);
    if ((status != 0) || (pI2C->Status != I2C_STATUS_NOERROR))
    {
        return status;
    }

    if (bEDIDCheckSum(pjBuffer, 128) != 0)  {
        return -1;
    }

    if (*pjBuffer == 0xF0)
    {   /* A Block Map, we should read other extension blocks*/
        pBlockMap = pjBuffer;
        for (i=1; i<=126; i++)
        {
            ulBlockTag = *(pBlockMap + i);
            if (ulBlockTag)
            {
                pjBuffer += 128;
                ulBufferSize -= 128;
                status = vGetEnhancedEDIDBlock(pHWDE, pI2C, i+1, ulBlockTag, pjBuffer, ulBufferSize);
                if ((status != 0) || (pI2C->Status != I2C_STATUS_NOERROR))
                {
                    return -1;
                }
            }
            else
            {
                if (i > 1)
                {
                    return 0;  /* All Extension blocks must be sequential, no holes allowed. (VESA E-EDID)*/
                }
                else
                {
                    return -1;
                }
            }
        }
        /* We should read block # 128 */
        pjBuffer += 128;
        ulBufferSize -= 128;
        ulBlockTag = 0xF0;
        status = vGetEnhancedEDIDBlock(pHWDE, pI2C, 128, ulBlockTag, pjBuffer, ulBufferSize);
        if ((status != 0) || (pI2C->Status != I2C_STATUS_NOERROR))
        {
            return 0;    /* Monitor may has only 128 blocks (0~127) */
        }

        pBlockMap = pjBuffer;
        for (i=1; i<=126; i++)
        {   /* Read Block # 128 ~ 254 */
            ulBlockTag = *(pBlockMap + i);
            if (ulBlockTag)
            {
                pjBuffer += 128;
                ulBufferSize -= 128;
                status = vGetEnhancedEDIDBlock(pHWDE, pI2C, i+128, ulBlockTag, pjBuffer, ulBufferSize);
                if ((status != 0) || (pI2C->Status != I2C_STATUS_NOERROR))
                {
                    return -1;
                }
            }
            else
            {
                if (i > 1)
                {
                    return 0;  /* All Extension blocks must be sequential, no holes allowed. (VESA E-EDID) */
                }
                else
                {
                    return -1;
                }
            }
        }
    }

    return 0;
}

/*************************************************************************
// char vGetEnhancedEDIDBlock
//
// Routine Description:
//  Get the EDID which is in Enhanced-EDID format via I2C. The parse-in block
//  tag(ulBlockTag) and the first UCHAR of the retrieved buffer should be identical.
//  Returns error when they are not, otherwise return NO_ERROR.
//
// Arguments:
//      pHWDE           -   Hardware extension object pointer
//      pI2C            -   I2C pointer
//      ulBlockID       -   Block ID
//      ulBlockTag      -   Block Tag
//      pjBuffer        -   EDID Buffer
//      ulBufferSize    -   Buffer size
//
//
// Return Value:
//      Status of returning EDID.
//      NO_ERROR                -   success
//      ERROR_INVALID_PARAMETER -   failed
//
****************************************************************************/
char vGetEnhancedEDIDBlock(
    PXGI_HW_DEVICE_INFO  pHWDE,
    PI2CControl pI2C,
    ULONG       ulBlockID,
    ULONG       ulBlockTag,
    PUCHAR      pjBuffer,
    ULONG       ulBufferSize)
{
    ULONG       ulOffset, SegmentID;
    char   status;

    if ((ulBufferSize < 128) || (pjBuffer == NULL))
    {
        return -1;
    }

    SegmentID = ulBlockID / 2;
    ulOffset = (ulBlockID % 2) * 128;

    pI2C->Command = I2C_COMMAND_WRITE;
    status = I2CAccessBuffer(pHWDE, pI2C, 0x60, 0, (PUCHAR)&SegmentID, 1);
    if ((status == NO_ERROR) && (pI2C->Status == I2C_STATUS_NOERROR))
    {
        pI2C->Command = I2C_COMMAND_READ;
        status = I2CAccessBuffer(pHWDE, pI2C, 0xA0, ulOffset, pjBuffer, 128);
        if ((status == 0) && (pI2C->Status == I2C_STATUS_NOERROR))
        {
            if (*pjBuffer != (UCHAR)ulBlockTag)
            {
                return -1;
            }

            if (bEDIDCheckSum(pjBuffer, 128) != 0)
            {
                return -1;
            }
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
          return ERROR_INVALID_PARAMETER;
    }


    return NO_ERROR;
}
char I2COpen (PXGI_HW_DEVICE_INFO  pHWDE, ULONG ulI2CEnable, ULONG ulChannelID, PI2CControl pI2CControl)
{
/*
//    printk("\nI2COpen(%d) : Channel ID = %d\n", ulI2CEnable, ulChannelID);
    // we need to determine the Context area for each command we receive
    // i.e. which hardware I2C bus is the command for.
    // this is unimplemented here!
*/
    if (ulChannelID >= MAX_I2C_CHANNEL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    if (ulI2CEnable)    /* Open I2C Port */
    {
/*        // verify clock rate. If you cannot implement the given rate,
        // enable a lower clock rate closest to the request clock rate.
        //
        // put a better check here if your device can only do discrete
        // clock rate values.
*/
        if (pI2CControl->ClockRate > I2C_MAX_CLOCK_RATE)
        {
            pI2CControl->ClockRate = I2C_MAX_CLOCK_RATE;
        }
        pI2CControl->Status = I2C_STATUS_NOERROR;
    }
    else    /* Close I2C Port*/
    {
        /* Set Acquired state to FALSE */

        pI2CControl->dwCookie = 0;
        /* Set status */
        pI2CControl->Status = I2C_STATUS_NOERROR;
    }



    return 0;
}
/* end of I2COpen */
char I2CAccess(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl)
{
    ULONG                       ulChannel = pI2CControl->dwCookie % MAX_I2C_CHANNEL;


    if (pI2CControl->ClockRate > I2C_MAX_CLOCK_RATE)
    {
        pI2CControl->ClockRate = I2C_MAX_CLOCK_RATE;
    }
    if (pI2CControl->ClockRate == 0)  {
        pI2CControl->ClockRate = 20000;
    }
    pHWDE->I2CDelay = (1000000 / pI2CControl->ClockRate) * 10; /* in 100ns */

 pHWDE->I2CDelay = 100;
    switch (pI2CControl->Command)
    {
        /* Issue a STOP or START without a READ or WRITE Command */
        case I2C_COMMAND_NULL:
            if (I2CNull(pHWDE, pI2CControl) == FALSE)  break;
/*          if (pI2CControl->Flags & I2C_FLAGS_STOP)  {
                pI2CContext->dwI2CPortAcquired = FALSE;
            }
*/          break;

        /* READ or WRITE Command */
        case I2C_COMMAND_READ:
            if (I2CRead(pHWDE, pI2CControl) == FALSE)  break;
/*          if (pI2CControl->Flags & I2C_FLAGS_STOP)  {
                pI2CContext->dwI2CPortAcquired = FALSE;
            }
*/          break;

        case I2C_COMMAND_WRITE:
            if (I2CWrite(pHWDE, pI2CControl) == FALSE)  break;
/*          if (pI2CControl->Flags & I2C_FLAGS_STOP)  {
                pI2CContext->dwI2CPortAcquired = FALSE;
            }
*/          break;

        case I2C_COMMAND_STATUS:
         pI2CControl->Status = I2C_STATUS_NOERROR;
            break;

        case I2C_COMMAND_RESET:
            /* Reset I2C bus */
            if (ResetI2C(pHWDE, pI2CControl) == FALSE)  break;
            break;

        default:
            /* Invalid Command */
            return ERROR_INVALID_PARAMETER;
    }
    

/*    printk("\nI2CAccess(): I2C Cmd = 0x%X I2C Flags = 0x%X I2C Status = 0x%X I2C Data = 0x%X", pI2CControl->Command, pI2CControl->Flags, pI2CControl->Status, pI2CControl->Data); */

    return NO_ERROR;
}


/*^^*
 * Function:    I2CNull
 *
 * Purpose:             To complete an I2C instruction.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated
 *                                          with card being accessed
 *                                I2CCntl : PI2CControl, pointer to I2C control structure
 *
 * Outputs:             void.
 *^^*/
BOOLEAN I2CNull( PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl)
{
    pI2CControl->Status = I2C_STATUS_ERROR;

/*///// Issue STOP - START, if I2C_FLAGS_DATACHAINING ////////*/
    if (pI2CControl->Flags & I2C_FLAGS_DATACHAINING)
    {
        if (Stop(pHWDE) == FALSE)  return FALSE;
        if (Start(pHWDE) == FALSE)  return FALSE;
    }

/*///// Issue START ////////*/
    if (pI2CControl->Flags & I2C_FLAGS_START)
    {
        if (Start(pHWDE) == FALSE)  return FALSE;
    }

/*////// Issue STOP /////////*/
    if (pI2CControl->Flags & I2C_FLAGS_STOP)
    {
        if (Stop(pHWDE) == FALSE)  return FALSE;
    }

    pI2CControl->Status = I2C_STATUS_NOERROR;
    return TRUE;
}/* end of I2CNull()*/

/*^^*
 * Function:    I2CRead
 *
 * Purpose:             To complete an I2C instruction.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated
 *                                          with card being accessed
 *                                I2CCntl : PI2CControl, pointer to I2C control structure
 *
 * Outputs:             void.
 *^^*/
BOOLEAN I2CRead(PXGI_HW_DEVICE_INFO pHWDE, PI2CControl pI2CControl)
{
    pI2CControl->Status = I2C_STATUS_ERROR;

/*///// Issue STOP - START, if I2C_FLAGS_DATACHAINING ////////*/
    if (pI2CControl->Flags & I2C_FLAGS_DATACHAINING)
    {
        if (Stop(pHWDE) == FALSE)  return FALSE;
        if (Start(pHWDE) == FALSE)  return FALSE;
    }

/*///// Issue START ////////*/
    if (pI2CControl->Flags & I2C_FLAGS_START)
    {
        if (Start(pHWDE) == FALSE)  return FALSE;
    }

/*///// Read Command ///////*/
    if (ReadUCHARI2C(pHWDE, &pI2CControl->Data) == FALSE)
        return FALSE;

    if (pI2CControl->Flags & I2C_FLAGS_STOP)  {
        if (NoAck(pHWDE) == FALSE)  return FALSE;
    }
    else  {  /* Should we issue ACK*/
        if (pI2CControl->Flags & I2C_FLAGS_ACK)  {
            if (Ack(pHWDE, SEND_ACK) == FALSE)  return FALSE;
        }
    }

/*////// Issue STOP /////////*/
    if (pI2CControl->Flags & I2C_FLAGS_STOP)
    {
        if (Stop(pHWDE) == FALSE)  return FALSE;
    }

    pI2CControl->Status = I2C_STATUS_NOERROR;
    return TRUE;
} /* end of I2CRead() */

/*^^*
 * Function:    I2CWrite
 *
 * Purpose:             To complete an I2C instruction.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated
 *                                          with card being accessed
 *                                I2CCntl : PI2CControl, pointer to I2C control structure
 *
 * Outputs:             void.
 *^^*/
BOOLEAN I2CWrite(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl)
{
    pI2CControl->Status = I2C_STATUS_ERROR;

/*///// Issue STOP - START, if I2C_FLAGS_DATACHAINING ////////*/
    if (pI2CControl->Flags & I2C_FLAGS_DATACHAINING)
    {
        if (Stop(pHWDE) == FALSE) 
        {
             
             return FALSE;
        }
        if (Start(pHWDE) == FALSE)  
        {
            
 
            return FALSE;
        }
    }

/*///// Issue START ////////*/
    if (pI2CControl->Flags & I2C_FLAGS_START)
    {
        if (Start(pHWDE) == FALSE) 
         {
            return FALSE;
          }
    }

/*///// Write Command ///////*/
    if (WriteUCHARI2C(pHWDE, pI2CControl->Data) == FALSE)
    {
   
       return FALSE;
     }

    if (pI2CControl->Flags & I2C_FLAGS_ACK)  {
        if (Ack(pHWDE, RECV_ACK) == FALSE) 
        {
            return FALSE;
        }
    }

/*////// Issue STOP /////////*/
    if (pI2CControl->Flags & I2C_FLAGS_STOP)
    {
        if (Stop(pHWDE) == FALSE) 
        {
           return FALSE;
        }
    }

    pI2CControl->Status = I2C_STATUS_NOERROR;
    return TRUE;
} /* end of I2CWrite() */

/*^^*
 * Function:    ResetI2C
 *
 * Purpose:             To reset I2CBus
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated with
 *                                          lpI2C_ContextData being accessed
 *
 * Outputs:             UCHAR, the data that was read.
 *^^*/
BOOLEAN ResetI2C(PXGI_HW_DEVICE_INFO pHWDE,  PI2CControl pI2CControl)
{

    if (Stop(pHWDE) == TRUE)  {
        pI2CControl->Status = I2C_STATUS_NOERROR;
        return TRUE;
    }
    else  {
        pI2CControl->Status = I2C_STATUS_ERROR;
        return FALSE;
    }
} /* ResetI2C() */




/*^^*
 * Function:    Ack
 *
 * Purpose:             To ask the I2C bus for an acknowledge.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated with
 *                      lpI2C_ContextData being accessed
 *
 * Outputs:             char, ack status.
 *^^*/
BOOLEAN Ack (PXGI_HW_DEVICE_INFO pHWDE,  bool fPut)
{
    BOOLEAN     status = FALSE;
    ULONG       i, delay, delay2;
    ULONG       ack;

    delay = pHWDE->I2CDelay / 10 / 2;
    if (fPut == SEND_ACK)   /* Send Ack into I2C bus */
    {
        vWriteDataLineDVI(pHWDE, LODAT);
        DelayUS(delay);

        vWriteClockLineDVI(pHWDE, HICLK);
        DelayUS(delay);
        if (bReadClockLineDVI(pHWDE) != HICLK)  {
            i = 0;
            delay2 = delay * 2;
            do {
                vWriteClockLineDVI(pHWDE, HICLK);
                DelayUS(delay2);
                if (bReadClockLineDVI(pHWDE) == HICLK)  break;
                i++;
                delay2 *= 2;
                if (i >= I2C_RETRY_COUNT)  return FALSE;
            } while (1);
        }

        vWriteClockLineDVI(pHWDE, LOCLK);
        DelayUS(delay);

        return TRUE;
    }
    else
    {
        /* Receive Ack from I2C bus */
        vWriteDataLineDVI(pHWDE, HIDAT);
        DelayUS(delay);
        ack = bReadDataLineDVI(pHWDE);

        vWriteClockLineDVI(pHWDE, HICLK);
        DelayUS(delay);
        if (bReadClockLineDVI(pHWDE) != HICLK)  {
            i = 0;
            delay2 = delay * 2;
            do {
                vWriteClockLineDVI(pHWDE, HICLK);
                DelayUS(delay2);
                if (bReadClockLineDVI(pHWDE) == HICLK)  break;
                i++;
                delay2 *= 2;
                if (i >= I2C_RETRY_COUNT)  return FALSE;
            } while (1);
        }

        status = bReadDataLineDVI(pHWDE);

        vWriteClockLineDVI(pHWDE, LOCLK);
        DelayUS(delay);

        if (status != LODAT)  {
            if (ack == LODAT)  {
                status = LODAT;
            }

            else  {
               
            }

        }
        return (BOOLEAN)(status == LODAT);
    }
}/* end of Ack() */


BOOLEAN NoAck(PXGI_HW_DEVICE_INFO pHWDE)
{
    ULONG       i, delay, delay2;

    delay = pHWDE->I2CDelay / 10 / 2;

    vWriteDataLineDVI(pHWDE, HIDAT);
    DelayUS(delay);

    vWriteClockLineDVI(pHWDE, HICLK);
    DelayUS(delay);
    if (bReadClockLineDVI(pHWDE) != HICLK)  {
        delay2 = delay * 2;
        i = 0;
        do  {
            vWriteClockLineDVI(pHWDE, HICLK);
            DelayUS(delay2);
            if (bReadClockLineDVI(pHWDE) == HICLK)  break;
            i++;
            delay2 *= 2;
            if (i >= I2C_RETRY_COUNT) return FALSE;
        } while (1);
    }

    vWriteClockLineDVI(pHWDE, LOCLK);
    DelayUS(delay);

    return TRUE;
}


/*^^*
 * Function:    Start
 *
 * Purpose:             To start a transfer on the I2C bus.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated with
 *                      lpI2C_ContextData being accessed
 *
 * Outputs:             void.
 *^^*/
BOOLEAN Start( PXGI_HW_DEVICE_INFO pHWDE)
{
    ULONG       i, delay, delay2;

    delay = pHWDE->I2CDelay / 10 / 2;

    vWriteDataLineDVI(pHWDE, HIDAT);
    DelayUS(delay);
    if (bReadDataLineDVI(pHWDE) != HIDAT)  {
        delay2 = delay * 2;
        i = 0;
        do  {
            vWriteDataLineDVI(pHWDE, HIDAT);
            DelayUS(delay2);
            if (bReadDataLineDVI(pHWDE) == HIDAT)  break;
            i++;
            delay2 *= 2;
            if (i >= I2C_RETRY_COUNT)  return FALSE;
        } while (1);
    }

    vWriteClockLineDVI(pHWDE, HICLK);
    DelayUS(delay);
    if (bReadClockLineDVI(pHWDE) != HICLK)  {
        delay2 = delay * 2;
        i = 0;
        do  {
            vWriteClockLineDVI(pHWDE, HICLK);
            DelayUS(delay2);
            if (bReadClockLineDVI(pHWDE) == HICLK)  break;
            i++;
            delay2 *= 2;
            if (i >= I2C_RETRY_COUNT) return FALSE;
        } while (1);
    }

    vWriteDataLineDVI(pHWDE, LODAT);
    DelayUS(delay);

    vWriteClockLineDVI(pHWDE, LOCLK);
    DelayUS(delay);

    return TRUE;
}/* end of Start */

/*^^*
 * Function:    Stop
 *
 * Purpose:             To stop a transfer on the I2C bus.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated with
 *                                          lpI2C_ContextData being accessed
 *
 * Outputs:             void.
 *^^*/
BOOLEAN Stop(PXGI_HW_DEVICE_INFO pHWDE)
{
    ULONG       i, delay, delay2;

    delay = pHWDE->I2CDelay / 10 / 2;

    vWriteDataLineDVI(pHWDE, LODAT);
    DelayUS(delay);

    vWriteClockLineDVI(pHWDE, HICLK);
    DelayUS(delay);
    if (bReadClockLineDVI(pHWDE) != HICLK)  {
        i = 0;
        delay2 = delay * 2;
        do {
            vWriteClockLineDVI(pHWDE, HICLK);
            DelayUS(delay2);
            if (bReadClockLineDVI(pHWDE) == HICLK)  break;
            i++;
            delay2 *= 2;
            if (i >= I2C_RETRY_COUNT)  return FALSE;
        } while (1);
    }

    vWriteDataLineDVI(pHWDE, HIDAT);
    DelayUS(delay);

    return (BOOLEAN)(bReadDataLineDVI(pHWDE) == HIDAT);
}/* end of Stop*/

/*^^*
 * Function:    WriteUCHARI2C
 *
 * Purpose:             To write a UCHAR of data to the I2C bus.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated with
 *                                          lpI2C_ContextData being accessed
 *                                   cData: UCHAR, the data to write
 *
 * Outputs:             void.
 *^^*/
BOOLEAN WriteUCHARI2C(PXGI_HW_DEVICE_INFO pHWDE,  UCHAR cData)
{
    ULONG  i, j, delay, delay2;

    cData = ReverseUCHAR(cData);

    delay = pHWDE->I2CDelay / 10 / 2;

    for (j=0; j<8; j++, cData>>=1)
    {
        vWriteDataLineDVI(pHWDE, cData);
        DelayUS(delay);

        vWriteClockLineDVI(pHWDE, HICLK);
        DelayUS(delay);
        if (bReadClockLineDVI(pHWDE) != HICLK)  {
            i = 0;
            delay2 = delay * 2;
            do {
                vWriteClockLineDVI(pHWDE, HICLK);
                DelayUS(delay2);
                if (bReadClockLineDVI(pHWDE) == HICLK)  break;
                i++;
                delay2 *= 2;
                if (i >= I2C_RETRY_COUNT)  return FALSE;
            } while (1);
        }

        vWriteClockLineDVI(pHWDE, LOCLK);
        DelayUS(delay);
    }
    return TRUE;
}/* end of WriteUCHARI2C */

/*^^*
 * Function:    ReadUCHARI2C
 *
 * Purpose:             To read a UCHAR of data from the I2C bus.
 *
 * Inputs:              lpI2C_ContextData : PI2C_CONTEXT, pointer to data struct associated with
 *                                          lpI2C_ContextData being accessed
 *
 * Outputs:             UCHAR, the data that was read.
 *^^*/
BOOLEAN ReadUCHARI2C(PXGI_HW_DEVICE_INFO pHWDE,  PUCHAR pBuffer)
{
    ULONG       ulReadData, data, i, j, delay, delay2;

    delay = pHWDE->I2CDelay / 10 / 2;

    vWriteDataLineDVI(pHWDE, HIDAT);
    DelayUS(delay);

    ulReadData = 0;
    for (j = 0; j < 8; j++)
    {
        vWriteClockLineDVI(pHWDE, HICLK);
        DelayUS(delay);
        if (bReadClockLineDVI(pHWDE) != HICLK)  {
            i = 0;
            delay2 = delay * 2;
            do {
                vWriteClockLineDVI(pHWDE, HICLK);
                DelayUS(delay2);
                if (bReadClockLineDVI(pHWDE) == HICLK)  break;
                i++;
                delay2 *= 2;
                if (i >= I2C_RETRY_COUNT)  return FALSE;
            } while (1);
        }

        data = bReadDataLineDVI(pHWDE);
        ulReadData = (ulReadData << 1) | (data & 1);

        vWriteClockLineDVI(pHWDE, LOCLK);
        DelayUS(delay);

        vWriteDataLineDVI(pHWDE, HIDAT);
        DelayUS(delay);
    }

    *pBuffer = (UCHAR) ulReadData;
    return TRUE;
}


UCHAR ReverseUCHAR(UCHAR data)
{
    UCHAR  rdata = 0;
    int   i;

    for (i=0; i<8; i++)
    {
        rdata <<= 1;
        rdata |= (data & 1);
        data  >>= 1;
    }

    return(rdata);
}

/************************************************************************************
 For DVI I2C Interface
***************************************************************************************/

/*************************************************************************
// VOID vWriteClockLineDVI()
// IOReg xx14, index 0F is defined as follows:
//
//      ...    1      0
// --------|------|-------|
//      ...| Data | Clock |
// ------------------------
*************************************************************************/
VOID vWriteClockLineDVI(PXGI_HW_DEVICE_INFO pHWDE, UCHAR data)
{
    UCHAR       temp;
    PUCHAR      pjI2cIOBase;

    if ((pHWDE->jChipType < XG21)&&(pHWDE->jChipType != XG27))
    {
    	  pjI2cIOBase = pHWDE->pjIOAddress + VB_PART4_OFFSET_14;
        pHWDE->ucI2cDVI = (pHWDE->ucI2cDVI & MASK(1:1)) | SETBITS(data, 0:0);

        temp = XGINew_GetReg1(pjI2cIOBase, 0x0F);
        temp = (temp & (~MASK(1:0))) | pHWDE->ucI2cDVI;

        XGINew_SetReg1(pjI2cIOBase,0x0F, temp);
    }
    else
    {
        pjI2cIOBase = pHWDE->pjIOAddress + CRTC_ADDRESS_PORT_COLOR;

        /* Enable GPIOA Write */

        EnableGPIOA(pjI2cIOBase, WRITE);

        pHWDE->ucI2cDVI = (pHWDE->ucI2cDVI & MASK(1:1)) | SETBITS(data, 0:0);

        temp = XGINew_GetReg1(pjI2cIOBase, IND_CR48_GPIO_REG_I);
        {
            UCHAR temp2 = getGPIORWTransfer(temp);
            temp = temp2;
        }
        temp = (temp & (~MASK(1:0))) | pHWDE->ucI2cDVI;
        XGINew_SetReg1(pjI2cIOBase,IND_CR48_GPIO_REG_I, temp);
    }


    
}

/*************************************************************************
// VOID vWriteDataLineDVI()
// IOReg xx14, index 0F is defined as follows:
//
//      ...    1      0
// --------|------|-------|
//      ...| Data | Clock |
// ------------------------
*************************************************************************/
VOID vWriteDataLineDVI(PXGI_HW_DEVICE_INFO pHWDE, UCHAR data)
{
    UCHAR       temp;
    PUCHAR      pjI2cIOBase;

    if ((pHWDE->jChipType < XG21)&&(pHWDE->jChipType != XG27))
    {
        pjI2cIOBase = pHWDE->pjIOAddress + VB_PART4_OFFSET_14;
        pHWDE->ucI2cDVI = (pHWDE->ucI2cDVI & MASK(0:0)) | SETBITS(data, 1:1);

        temp = XGINew_GetReg1(pjI2cIOBase, 0x0F);
        temp = (temp & (~MASK(1:0))) | pHWDE->ucI2cDVI;

        XGINew_SetReg1(pjI2cIOBase,0x0F, temp);
    
    }
    else
    {
        pjI2cIOBase = pHWDE->pjIOAddress + CRTC_ADDRESS_PORT_COLOR;

        
        /* Enable GPIOB Write */
        
        EnableGPIOB(pjI2cIOBase, WRITE);

        pHWDE->ucI2cDVI = (pHWDE->ucI2cDVI & MASK(0:0)) | SETBITS(data, 1:1);

        temp = XGINew_GetReg1(pjI2cIOBase, IND_CR48_GPIO_REG_I);

        {
            UCHAR temp2 = getGPIORWTransfer(temp);

            temp = temp2;
        }

        temp = (temp & (~MASK(1:0))) | pHWDE->ucI2cDVI;
        XGINew_SetReg1(pjI2cIOBase,IND_CR48_GPIO_REG_I, temp);
    }

    
}

/*************************************************************************
// BOOLEAN bReadClockLineDVI()
// IOReg xx14, index 0F is defined as follows:
//
//      ...    1      0
// --------|------|-------|
//      ...| Data | Clock |
// ------------------------
*************************************************************************/
BOOLEAN bReadClockLineDVI(PXGI_HW_DEVICE_INFO pHWDE)
{
    UCHAR   cPortData;
    PUCHAR  pjI2cIOBase;


    if ((pHWDE->jChipType != XG21)&&(pHWDE->jChipType != XG27))
    {

        pjI2cIOBase = pHWDE->pjIOAddress + VB_PART4_OFFSET_14;

        cPortData = XGINew_GetReg1(pjI2cIOBase, 0x0F);
        cPortData = GETBITS(cPortData, 0:0);

    }
    else
    {  
        pjI2cIOBase = pHWDE->pjIOAddress + CRTC_ADDRESS_PORT_COLOR;

        /* Enable GPIOA READ */

        EnableGPIOA(pjI2cIOBase, READ);

        cPortData = XGINew_GetReg1(pjI2cIOBase, IND_CR48_GPIO_REG_I);
        cPortData = GETBITS(cPortData, 7:7);
    }


    return cPortData;
}

/*************************************************************************
// BOOLEAN bReadDataLineDVI()
// IOReg xx14, index 0F is defined as follows:
//
//      ...    1      0
// --------|------|-------|
//      ...| Data | Clock |
// ------------------------
*************************************************************************/
BOOLEAN bReadDataLineDVI(PXGI_HW_DEVICE_INFO pHWDE)
{
    UCHAR       cPortData;
    PUCHAR      pjI2cIOBase; 


    if ((pHWDE->jChipType != XG21)&&(pHWDE->jChipType != XG27))
    {

        pjI2cIOBase = pHWDE->pjIOAddress + VB_PART4_OFFSET_14;

        cPortData = XGINew_GetReg1(pjI2cIOBase, 0x0F);
        cPortData = GETBITS(cPortData, 1:1);
    }
    else
    {
        pjI2cIOBase = pHWDE->pjIOAddress + CRTC_ADDRESS_PORT_COLOR;

        
        /* Enable GPIOB Write */
        
        EnableGPIOB(pjI2cIOBase, READ);

        cPortData = XGINew_GetReg1(pjI2cIOBase, IND_CR48_GPIO_REG_I);
        cPortData = GETBITS(cPortData, 6:6);
    }


    return cPortData;
}

BOOLEAN bEDIDCheckSum(PUCHAR  pjEDIDBuf,ULONG   ulBufSize)
{
    ULONG  i;
    UCHAR  ujSum = 0;
    PUCHAR pujPtr;

    pujPtr = pjEDIDBuf;

    for (i = 0; i < ulBufSize; i++)
    {
  /*     printk("pujPtr=%x, ",*pujPtr); */
       ujSum += *(pujPtr++);
    } /*for-loop */

    return(ujSum);

} 
