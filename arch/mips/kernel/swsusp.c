#include <asm/page.h>
#include <linux/preempt.h>

extern const void __nosave_begin, __nosave_end;
/*
static struct __saved_context 
{
	unsigned long v[2];
	unsigned long a[4];
	unsigned long t[10];
	unsigned long s[8];
	unsigned long k[2];
	unsigned long gp;
	unsigned long sp;
	unsigned long fp;
	unsigned long ra;
	unsigned long hi;
	unsigned long lo;
	unsigned long cp0[32];
	
} saved_context;
*/
void __save_processor_state(void)
{
      // preempt_disable();
/*
       asm volatile ("dsubu $29, 8");
       asm volatile ("sd $2, ($29)");

       asm volatile ("sd $3, (%0)" : : "r" (&saved_context.v[1]));
       asm volatile ("sd $4, (%0)" : : "r" (&saved_context.a[0]));
       asm volatile ("sd $5, (%0)" : : "r" (&saved_context.a[1]));
       asm volatile ("sd $6, (%0)" : : "r" (&saved_context.a[2]));
       asm volatile ("sd $7, (%0)" : : "r" (&saved_context.a[3]));
       asm volatile ("sd $8, (%0)" : : "r" (&saved_context.t[0]));
       asm volatile ("sd $9, (%0)" : : "r" (&saved_context.t[1]));
       asm volatile ("sd $10, (%0)" : : "r" (&saved_context.t[2]));
       asm volatile ("sd $11, (%0)" : : "r" (&saved_context.t[3]));
       asm volatile ("sd $12, (%0)" : : "r" (&saved_context.t[4]));
       asm volatile ("sd $13, (%0)" : : "r" (&saved_context.t[5]));
       asm volatile ("sd $14, (%0)" : : "r" (&saved_context.t[6]));
       asm volatile ("sd $15, (%0)" : : "r" (&saved_context.t[7]));
       asm volatile ("sd $16, (%0)" : : "r" (&saved_context.s[0]));
       asm volatile ("sd $17, (%0)" : : "r" (&saved_context.s[1]));
       asm volatile ("sd $18, (%0)" : : "r" (&saved_context.s[2]));
       asm volatile ("sd $19, (%0)" : : "r" (&saved_context.s[3]));
       asm volatile ("sd $20, (%0)" : : "r" (&saved_context.s[4]));
       asm volatile ("sd $21, (%0)" : : "r" (&saved_context.s[5]));
       asm volatile ("sd $22, (%0)" : : "r" (&saved_context.s[6]));
       asm volatile ("sd $23, (%0)" : : "r" (&saved_context.s[7]));
       asm volatile ("sd $24, (%0)" : : "r" (&saved_context.t[8]));
       asm volatile ("sd $25, (%0)" : : "r" (&saved_context.t[9]));
       asm volatile ("sd $26, (%0)" : : "r" (&saved_context.k[0]));
       asm volatile ("sd $27, (%0)" : : "r" (&saved_context.k[1]));
       asm volatile ("sd $28, (%0)" : : "r" (&saved_context.gp));
       asm volatile ("sd $30, (%0)" : : "r" (&saved_context.fp));
       asm volatile ("sd $31, (%0)" : : "r" (&saved_context.ra));

       asm volatile ("ld $2, ($29)");
       asm volatile ("daddu $29, 8");

       asm volatile ("sd $2, (%0)" : : "r" (&saved_context.v[0]) : "$2");
       asm volatile ("sd $29, (%0)" : : "r" (&saved_context.sp));
*/
       /*
        * special registers
        */
/*       asm volatile ("mfhi %0" : "=r" (saved_context.hi));
       asm volatile ("mflo %0" : "=r" (saved_context.lo));
  */     // load/link register??

       /*
        * coprocessor 0 registers (inclde/asm-mips/mipsregs.h)
        */
	   /*
       asm volatile ("dmfc0 %0, $0" : "=r" (saved_context.cp0[0]));
       asm volatile ("dmfc0 %0, $1" : "=r" (saved_context.cp0[1]));
       asm volatile ("dmfc0 %0, $2" : "=r" (saved_context.cp0[2]));
       asm volatile ("dmfc0 %0, $3" : "=r" (saved_context.cp0[3]));
       asm volatile ("dmfc0 %0, $4" : "=r" (saved_context.cp0[4]));
       asm volatile ("dmfc0 %0, $5" : "=r" (saved_context.cp0[5]));
       asm volatile ("dmfc0 %0, $6" : "=r" (saved_context.cp0[6]));
       asm volatile ("dmfc0 %0, $7" : "=r" (saved_context.cp0[7]));
       asm volatile ("dmfc0 %0, $8" : "=r" (saved_context.cp0[8]));
       asm volatile ("dmfc0 %0, $9" : "=r" (saved_context.cp0[9]));
       asm volatile ("dmfc0 %0, $10" : "=r" (saved_context.cp0[10]));
       asm volatile ("dmfc0 %0, $11" : "=r" (saved_context.cp0[11]));
       asm volatile ("dmfc0 %0, $12" : "=r" (saved_context.cp0[12]));
       asm volatile ("dmfc0 %0, $13" : "=r" (saved_context.cp0[13]));
       asm volatile ("dmfc0 %0, $14" : "=r" (saved_context.cp0[14]));
       asm volatile ("dmfc0 %0, $15" : "=r" (saved_context.cp0[15]));
       asm volatile ("dmfc0 %0, $16" : "=r" (saved_context.cp0[16]));
       asm volatile ("dmfc0 %0, $17" : "=r" (saved_context.cp0[17]));
       asm volatile ("dmfc0 %0, $18" : "=r" (saved_context.cp0[18]));
       asm volatile ("dmfc0 %0, $19" : "=r" (saved_context.cp0[19]));
       asm volatile ("dmfc0 %0, $20" : "=r" (saved_context.cp0[20]));
       asm volatile ("dmfc0 %0, $21" : "=r" (saved_context.cp0[21]));
       asm volatile ("dmfc0 %0, $22" : "=r" (saved_context.cp0[22]));
       asm volatile ("dmfc0 %0, $23" : "=r" (saved_context.cp0[23]));
       asm volatile ("dmfc0 %0, $24" : "=r" (saved_context.cp0[24]));
       asm volatile ("dmfc0 %0, $25" : "=r" (saved_context.cp0[25]));
       asm volatile ("dmfc0 %0, $26" : "=r" (saved_context.cp0[26]));
       asm volatile ("dmfc0 %0, $27" : "=r" (saved_context.cp0[27]));
       asm volatile ("dmfc0 %0, $28" : "=r" (saved_context.cp0[28]));
       asm volatile ("dmfc0 %0, $29" : "=r" (saved_context.cp0[29]));
       asm volatile ("dmfc0 %0, $30" : "=r" (saved_context.cp0[30]));
       asm volatile ("dmfc0 %0, $31" : "=r" (saved_context.cp0[31]));
	   */
}

