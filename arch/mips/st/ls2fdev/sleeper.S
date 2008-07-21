/*
 * Copyright 2002 Embedded Edge, LLC
 * Author: dan@embeddededge.com
 *
 * Sleep helper for Au1xxx sleep mode.
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General	 Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <asm/cacheops.h>

	.text
	.set	macro
	.set	noat
	.align	5

/* Save all of the processor general registers and go to sleep.
 * A wakeup condition will get us back here to restore the registers.
 */

LEAF(do_suspend_lowlevel)
	dli k0, 0xffffffffafe00000

	sd	$1, 0x00(k0)
	sd	$2, 0x08(k0)
	sd	$3, 0x10(k0)
	sd	$4, 0x18(k0) 
	sd	$5, 0x20(k0)
	sd	$6, 0x28(k0)
	sd	$7, 0x30(k0)
	sd	$8, 0x38(k0)
	sd	$9, 0x40(k0)
	sd	$10, 0x48(k0)
	sd	$11, 0x50(k0)
	sd	$12, 0x58(k0)
	sd	$13, 0x60(k0)
	sd	$14, 0x68(k0)
	sd	$15, 0x70(k0)
	sd	$16, 0x78(k0)
	sd	$17, 0x80(k0)
	sd	$18, 0x88(k0)
	sd	$19, 0x90(k0)
	sd	$20, 0x98(k0)
	sd	$21, 0xa0(k0)
	sd	$22, 0xa8(k0)
	sd	$23, 0xb0(k0)
	sd	$24, 0xb8(k0)
	sd	$25, 0xc0(k0)
	sd	$26, 0xc8(k0)
	sd	$27, 0xd0(k0)
	sd	$28, 0xd8(k0)
	sd	$29, 0xe0(k0)
	sd	$30, 0xe8(k0)
	sd	$31, 0xf0(k0)
	ssnop

	dmfc0 k1, $0
	sd k1, 0x100(k0)
	ssnop

	dmfc0 k1, $1
	sd k1, 0x108(k0)
	ssnop

	dmfc0 k1, $2
	sd k1, 0x110(k0)
	ssnop

	dmfc0 k1, $3
	sd k1, 0x118(k0)
	ssnop

	dmfc0 k1, $4
	sd k1, 0x120(k0)
	ssnop

	dmfc0 k1, $5
	sd k1, 0x128(k0)
	ssnop

	dmfc0 k1, $6
	sd k1, 0x130(k0)
	ssnop

	dmfc0 k1, $7
	sd k1, 0x138(k0)
	ssnop

	dmfc0 k1, $8
	sd k1, 0x140(k0)
	ssnop

	dmfc0 k1, $9
	sd k1, 0x148(k0)
	ssnop

	dmfc0 k1, $10
	sd k1, 0x150(k0)
	ssnop

	dmfc0 k1, $11
	sd k1, 0x158(k0)
	ssnop

	dmfc0 k1, $12
	sd k1, 0x160(k0)
	ssnop

	dmfc0 k1, $13
	sd k1, 0x168(k0)
	ssnop

	dmfc0 k1, $14
	sd k1, 0x170(k0)
	ssnop

	dmfc0 k1, $15
	sd k1, 0x178(k0)
	ssnop

	dmfc0 k1, $16
	sd k1, 0x180(k0)
	ssnop

	dmfc0 k1, $17
	sd k1, 0x188(k0)
	ssnop

	dmfc0 k1, $18
	sd k1, 0x190(k0)
	ssnop
	
	dmfc0 k1, $19
	sd k1, 0x198(k0)
	ssnop
	
	dmfc0 k1, $20
	sd k1, 0x1a0(k0)
	ssnop
	
	dmfc0 k1, $21
	sd k1, 0x1a8(k0)
	ssnop
	
	dmfc0 k1, $22
	sd k1, 0x1b0(k0)
	ssnop
	
	dmfc0 k1, $23
	sd k1, 0x1b8(k0)
	ssnop
	
	dmfc0 k1, $24
	sd k1, 0x1c0(k0)
	ssnop
	
	dmfc0 k1, $25
	sd k1, 0x1c8(k0)
	ssnop

	dmfc0 k1, $26
	sd k1, 0x1d0(k0)
	ssnop

	dmfc0 k1, $27
	sd k1, 0x1d8(k0)
	ssnop

	dmfc0 k1, $28
	sd k1, 0x1e0(k0)
	ssnop

	dmfc0 k1, $29
	sd k1, 0x1e8(k0)
	ssnop

	dmfc0 k1, $30
	sd k1, 0x1f0(k0)
	ssnop

	dmfc0 k1, $31
	sd k1, 0x1f8(k0)
	ssnop

	mfhi k1
	sd k1, 0x208(k0)
	ssnop
	
	mflo k1
	sd k1, 0x210(k0)
	ssnop

	sync
	ssnop
