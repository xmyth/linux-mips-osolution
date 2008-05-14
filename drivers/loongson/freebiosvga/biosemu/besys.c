/****************************************************************************
*
*                        BIOS emulator and interface
*                      to Realmode X86 Emulator Library
*
*               Copyright (C) 1996-1999 SciTech Software, Inc.
*
*  ========================================================================
*
*  Permission to use, copy, modify, distribute, and sell this software and
*  its documentation for any purpose is hereby granted without fee,
*  provided that the above copyright notice appear in all copies and that
*  both that copyright notice and this permission notice appear in
*  supporting documentation, and that the name of the authors not be used
*  in advertising or publicity pertaining to distribution of the software
*  without specific, written prior permission.  The authors makes no
*  representations about the suitability of this software for any purpose.
*  It is provided "as is" without express or implied warranty.
*
*  THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
*  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
*  EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
*  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
*  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
*  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
*  PERFORMANCE OF THIS SOFTWARE.
*
*  ========================================================================
*
* Language:     ANSI C
* Environment:  Any
* Developer:    Kendall Bennett
*
* Description:  This file includes BIOS emulator I/O and memory access
*               functions.
*
****************************************************************************/
/* BE CAREFUL: outb here is using linux style outb(value,addr)
 * while the same function in pmon&xfree86 are different
 */

//#include <stdio.h>
//#include <dev/pci/pcivar.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/io.h>
#include "biosemui.h"

#define printf printk
#define linux_inb inb
#define linux_inw inw
#define linux_inl inl

#define linux_outb outb
#define linux_outw outw
#define linux_outl outl


/*------------------------------- Macros ----------------------------------*/

/* Macros to read and write values to x86 bus memory. Replace these as
 * necessary if you need to do something special to access memory over
 * the bus on a particular processor family.
 */

/* Use char read/write to avoid unaligned access */

#define READ8(addr,val) { val = *(u8*)(addr) ; }
#define READ16(addr,val) { if ((addr)&1) \
	                 val = ((*(u8*)(addr)) | ((*(u8*)((addr)+1)) << 8)); \
			else \
	                 val = *(u16*)(addr); \
		     }
#define READ32(addr,val) { if ( (addr)&3 ) \
	                 val = ( (*(u8*)(addr)) | ((*(u8*)((addr)+1)) << 8) | ( (*(u8*)((addr)+2))<<16) | ((*(u8*)((addr)+3))<<24) ); \
			else \
	                 val = *(u32*)(addr); \
		     }
#define WRITE8(addr,val) { *(u8*)(addr) = val; }
#define WRITE16(addr,val) { if ((addr)&1) { \
	                     *(u8*)(addr) = val; \
	                     *(u8*)((addr)+1) = val>>8; \
			   } else \
	                     *(u16*)(addr) = val; \
		     }
#define WRITE32(addr,val) { if ((addr) & 3) { \
	                       *(u8*)(addr) = val; \
	                       *(u8*)((addr)+1) = val>>8; \
	                       *(u8*)((addr)+2) = val>>16; \
	                       *(u8*)((addr)+3) = val>>24; \
			     } else \
	                       *(u32*)(addr) = val; \
		          }

/*----------------------------- Implementation ----------------------------*/

#ifdef DEBUG_EMU_VGA
# define DEBUG_MEM()        (M.x86.debug & DEBUG_MEM_TRACE_F)
#else
# define DEBUG_MEM()
#endif

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read

RETURNS:
Byte value read from emulator memory.

