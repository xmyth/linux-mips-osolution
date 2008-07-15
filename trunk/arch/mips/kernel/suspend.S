#include <asm/asm-offsets.h>
#include <asm/regdef.h>
#include <asm/asm.h>
	.text
	.align 4
LEAF(swsusp_arch_suspend)
	REG_S		s0, saved_reg_s0
	REG_S		s1, saved_reg_s1
	REG_S		s2, saved_reg_s2
	REG_S		s3, saved_reg_s3
	REG_S		s4, saved_reg_s4
	REG_S		s5, saved_reg_s5
	REG_S		s6, saved_reg_s6
	REG_S		s7, saved_reg_s7
	REG_S		k0, saved_reg_k0
	REG_S		k1, saved_reg_k1
	REG_S		v0, saved_reg_v0
	REG_S		v1, saved_reg_v1
	REG_S		t0, saved_reg_t0
	REG_S		t1, saved_reg_t1
	REG_S		t2, saved_reg_t2
	REG_S		t3, saved_reg_t3
	REG_S		a0, saved_reg_a0
	REG_S		a1, saved_reg_a1
	REG_S		a2, saved_reg_a2
	REG_S		a3, saved_reg_a3
	REG_S		sp, saved_reg_sp
	REG_S		gp, saved_reg_gp
	REG_S		fp, saved_reg_fp
	REG_S		ra, saved_reg_ra
	jal			swsusp_save
	nop
	REG_L		ra, saved_reg_ra
	jr			ra
	nop
END(swsusp_arch_suspend)

LEAF(swsusp_arch_resume)
	PTR_LA		s0, restore_pblist
	REG_L		s0,0(s0)
	ssnop
1:	PTR_L		s1, PBE_ADDRESS(s0)
	PTR_L		s2, PBE_ORIG_ADDRESS(s0)
	ssnop
	PTR_ADDIU	s3, s1, _PAGE_SIZE
2:	REG_L		s4, 0(s1)
	ssnop
	REG_S		s4, 0(s2)
	PTR_ADDIU	s1, s1, SZREG
	PTR_ADDIU	s2, s2, SZREG
	bne			s1, s3, 2b
	nop
	PTR_L		s0, PBE_NEXT(s0)
	ssnop
	bnez		s0, 1b
	nop

//	ld s0, 0
	nop

	
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

	li v0, 0
*/
    REG_L		s0, saved_reg_s0
    REG_L		s1, saved_reg_s1
    REG_L		s2, saved_reg_s2
    REG_L		s3, saved_reg_s3
    REG_L		s4, saved_reg_s4
    REG_L		s5, saved_reg_s5
    REG_L		s6, saved_reg_s6
    REG_L		s7, saved_reg_s7
	REG_L		k0, saved_reg_k0
	REG_L		k1, saved_reg_k1
	REG_L		v0, saved_reg_v0
	REG_L		v1, saved_reg_v1
	REG_L		t0, saved_reg_t0
	REG_L		t1, saved_reg_t1
	REG_L		t2, saved_reg_t2
	REG_L		t3, saved_reg_t3
	REG_L		a0, saved_reg_a0
	REG_L		a1, saved_reg_a1
	REG_L		a2, saved_reg_a2
	REG_L		a3, saved_reg_a3
	REG_L		sp, saved_reg_sp
	REG_L		gp, saved_reg_gp
	REG_L		fp, saved_reg_fp
    REG_L		ra, saved_reg_ra
    jr 			ra
	nop
END(swsusp_arch_resume)


	.data
	.align 4
saved_reg_ra: .dword 0
saved_reg_s0: .dword 0
saved_reg_s1: .dword 0
saved_reg_s2: .dword 0
saved_reg_s3: .dword 0
saved_reg_s4: .dword 0
saved_reg_s5: .dword 0
saved_reg_s6: .dword 0
saved_reg_s7: .dword 0
saved_reg_k0: .dword 0
saved_reg_k1: .dword 0
saved_reg_t0: .dword 0
saved_reg_t1: .dword 0
saved_reg_t2: .dword 0
saved_reg_t3: .dword 0
saved_reg_v0: .dword 0
saved_reg_v1: .dword 0
saved_reg_a0: .dword 0
saved_reg_a1: .dword 0
saved_reg_a2: .dword 0
saved_reg_a3: .dword 0
saved_reg_sp: .dword 0
saved_reg_gp: .dword 0
saved_reg_fp: .dword 0