/*
	dli k1, 0xffffffff80389250
	dli a0, 0x00
	dli a1, 0xffffffffa0000000
	dli a2, 0xfe00000
	
	jalr k1
	nop

	sd v0, 0x208(k0)
	ssnop
*/

	/* Now set up the scratch registers so the boot rom will
	 * return to this point upon wakeup.
	 */
	dli k1, 0xffffffffafe00000
	dla	k0, wakeup
	nop
	sd	k0, 0x200(k1)
	sync
	ssnop
	nop

/* Put SDRAM into self refresh.  Preload instructions into cache,
 * issue a precharge, then auto refresh, then sleep commands to it.
 */
/*
 	la	t0, sdsleep
	.set	mips3
 	cache	0x14, 0(t0)
 	cache	0x14, 32(t0)
 	cache	0x14, 64(t0)
 	cache	0x14, 96(t0)
	.set	mips0
*/
sdsleep:
/*
	and $1, zero
	and $2, zero
	and $3, zero
	and $4, zero
	and $5, zero
	and $6, zero
	and $7, zero
	and $8, zero
	and $9, zero
	and $10, zero
	and $11, zero
	and $12, zero
	and $13, zero
	and $14, zero
	and $15, zero
	and $16, zero
	and $17, zero
	and $18, zero
	and $19, zero
	and $20, zero
	and $21, zero
	and $22, zero
	and $23, zero
	and $24, zero
	and $25, zero
	and $26, zero
	and $27, zero
	and $28, zero
	and $29, zero
	and $30, zero
	and $31, zero
	ssnop
	ehb
*/	
	dli k0, 0xffffffff80000000
	dli k1, 0xffffffff8f000000
1:
	ld zero, 0(k0)
	daddiu k0,k0,8
	bne k0,k1,1b
	nop
	ssnop

	dli k0, 0x9800000000000000
	dli k1, 0x980000000f000000
1:
	ld zero, 0(k0)
	daddiu k0,k0,8
	bne k0,k1,1b
	nop
	ssnop
/*	
	dli k0, 0xffffffff803896d0
	dli k1, 0xffffffffafe00300
	move a0, zero
	dli a1, 0xffffffffa0000000
	li a2,  0xfe00000

	jalr k0
	nop
	ssnop
	sd v0, 0(k1)
	ssnop

	move a0, zero
	dli a1, 0xffffffff80000000
	li a2, 0xfe00000
	jalr k0
	nop
	sd v0, 8(k1)
	ssnop

*/	
	.set at
	dli k0, 0xffffffffbfd00000
	li k1, 0x04
	sb k1, 0xeee5(k0)
	sb zero, 0xeee6(k0)
	li k1, 0x24
	sb k1, 0xeee4(k0)
	sb zero, 0xeee3(k0)
	li k1, 0x08
	sb k1, 0xeee2(k0)
	lb k1, 0xeee0(k0)
	andi k1,k1,0x1f
	beqz k1, 1f
	nop
	sb k1, 0xeee0(k0)
	lb k1, 0xeee0(k0)
	sync
1:
	lb k1, 0xeee2(k0)
	ori k1, k1, 0x40
	sb k1, 0xeee2(k0)
	sync
	dla k0, 1f
	ssnop
	
	ld zero, 0x00(k0)
	ld zero, 0x08(k0)
	ld zero, 0x10(k0)
	ld zero, 0x18(k0)
	ld zero, 0x20(k0)
	ld zero, 0x28(k0)
	ld zero, 0x30(k0)
	ld zero, 0x38(k0)
	ld zero, 0x40(k0)
	ld zero, 0x48(k0)
	ld zero, 0x50(k0)
	ld zero, 0x58(k0)
	ld zero, 0x60(k0)
	ld zero, 0x68(k0)
	ld zero, 0x70(k0)
	ld zero, 0x78(k0)
	ld zero, 0x80(k0)
	ld zero, 0x88(k0)
	ld zero, 0x90(k0)
	ld zero, 0x98(k0)
	ld zero, 0xa0(k0)
	ld zero, 0xa8(k0)
	