void save_processor_state(void)
{
       printk("[DEBUG] before __save_processor_state() %s,%d\n",__FILE__,__LINE__);
       __save_processor_state();
       printk("[DEBUG] after __save_processor_state() %s,%d\n",__FILE__,__LINE__);
}

void __restore_processor_state(void)
{
       /*
        * first restore %ds, so we can access our data properly
        */
   //    asm volatile (".align 4");
//      asm volatile ("movw %0, %%ds" :: "r" ((u16)__KERNEL_DS));


       /*
        * coprocessor 0 registers (inclde/asm-mips/mipsregs.h)
        */
	   /*
       asm volatile ("dmtc0 %0, $0" : : "r" (saved_context.cp0[0]));
       asm volatile ("dmtc0 %0, $1" : : "r" (saved_context.cp0[1]));
       asm volatile ("dmtc0 %0, $2" : : "r" (saved_context.cp0[2]));
       asm volatile ("dmtc0 %0, $3" : : "r" (saved_context.cp0[3]));
       asm volatile ("dmtc0 %0, $4" : : "r" (saved_context.cp0[4]));
       asm volatile ("dmtc0 %0, $5" : : "r" (saved_context.cp0[5]));
       asm volatile ("dmtc0 %0, $6" : : "r" (saved_context.cp0[6]));
       asm volatile ("dmtc0 %0, $7" : : "r" (saved_context.cp0[7]));
       asm volatile ("dmtc0 %0, $8" : : "r" (saved_context.cp0[8]));
       asm volatile ("dmtc0 %0, $9" : : "r" (saved_context.cp0[9]));
       asm volatile ("dmtc0 %0, $10" : : "r" (saved_context.cp0[10]));
       asm volatile ("dmtc0 %0, $11" : : "r" (saved_context.cp0[11]));
       asm volatile ("dmtc0 %0, $12" : : "r" (saved_context.cp0[12]));
       asm volatile ("dmtc0 %0, $13" : : "r" (saved_context.cp0[13]));
       asm volatile ("dmtc0 %0, $14" : : "r" (saved_context.cp0[14]));
       asm volatile ("dmtc0 %0, $15" : : "r" (saved_context.cp0[15]));
       asm volatile ("dmtc0 %0, $16" : : "r" (saved_context.cp0[16]));
       asm volatile ("dmtc0 %0, $17" : : "r" (saved_context.cp0[17]));
       asm volatile ("dmtc0 %0, $18" : : "r" (saved_context.cp0[18]));
       asm volatile ("dmtc0 %0, $19" : : "r" (saved_context.cp0[19]));
       asm volatile ("dmtc0 %0, $20" : : "r" (saved_context.cp0[20]));
       asm volatile ("dmtc0 %0, $21" : : "r" (saved_context.cp0[21]));
       asm volatile ("dmtc0 %0, $22" : : "r" (saved_context.cp0[22]));
       asm volatile ("dmtc0 %0, $23" : : "r" (saved_context.cp0[23]));
       asm volatile ("dmtc0 %0, $24" : : "r" (saved_context.cp0[24]));
       asm volatile ("dmtc0 %0, $25" : : "r" (saved_context.cp0[25]));
       asm volatile ("dmtc0 %0, $26" : : "r" (saved_context.cp0[26]));
       asm volatile ("dmtc0 %0, $27" : : "r" (saved_context.cp0[27]));
       asm volatile ("dmtc0 %0, $28" : : "r" (saved_context.cp0[28]));
       asm volatile ("dmtc0 %0, $29" : : "r" (saved_context.cp0[29]));
       asm volatile ("dmtc0 %0, $30" : : "r" (saved_context.cp0[30]));
       asm volatile ("dmtc0 %0, $31" : : "r" (saved_context.cp0[31]));
*/
       /*
        * special registers
        */
  /*     asm volatile ("mthi %0" : : "r" (saved_context.hi));
       asm volatile ("mtlo %0" : : "r" (saved_context.lo));
*/
       /*
        * the other general registers
        *
        * note that even though gcc has constructs to specify memory
        * input into certain registers, it will try to be too smart
        * and save them at the beginning of the function.  This is esp.
        * bad since we don't have a stack set up when we enter, and we
        * want to preserve the values on exit. So, we set them manually.
        */
 /*      asm volatile ("ld $3, (%0)" : : "r" (&saved_context.v[1]));
       asm volatile ("ld $4, (%0)" : : "r" (&saved_context.a[0]));
       asm volatile ("ld $5, (%0)" : : "r" (&saved_context.a[1]));
       asm volatile ("ld $6, (%0)" : : "r" (&saved_context.a[2]));
       asm volatile ("ld $7, (%0)" : : "r" (&saved_context.a[3]));
       asm volatile ("ld $8, (%0)" : : "r" (&saved_context.t[0]));
       asm volatile ("ld $9, (%0)" : : "r" (&saved_context.t[1]));
       asm volatile ("ld $10, (%0)" : : "r" (&saved_context.t[2]));
       asm volatile ("ld $11, (%0)" : : "r" (&saved_context.t[3]));
       asm volatile ("ld $12, (%0)" : : "r" (&saved_context.t[4]));
       asm volatile ("ld $13, (%0)" : : "r" (&saved_context.t[5]));
       asm volatile ("ld $14, (%0)" : : "r" (&saved_context.t[6]));
       asm volatile ("ld $15, (%0)" : : "r" (&saved_context.t[7]));
       asm volatile ("ld $16, (%0)" : : "r" (&saved_context.s[0]));
       asm volatile ("ld $17, (%0)" : : "r" (&saved_context.s[1]));
       asm volatile ("ld $18, (%0)" : : "r" (&saved_context.s[2]));
       asm volatile ("ld $19, (%0)" : : "r" (&saved_context.s[3]));
       asm volatile ("ld $20, (%0)" : : "r" (&saved_context.s[4]));
       asm volatile ("ld $21, (%0)" : : "r" (&saved_context.s[5]));
       asm volatile ("ld $22, (%0)" : : "r" (&saved_context.s[6]));
       asm volatile ("ld $23, (%0)" : : "r" (&saved_context.s[7]));
       asm volatile ("ld $24, (%0)" : : "r" (&saved_context.t[8]));
       asm volatile ("ld $25, (%0)" : : "r" (&saved_context.t[9]));
       asm volatile ("ld $26, (%0)" : : "r" (&saved_context.k[0]));
       asm volatile ("ld $27, (%0)" : : "r" (&saved_context.k[1]));
       asm volatile ("ld $28, (%0)" : : "r" (&saved_context.gp));
       asm volatile ("ld $29, (%0)" : : "r" (&saved_context.sp));
       asm volatile ("ld $30, (%0)" : : "r" (&saved_context.fp));
       asm volatile ("ld $31, (%0)" : : "r" (&saved_context.ra));
       // Good job 'v0'. It's your turn!
       asm volatile ("ld $2, (%0)" : : "r" (&saved_context.v[0]));
*/
       //preempt_enable();
}

void restore_processor_state(void)
{
       printk("[DEBUG] before __restore_processor_state() %s,%d\n",__FILE__,__LINE__);
       __restore_processor_state();
       printk("[DEBUG] after __restore_processor_state() %s,%d\n",__FILE__,__LINE__);
}

int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = __pa(&__nosave_begin) >> PAGE_SHIFT;
	unsigned long nosave_end_pfn   = PAGE_ALIGN(__pa(&__nosave_end)) >> PAGE_SHIFT;

	return (pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}
