#include <asm/asm-offsets.h>
#include <asm/regdef.h>
#include <asm/asm.h>
	.text
	.align 4
LEAF(swsusp_arch_suspend)
	REG_S		ra, saved_reg_ra
	jal			swsusp_save
	nop
	REG_L		ra, saved_reg_ra
	jr			ra
	nop
END(swsusp_arch_suspend)

LEAF(swsusp_arch_resume)
	PTR_LA		t0, restore_pblist
1:	PTR_L		t1, PBE_ADDRESS(t0)
	PTR_L		t2, PBE_ORIG_ADDRESS(t0)
	PTR_ADDIU	t3, t1, _PAGE_SIZE
2:	REG_L		k0, (t1)
	REG_S		k0, (t2)
	PTR_ADDIU	t1, t1, SZREG
	PTR_ADDIU	t2, t2, SZREG
	bne			t1, t3, 2b
	nop
	PTR_L		t0, PBE_NEXT(t0)
	bnez		t0, 1b
	nop
    REG_L		ra, saved_reg_ra
    jr 			ra
	nop
END(swsusp_arch_resume)


	.data
	.align 4
saved_reg_ra: .dword 0