1:	
	dli k0, 0xffffffffbfe00180
	lw k1, 0x0(k0)
	andi k1, k1, 0x06ff
	sw k1, 0x0(k0)
	ssnop
	
	dli k0, 0xffffffffaffffe00
	ld v0, 0x80(k0)
	sd zero, 0x80(k0)
	lw k1, 0x34(k0)
	ori k1, k1, 0x01
	sw k1, 0x34(k0)
	ssnop

2:
	nop
	b 2b 
	nop
	.set	noat

	/* This is where we return upon wakeup.
	 * Reload all of the registers and return.
	 */
	.align 4
wakeup:	
	ssnop
/*
	dli k0, 0xffffffff80000000

	dli k1, 0xffffffff8f000000
1:
	ld zero, 0(k0)
	daddiu k0,k0,8
	bne k0,k1,1b
	nop
*/	
	dli k0, 0x900000003ff00000
	dli k1, 0x0000000080000000
	ssnop
	sd k1, 0x10(k0)
	ssnop
	dli k1, 0xffffffff80000000
	ssnop
	sd k1, 0x30(k0)
	ssnop
	sd zero, 0x50(k0)
	ssnop

	dli k0, 0xffffffffbfe00000
	ssnop
	li k1, 0x46140
	ssnop
	sw k1, 0x110(k0)
	ssnop
	li k1, 0x80000000
	ssnop
	sw k1, 0x10(k0)
	ssnop
	sw zero,  0x14(k0)
	ssnop
	li k1, 0x8000000c
	ssnop
	sw k1, 0x150(k0)
	ssnop
	li k1, 0xffffffff
	ssnop
	sw k1, 0x154(k0)
	ssnop
	li k1, 0x00fe0105
	ssnop
	sw k1, 0x168(k0)
	ssnop

	ehb

	dli k0, 0xffffffffafe00000
	ssnop
	ld k1, 0x208(k0)
	mthi k1
	nop
	ld k1, 0x210(k0)
	mtlo k1
	nop
	ssnop
/*
	mfc0 t0, $10
	mtc0 zero, $2
	mtc0 zero, $3

	move v0, zero
	dli k0, 0xffffffff80000000
	move t1, zero
	or t1, t1, 64
1:
	dsll k1, v0, 15
	daddu v1, k0, k1
	dmtc0 v1, $10
	dmtc0 v0, $0
 	ehb
	tlbwi
	daddiu v0,1
	bne v0, t1, 1b
	nop
	mtc0 t0,$10
	ehb
*/

/*	
	dli t0, 0xffffffff803896d0
	dli t1, 0xffffffffafe00300
	move a0, zero
	dli a1, 0xffffffffa0000000
	li a2,  0xfe00000

	jalr t0
	nop
	ssnop
	sd v0, 0(t1)

	
	dli t0, 0xffffffff80000000

	dli t1, 0xffffffff8f000000
1:
	ld zero, 0(t0)
	daddiu t0,t0,8
	bne t0,t1,1b
	nop

	dli t0, 0xffffffff803896d0
	dli t1, 0xffffffffafe00300
	move a0, zero
	dli a1, 0xffffffff80000000
	li a2,  0xfe00000

	jalr t0
	nop
	ssnop
	sd v0, 0x10(t1)
*/
	dli k0, 0xffffffffafe00000
	ssnop
	ld	$1, 0x00(k0)
	ld	$2, 0x08(k0)
	ld	$3, 0x10(k0)
	ld	$4, 0x18(k0) 
	ld	$5, 0x20(k0)
	ld	$6, 0x28(k0)
	ld	$7, 0x30(k0)
	ld	$8, 0x38(k0)
	ld	$9, 0x40(k0)
	ld	$10, 0x48(k0)
	ld	$11, 0x50(k0)
	ld	$12, 0x58(k0)
	ld	$13, 0x60(k0)
	ld	$14, 0x68(k0)
	ld	$15, 0x70(k0)
	ld	$16, 0x78(k0)
	ld	$17, 0x80(k0)
	ld	$18, 0x88(k0)
	ld	$19, 0x90(k0)
	ld	$20, 0x98(k0)
	ld	$21, 0xa0(k0)
	ld	$22, 0xa8(k0)
	ld	$23, 0xb0(k0)
	ld	$24, 0xb8(k0)
	ld	$25, 0xc0(k0)
	ld	$26, 0xc8(k0)
	ld	$27, 0xd0(k0)
	ld	$28, 0xd8(k0)
	ld	$29, 0xe0(k0)
	ld	$30, 0xe8(k0)
	ld	$31, 0xf0(k0)
	ssnop
	nop
	jr	ra
	nop
END(do_suspend_lowlevel)