REMARKS:
Reads a byte value from the emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
u8 X86API BE_rdb(
    u32 addr)
{
    u8 val = 0;

    if (addr < M.mem_size) {
        READ8(M.mem_base + addr, val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        READ8(_BE_env.biosmem_base + addr - 0xC0000, val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        READ8(_BE_env.busmem_base + addr - 0xA0000, val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        READ8(_BE_env.sysmem_base + addr - 0xf0000, val);
    }else {
	printf("mem_read: address %#x out of range!\n", addr);
        HALT_SYS();
    }
DB( if (DEBUG_MEM())
        printf("%#08x 1 -> %#x\n", addr, val);)
    return val;
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read

RETURNS:
Word value read from emulator memory.

REMARKS:
Reads a word value from the emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
u16 X86API BE_rdw(
    u32 addr)
{
    u16 val = 0;

    if (addr < M.mem_size - 1 ) {
         READ16(M.mem_base + addr, val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
         READ16(_BE_env.biosmem_base + addr - 0xC0000, val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
         READ16(_BE_env.busmem_base + addr - 0xA0000, val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
         READ16(_BE_env.sysmem_base + addr - 0xf0000, val);
    }else {
	printf("mem_read: address %#x out of range!\n", addr);
        HALT_SYS();
    }
DB( if (DEBUG_MEM())
        printf("%#08x 2 -> %#x\n", addr, val);)
    return val;
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read

RETURNS:
Long value read from emulator memory.

REMARKS:
Reads a long value from the emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
u32 X86API BE_rdl(
    u32 addr)
{
    u32 val = 0;

    if (addr < M.mem_size - 3 ) {
         READ32(M.mem_base + addr, val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
         READ32(_BE_env.biosmem_base + addr - 0xC0000, val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
         READ32(_BE_env.busmem_base + addr - 0xA0000, val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
         READ32(_BE_env.sysmem_base + addr - 0xf0000, val);
    }else {
#ifdef DEBUG_EMU_VGA
	printf("mem_read: address %#x out of range!\n", addr);
#endif
        HALT_SYS();
    }
DB( if (DEBUG_MEM())
        printf("%#08x 4 -> %#x\n", addr, val);)
    return val;
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read
val     - Value to store

REMARKS:
Writes a byte value to emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
void X86API BE_wrb(
    u32 addr,
    u8 val)
{
DB( if (DEBUG_MEM())
        printf("%#08x 1 <- %#x\n", addr, val);)

    if (addr < M.mem_size) {
        WRITE8(M.mem_base + addr , val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        WRITE8(_BE_env.biosmem_base + addr - 0xC0000 , val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        WRITE8(_BE_env.busmem_base + addr - 0xA0000 , val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        WRITE8(_BE_env.sysmem_base + addr - 0xf0000 , val);
    }else {
#ifdef DEBUG_EMU_VGA
	printf("mem_write: address %#x out of range!\n", addr);
#endif
        HALT_SYS();
    }
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read
val     - Value to store

REMARKS:
Writes a word value to emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
void X86API BE_wrw(
    u32 addr,
    u16 val)
{
DB( if (DEBUG_MEM())
        printf("%#08x 2 <- %#x\n", addr, val);)
    if (addr < M.mem_size - 1) {
        WRITE16(M.mem_base + addr , val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        WRITE16(_BE_env.biosmem_base + addr - 0xC0000 , val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        WRITE16(_BE_env.busmem_base + addr - 0xA0000 , val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        WRITE16(_BE_env.sysmem_base + addr - 0xf0000 , val);
    }else {
	printf("mem_write: address %#x out of range!\n", addr);
        HALT_SYS();
    }
}

/****************************************************************************
PARAMETERS:
addr    - Emulator memory address to read
val     - Value to store

REMARKS:
Writes a long value to emulator memory. We have three distinct memory
regions that are handled differently, which this function handles.
****************************************************************************/
void X86API BE_wrl(
    u32 addr,
    u32 val)
{
DB( if (DEBUG_MEM())
        printf("%#08x 4 <- %#x\n", addr, val);)
    if (addr < M.mem_size - 3) {
        WRITE32(M.mem_base + addr , val);
    } else if (addr >= 0xC0000 && addr <= _BE_env.biosmem_limit) {
        WRITE32(_BE_env.biosmem_base + addr - 0xC0000 , val);
    } else if (addr >= 0xA0000 && addr < 0xC0000) {
        WRITE32(_BE_env.busmem_base + addr - 0xA0000 , val);
    } else if (addr >= 0xf0000 && addr < SYS_SIZE) {
        WRITE32(_BE_env.sysmem_base + addr - 0xf0000 , val);
    }else {
	printf("mem_write: address %#x out of range!\n", addr);
        HALT_SYS();
    }
}



u8 X86API BE_inb(int port)
{
    u8 val;
    val = linux_inb(port);
    return val;
}

u16 X86API BE_inw(int port)
{
    u16 val;

    val = inw(port);

    return val;
}

u32 X86API BE_inl(int port)
{
    u32 val;
    val = inl(port);
    
    return val;
}

void X86API BE_outb(int port, u8 val)
{
    outb(port, val);
}

void X86API BE_outw(int port, u16 val)
{
    outw(val,port);
}

void X86API BE_outl(int port, u32 val)
{
    linux_outl(val,port);
}

